// SPDX-License-Identifier: MIT
// Copyright (c) 2026-present K. S. Ernest (iFire) Lee
//
// Headless Vulkan compute host for the Lean-emitted LBS SPIR-V kernel.  Binds the
// four storage buffers (bind / weights / bone / verts), dispatches lbs on the GPU,
// reads back, and checks the skinning against the same hand-computed vertices the
// CPU test uses — so the Slang kernel is verified on a real GPU (no window).
//
// std430 note: StructuredBuffer<float3> has 16-byte stride (vec3 padded to vec4),
// so bind/verts use 4 floats per vertex; weights (float) and bone (float4) pack.
//
// Build + run: spec/tests/lbs_vulkan/run.sh (passes -DVK_NO_PROTOTYPES)
#include "volk.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#define VKC(x)                                                             \
	do {                                                                   \
		VkResult _r = (x);                                                 \
		if (_r != VK_SUCCESS) {                                            \
			printf("VK error %d at %s:%d\n", (int)_r, __FILE__, __LINE__); \
			return 2;                                                      \
		}                                                                  \
	} while (0)

static VkPhysicalDevice g_pd;
static VkDevice g_dev;

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

struct Buf {
	VkBuffer buf;
	VkDeviceMemory mem;
};

// Deterministic LCG PRNG (was a capturing lambda; now a freestanding function).
static float rnd(uint32_t &s) {
	s = s * 1664525u + 1013904223u;
	return (float)((s >> 8) & 0xFFFFu) / 65535.0f;
}

static Buf make_buf(VkDeviceSize size, const void *data) {
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
	if (data) {
		void *p = nullptr;
		vkMapMemory(g_dev, b.mem, 0, size, 0, &p);
		memcpy(p, data, size);
		vkUnmapMemory(g_dev, b.mem);
	}
	return b;
}

int main(int argc, char **argv) {
	const char *spv_path = argc > 1 ? argv[1] : "lbs.spv";

	if (volkInitialize() != VK_SUCCESS) {
		printf("volkInitialize failed (no vulkan-1 loader?)\n");
		return 2;
	}
	VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	app.apiVersion = VK_API_VERSION_1_1;
	VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	ici.pApplicationInfo = &app;
	// MoltenVK (macOS) is a portability driver: enable enumeration when present,
	// else vkCreateInstance returns VK_ERROR_INCOMPATIBLE_DRIVER (-9).
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
	VkInstance inst;
	VKC(vkCreateInstance(&ici, nullptr, &inst));
	volkLoadInstance(inst);

	uint32_t nd = 0;
	vkEnumeratePhysicalDevices(inst, &nd, nullptr);
	std::vector<VkPhysicalDevice> pds(nd);
	vkEnumeratePhysicalDevices(inst, &nd, pds.data());
	g_pd = VK_NULL_HANDLE;
	uint32_t qf = 0;
	for (VkPhysicalDevice cand : pds) {
		uint32_t nq = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(cand, &nq, nullptr);
		std::vector<VkQueueFamilyProperties> qs(nq);
		vkGetPhysicalDeviceQueueFamilyProperties(cand, &nq, qs.data());
		for (uint32_t i = 0; i < nq; i++) {
			if (qs[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
				g_pd = cand;
				qf = i;
				break;
			}
		}
		if (g_pd) {
			break;
		}
	}
	if (!g_pd) {
		printf("no Vulkan compute device\n");
		return 2;
	}
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(g_pd, &props);
	printf("GPU: %s\n", props.deviceName);

	float prio = 1.0f;
	VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	qci.queueFamilyIndex = qf;
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
	VKC(vkCreateDevice(g_pd, &dci, nullptr, &g_dev));
	volkLoadDevice(g_dev);
	VkQueue queue;
	vkGetDeviceQueue(g_dev, qf, 0, &queue);

	// ── Data ──────────────────────────────────────────────────────────────────
	// Default: the 4-vertex hand-checked case (matches the CPU test).  With a
	// second arg "scale", run ANNY's real dimensions (V=18056, J=77) with
	// deterministic pseudo-random data and compare the GPU result to a CPU
	// reference.  bind / verts use 4 floats per vertex (vec3 padded to 16 B std430).
	bool scale = (argc > 2 && strcmp(argv[2], "scale") == 0);
	uint32_t V = 4, J = 2;
	std::vector<float> bindP, w, bone;
	if (scale) {
		V = 18056;
		J = 77;
		bindP.assign(V * 4, 0.0f);
		w.assign((size_t)V * J, 0.0f);
		bone.assign((size_t)J * 3 * 4, 0.0f);
		uint32_t s = 12345u;
		for (uint32_t i = 0; i < V; i++) {
			for (int k = 0; k < 3; k++) {
				bindP[i * 4 + k] = rnd(s) * 2.0f - 1.0f;
			}
		}
		for (uint32_t i = 0; i < V; i++) {  // 4 random influences per vertex, normalized
			float sum = 0;
			for (int k = 0; k < 4; k++) {
				uint32_t j = (uint32_t)(rnd(s) * J) % J;
				float wv = rnd(s);
				w[(size_t)i * J + j] += wv;
				sum += wv;
			}
			if (sum > 0) {
				for (uint32_t j = 0; j < J; j++) {
					w[(size_t)i * J + j] /= sum;
				}
			}
		}
		for (size_t k = 0; k < bone.size(); k++) {
			bone[k] = rnd(s) * 2.0f - 1.0f;
		}
	} else {
		bindP = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 2, 3, 4, 0};
		w = {1, 0, 0, 1, 0.5f, 0.5f, 1, 0};
		bone = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 10, 0, 1, 0, 20, 0, 0, 1, 30};
	}
	std::vector<float> vertsP((size_t)V * 4, 0.0f);

	Buf b_bind = make_buf(bindP.size() * sizeof(float), bindP.data());
	Buf b_w = make_buf(w.size() * sizeof(float), w.data());
	Buf b_bone = make_buf(bone.size() * sizeof(float), bone.data());
	Buf b_verts = make_buf(vertsP.size() * sizeof(float), vertsP.data());

	// ── Shader module from the Lean-emitted SPIR-V ───────────────────────────
	FILE *f = fopen(spv_path, "rb");
	if (!f) {
		printf("cannot open %s\n", spv_path);
		return 2;
	}
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	std::vector<uint32_t> code(sz / 4);
	if (fread(code.data(), 1, sz, f) != (size_t)sz) {
		printf("read %s failed\n", spv_path);
		return 2;
	}
	fclose(f);
	VkShaderModuleCreateInfo smi{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	smi.codeSize = sz;
	smi.pCode = code.data();
	VkShaderModule sm;
	VKC(vkCreateShaderModule(g_dev, &smi, nullptr, &sm));

	// ── Descriptor + pipeline (4 storage buffers, set 0 bindings 0..3) ────────
	VkDescriptorSetLayoutBinding lb[4];
	for (uint32_t i = 0; i < 4; i++) {
		lb[i] = {i, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
	}
	VkDescriptorSetLayoutCreateInfo dli{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	dli.bindingCount = 4;
	dli.pBindings = lb;
	VkDescriptorSetLayout dsl;
	VKC(vkCreateDescriptorSetLayout(g_dev, &dli, nullptr, &dsl));

	VkPipelineLayoutCreateInfo pli{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	pli.setLayoutCount = 1;
	pli.pSetLayouts = &dsl;
	VkPipelineLayout pl;
	VKC(vkCreatePipelineLayout(g_dev, &pli, nullptr, &pl));

	VkComputePipelineCreateInfo cpi{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
	cpi.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	cpi.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	cpi.stage.module = sm;
	cpi.stage.pName = "lbs";
	cpi.layout = pl;
	VkPipeline pipe;
	VKC(vkCreateComputePipelines(g_dev, VK_NULL_HANDLE, 1, &cpi, nullptr, &pipe));

	VkDescriptorPoolSize ps{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4};
	VkDescriptorPoolCreateInfo dpi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	dpi.maxSets = 1;
	dpi.poolSizeCount = 1;
	dpi.pPoolSizes = &ps;
	VkDescriptorPool dp;
	VKC(vkCreateDescriptorPool(g_dev, &dpi, nullptr, &dp));
	VkDescriptorSetAllocateInfo dai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	dai.descriptorPool = dp;
	dai.descriptorSetCount = 1;
	dai.pSetLayouts = &dsl;
	VkDescriptorSet ds;
	VKC(vkAllocateDescriptorSets(g_dev, &dai, &ds));

	VkBuffer bufs[4] = {b_bind.buf, b_w.buf, b_bone.buf, b_verts.buf};
	VkDescriptorBufferInfo dbi[4];
	VkWriteDescriptorSet wr[4];
	for (uint32_t i = 0; i < 4; i++) {
		dbi[i] = {bufs[i], 0, VK_WHOLE_SIZE};
		wr[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		wr[i].dstSet = ds;
		wr[i].dstBinding = i;
		wr[i].descriptorCount = 1;
		wr[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		wr[i].pBufferInfo = &dbi[i];
	}
	vkUpdateDescriptorSets(g_dev, 4, wr, 0, nullptr);

	// ── Dispatch ──────────────────────────────────────────────────────────────
	VkCommandPoolCreateInfo cpci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	cpci.queueFamilyIndex = qf;
	VkCommandPool cmdPool;
	VKC(vkCreateCommandPool(g_dev, &cpci, nullptr, &cmdPool));
	VkCommandBufferAllocateInfo cbi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	cbi.commandPool = cmdPool;
	cbi.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cbi.commandBufferCount = 1;
	VkCommandBuffer cmd;
	VKC(vkAllocateCommandBuffers(g_dev, &cbi, &cmd));
	VkCommandBufferBeginInfo bbi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	bbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &bbi);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipe);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pl, 0, 1, &ds, 0, nullptr);
	vkCmdDispatch(cmd, (V + 63) / 64, 1, 1);
	vkEndCommandBuffer(cmd);
	VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
	si.commandBufferCount = 1;
	si.pCommandBuffers = &cmd;
	VKC(vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE));
	VKC(vkQueueWaitIdle(queue));

	// ── Read back + verify ────────────────────────────────────────────────────
	void *p = nullptr;
	vkMapMemory(g_dev, b_verts.mem, 0, vertsP.size() * sizeof(float), 0, &p);
	memcpy(vertsP.data(), p, vertsP.size() * sizeof(float));
	vkUnmapMemory(g_dev, b_verts.mem);

	double e = 0;
	if (scale) {
		// CPU reference (same float ops, same j-order as the kernel).
		for (uint32_t v = 0; v < V; v++) {
			float bx = bindP[v * 4], by = bindP[v * 4 + 1], bz = bindP[v * 4 + 2];
			float ax = 0, ay = 0, az = 0;
			for (uint32_t j = 0; j < J; j++) {
				float wv = w[(size_t)v * J + j];
				if (wv == 0) {
					continue;
				}
				const float *r = &bone[(size_t)j * 12];
				ax += wv * (r[0] * bx + r[1] * by + r[2] * bz + r[3]);
				ay += wv * (r[4] * bx + r[5] * by + r[6] * bz + r[7]);
				az += wv * (r[8] * bx + r[9] * by + r[10] * bz + r[11]);
			}
			e = std::max(e, (double)std::fabs(vertsP[v * 4] - ax));
			e = std::max(e, (double)std::fabs(vertsP[v * 4 + 1] - ay));
			e = std::max(e, (double)std::fabs(vertsP[v * 4 + 2] - az));
		}
		printf("scale V=%u J=%u  GPU vs CPU max|err| = %.3e  %s\n", V, J, e,
		       e < 1e-3 ? "OK" : "FAIL");
		return e < 1e-3 ? 0 : 1;
	}
	float exp[12] = {1, 0, 0, 10, 21, 30, 5, 10, 16.0f, 2, 3, 4};
	for (uint32_t i = 0; i < V; i++) {
		float x = vertsP[i * 4], y = vertsP[i * 4 + 1], z = vertsP[i * 4 + 2];
		e = std::max(e, (double)std::fabs(x - exp[i * 3]));
		e = std::max(e, (double)std::fabs(y - exp[i * 3 + 1]));
		e = std::max(e, (double)std::fabs(z - exp[i * 3 + 2]));
		printf("v%u = (%.3f, %.3f, %.3f)  exp (%.3f, %.3f, %.3f)\n", i, x, y, z, exp[i * 3],
		       exp[i * 3 + 1], exp[i * 3 + 2]);
	}
	printf("max|err| = %.3e  %s\n", e, e < 1e-5 ? "OK" : "FAIL");
	return e < 1e-5 ? 0 : 1;
}
