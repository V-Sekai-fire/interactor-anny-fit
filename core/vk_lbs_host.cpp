// SPDX-License-Identifier: MIT
// Per-frame Vulkan compute host for the LBS kernel — see vk_lbs_host.h.  Refactor
// of the headless test in spec/tests/lbs_vulkan/vk_lbs.cpp into init / set-bind /
// dispatch / shutdown: buffers are host-visible + persistently mapped and the
// command buffer is recorded once, so each frame just writes the bone affines,
// submits, and reads the verts back.
//
// std430: StructuredBuffer<float3> has 16-byte stride (vec3 padded to vec4), so
// bind / verts use 4 floats per vertex; weights (float) and bone (float4) pack.
#include "vk_lbs_host.h"

#include "volk.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

static VkInstance g_inst;
static VkPhysicalDevice g_pd;
static VkDevice g_dev;
static VkQueue g_queue;
static uint32_t g_qf;
static VkShaderModule g_sm;
static VkDescriptorSetLayout g_dsl;
static VkPipelineLayout g_pl;
static VkPipeline g_pipe;
static VkDescriptorPool g_dp;
static VkDescriptorSet g_ds;
static VkCommandPool g_cmdPool;
static VkCommandBuffer g_cmd;
static unsigned g_V, g_J;
static bool g_ok = false;

struct Buf {
	VkBuffer buf;
	VkDeviceMemory mem;
	void *map;
};
static Buf g_bind, g_w, g_bone, g_verts;

#define VKOK(x)                  \
	do {                         \
		if ((x) != VK_SUCCESS) { \
			return 1;            \
		}                        \
	} while (0)

static uint32_t find_mem(uint32_t bits, VkMemoryPropertyFlags want) {
	VkPhysicalDeviceMemoryProperties mp;
	vkGetPhysicalDeviceMemoryProperties(g_pd, &mp);
	for (uint32_t i = 0; i < mp.memoryTypeCount; i++) {
		if ((bits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & want) == want) {
			return i;
		}
	}
	return UINT32_MAX;
}

static Buf make_buf(VkDeviceSize size) {
	Buf b{};
	VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bi.size = size;
	bi.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	vkCreateBuffer(g_dev, &bi, nullptr, &b.buf);
	VkMemoryRequirements mr;
	vkGetBufferMemoryRequirements(g_dev, b.buf, &mr);
	VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	ai.allocationSize = mr.size;
	ai.memoryTypeIndex = find_mem(mr.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	                                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkAllocateMemory(g_dev, &ai, nullptr, &b.mem);
	vkBindBufferMemory(g_dev, b.buf, b.mem, 0);
	vkMapMemory(g_dev, b.mem, 0, size, 0, &b.map);  // persistent map (host-coherent)
	return b;
}

void vk_lbs_set_bind(const float *bind_v3) {
	float *p = (float *)g_bind.map;
	for (unsigned v = 0; v < g_V; v++) {
		p[v * 4 + 0] = bind_v3[v * 3 + 0];
		p[v * 4 + 1] = bind_v3[v * 3 + 1];
		p[v * 4 + 2] = bind_v3[v * 3 + 2];
		p[v * 4 + 3] = 0.0f;
	}
}

int vk_lbs_init(const char *spv_path, const float *bind_v3, const float *weights_dense, unsigned V,
                unsigned J) {
	g_V = V;
	g_J = J;
	if (volkInitialize() != VK_SUCCESS) {
		return 1;  // no vulkan-1 loader → CPU fallback
	}
	VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	app.apiVersion = VK_API_VERSION_1_1;
	VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	ici.pApplicationInfo = &app;
	// MoltenVK (macOS) is a portability driver: enable enumeration when present.
	uint32_t nie = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &nie, nullptr);
	std::vector<VkExtensionProperties> ies(nie);
	vkEnumerateInstanceExtensionProperties(nullptr, &nie, ies.data());
	std::vector<const char *> instExt;
	for (VkExtensionProperties &e : ies) {
		if (strcmp(e.extensionName, "VK_KHR_portability_enumeration") == 0) {
			instExt.push_back("VK_KHR_portability_enumeration");
			ici.flags |= 0x00000001;  // VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
		}
	}
	ici.enabledExtensionCount = (uint32_t)instExt.size();
	ici.ppEnabledExtensionNames = instExt.data();
	if (vkCreateInstance(&ici, nullptr, &g_inst) != VK_SUCCESS) {
		return 1;
	}
	volkLoadInstance(g_inst);

	uint32_t nd = 0;
	vkEnumeratePhysicalDevices(g_inst, &nd, nullptr);
	std::vector<VkPhysicalDevice> pds(nd);
	vkEnumeratePhysicalDevices(g_inst, &nd, pds.data());
	g_pd = VK_NULL_HANDLE;
	for (VkPhysicalDevice cand : pds) {
		uint32_t nq = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(cand, &nq, nullptr);
		std::vector<VkQueueFamilyProperties> qs(nq);
		vkGetPhysicalDeviceQueueFamilyProperties(cand, &nq, qs.data());
		for (uint32_t i = 0; i < nq; i++) {
			if (qs[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
				g_pd = cand;
				g_qf = i;
				break;
			}
		}
		if (g_pd) {
			break;
		}
	}
	if (!g_pd) {
		return 1;
	}
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(g_pd, &props);

	float prio = 1.0f;
	VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	qci.queueFamilyIndex = g_qf;
	qci.queueCount = 1;
	qci.pQueuePriorities = &prio;
	VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	dci.queueCreateInfoCount = 1;
	dci.pQueueCreateInfos = &qci;
	uint32_t nde = 0;  // VK_KHR_portability_subset is required if the device exposes it
	vkEnumerateDeviceExtensionProperties(g_pd, nullptr, &nde, nullptr);
	std::vector<VkExtensionProperties> des(nde);
	vkEnumerateDeviceExtensionProperties(g_pd, nullptr, &nde, des.data());
	std::vector<const char *> devExt;
	for (VkExtensionProperties &e : des) {
		if (strcmp(e.extensionName, "VK_KHR_portability_subset") == 0) {
			devExt.push_back("VK_KHR_portability_subset");
		}
	}
	dci.enabledExtensionCount = (uint32_t)devExt.size();
	dci.ppEnabledExtensionNames = devExt.data();
	if (vkCreateDevice(g_pd, &dci, nullptr, &g_dev) != VK_SUCCESS) {
		return 1;
	}
	volkLoadDevice(g_dev);
	vkGetDeviceQueue(g_dev, g_qf, 0, &g_queue);

	g_bind = make_buf((VkDeviceSize)V * 4 * sizeof(float));
	g_w = make_buf((VkDeviceSize)V * J * sizeof(float));
	g_bone = make_buf((VkDeviceSize)J * 12 * sizeof(float));
	g_verts = make_buf((VkDeviceSize)V * 4 * sizeof(float));
	memcpy(g_w.map, weights_dense, (size_t)V * J * sizeof(float));  // constant weights
	vk_lbs_set_bind(bind_v3);

	FILE *f = fopen(spv_path, "rb");
	if (!f) {
		return 1;
	}
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	std::vector<uint32_t> code(sz / 4);
	if (fread(code.data(), 1, sz, f) != (size_t)sz) {
		fclose(f);
		return 1;
	}
	fclose(f);
	VkShaderModuleCreateInfo smi{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	smi.codeSize = sz;
	smi.pCode = code.data();
	VKOK(vkCreateShaderModule(g_dev, &smi, nullptr, &g_sm));

	VkDescriptorSetLayoutBinding lb[4];
	for (uint32_t i = 0; i < 4; i++) {
		lb[i] = {i, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
	}
	VkDescriptorSetLayoutCreateInfo dli{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	dli.bindingCount = 4;
	dli.pBindings = lb;
	VKOK(vkCreateDescriptorSetLayout(g_dev, &dli, nullptr, &g_dsl));
	VkPipelineLayoutCreateInfo pli{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	pli.setLayoutCount = 1;
	pli.pSetLayouts = &g_dsl;
	VKOK(vkCreatePipelineLayout(g_dev, &pli, nullptr, &g_pl));
	VkComputePipelineCreateInfo cpi{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
	cpi.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	cpi.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	cpi.stage.module = g_sm;
	cpi.stage.pName = "lbs";
	cpi.layout = g_pl;
	VKOK(vkCreateComputePipelines(g_dev, VK_NULL_HANDLE, 1, &cpi, nullptr, &g_pipe));

	VkDescriptorPoolSize ps{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4};
	VkDescriptorPoolCreateInfo dpi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	dpi.maxSets = 1;
	dpi.poolSizeCount = 1;
	dpi.pPoolSizes = &ps;
	VKOK(vkCreateDescriptorPool(g_dev, &dpi, nullptr, &g_dp));
	VkDescriptorSetAllocateInfo dai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	dai.descriptorPool = g_dp;
	dai.descriptorSetCount = 1;
	dai.pSetLayouts = &g_dsl;
	VKOK(vkAllocateDescriptorSets(g_dev, &dai, &g_ds));
	VkBuffer bufs[4] = {g_bind.buf, g_w.buf, g_bone.buf, g_verts.buf};
	VkDescriptorBufferInfo dbi[4];
	VkWriteDescriptorSet wr[4];
	for (uint32_t i = 0; i < 4; i++) {
		dbi[i] = {bufs[i], 0, VK_WHOLE_SIZE};
		wr[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		wr[i].dstSet = g_ds;
		wr[i].dstBinding = i;
		wr[i].descriptorCount = 1;
		wr[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		wr[i].pBufferInfo = &dbi[i];
	}
	vkUpdateDescriptorSets(g_dev, 4, wr, 0, nullptr);

	VkCommandPoolCreateInfo cpci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	cpci.queueFamilyIndex = g_qf;
	VKOK(vkCreateCommandPool(g_dev, &cpci, nullptr, &g_cmdPool));
	VkCommandBufferAllocateInfo cbi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	cbi.commandPool = g_cmdPool;
	cbi.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cbi.commandBufferCount = 1;
	VKOK(vkAllocateCommandBuffers(g_dev, &cbi, &g_cmd));
	// Record once; the buffers are fixed handles, so re-submit each frame.
	VkCommandBufferBeginInfo bbi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	vkBeginCommandBuffer(g_cmd, &bbi);
	vkCmdBindPipeline(g_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, g_pipe);
	vkCmdBindDescriptorSets(g_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, g_pl, 0, 1, &g_ds, 0, nullptr);
	vkCmdDispatch(g_cmd, (V + 63) / 64, 1, 1);
	vkEndCommandBuffer(g_cmd);

	g_ok = true;
	return 0;
}

void vk_lbs_dispatch(const float *bone_j4x4, float *out_v3) {
	if (!g_ok) {
		return;
	}
	float *bp = (float *)g_bone.map;  // pack each joint's [R|t] 3 rows (12 of the 16 floats)
	for (unsigned j = 0; j < g_J; j++) {
		memcpy(&bp[j * 12], &bone_j4x4[j * 16], 12 * sizeof(float));
	}
	VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
	si.commandBufferCount = 1;
	si.pCommandBuffers = &g_cmd;
	vkQueueSubmit(g_queue, 1, &si, VK_NULL_HANDLE);
	vkQueueWaitIdle(g_queue);
	const float *vp = (const float *)g_verts.map;
	for (unsigned v = 0; v < g_V; v++) {
		out_v3[v * 3 + 0] = vp[v * 4 + 0];
		out_v3[v * 3 + 1] = vp[v * 4 + 1];
		out_v3[v * 3 + 2] = vp[v * 4 + 2];
	}
}

void vk_lbs_shutdown(void) {
	if (!g_ok) {
		return;
	}
	vkDeviceWaitIdle(g_dev);
	vkDestroyCommandPool(g_dev, g_cmdPool, nullptr);
	vkDestroyDescriptorPool(g_dev, g_dp, nullptr);
	vkDestroyPipeline(g_dev, g_pipe, nullptr);
	vkDestroyPipelineLayout(g_dev, g_pl, nullptr);
	vkDestroyDescriptorSetLayout(g_dev, g_dsl, nullptr);
	vkDestroyShaderModule(g_dev, g_sm, nullptr);
	for (Buf *b : {&g_bind, &g_w, &g_bone, &g_verts}) {
		vkDestroyBuffer(g_dev, b->buf, nullptr);
		vkFreeMemory(g_dev, b->mem, nullptr);
	}
	vkDestroyDevice(g_dev, nullptr);
	vkDestroyInstance(g_inst, nullptr);
	g_ok = false;
}
