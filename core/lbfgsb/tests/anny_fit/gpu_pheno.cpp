// SPDX-License-Identifier: MIT
// See gpu_pheno.h. One translation unit with soma_pheno_slang.cpp, to read its asset pointers.
#include "../../../soma_pheno_slang.cpp"

#include <vulkan/vulkan.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "gpu_pheno.h"

namespace {
struct Buf {
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	void *mapped = nullptr;
	size_t bytes = 0;
};
struct Kernel {
	VkShaderModule module = VK_NULL_HANDLE;
	VkDescriptorSetLayout set_layout = VK_NULL_HANDLE;
	VkPipelineLayout layout = VK_NULL_HANDLE;
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkDescriptorSet set = VK_NULL_HANDLE;
	uint32_t threads = 0; // dispatch size in work items
};

VkInstance g_instance = VK_NULL_HANDLE;
VkPhysicalDevice g_phys = VK_NULL_HANDLE;
VkDevice g_device = VK_NULL_HANDLE;
VkQueue g_queue = VK_NULL_HANDLE;
VkCommandPool g_pool = VK_NULL_HANDLE;
VkCommandBuffer g_cmd = VK_NULL_HANDLE;
VkFence g_fence = VK_NULL_HANDLE;
VkDescriptorPool g_dpool = VK_NULL_HANDLE;
VkPhysicalDeviceMemoryProperties g_mem{};
Kernel g_k[5];
Buf g_coeffs, g_v0, g_outbw;

bool vk_ok(VkResult r, const char *what) {
	if (r != VK_SUCCESS) {
		std::fprintf(stderr, "gpu_pheno: %s failed (%d)\n", what, int(r));
		return false;
	}
	return true;
}

bool make_buffer(Buf &b, size_t bytes, const void *src) {
	b.bytes = bytes;
	VkBufferCreateInfo bi{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bi.size = bytes;
	bi.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (!vk_ok(vkCreateBuffer(g_device, &bi, nullptr, &b.buffer), "vkCreateBuffer")) {
		return false;
	}
	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(g_device, b.buffer, &req);
	const VkMemoryPropertyFlags want = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	uint32_t type = UINT32_MAX;
	for (uint32_t i = 0; i < g_mem.memoryTypeCount; ++i) {
		if ((req.memoryTypeBits & (1u << i)) && (g_mem.memoryTypes[i].propertyFlags & want) == want) {
			type = i;
			break;
		}
	}
	if (type == UINT32_MAX) {
		return false;
	}
	VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	ai.allocationSize = req.size;
	ai.memoryTypeIndex = type;
	if (!vk_ok(vkAllocateMemory(g_device, &ai, nullptr, &b.memory), "vkAllocateMemory") || !vk_ok(vkBindBufferMemory(g_device, b.buffer, b.memory, 0), "vkBindBufferMemory") ||
			!vk_ok(vkMapMemory(g_device, b.memory, 0, VK_WHOLE_SIZE, 0, &b.mapped), "vkMapMemory")) {
		return false;
	}
	if (src) {
		std::memcpy(b.mapped, src, bytes);
	} else {
		std::memset(b.mapped, 0, bytes);
	}
	return true;
}

bool make_kernel(Kernel &k, const std::string &path, const std::vector<const Buf *> &binds) {
	std::ifstream f(path, std::ios::binary);
	std::vector<char> code((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
	if (code.empty()) {
		std::fprintf(stderr, "gpu_pheno: cannot read %s\n", path.c_str());
		return false;
	}
	VkShaderModuleCreateInfo si{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	si.codeSize = code.size();
	si.pCode = reinterpret_cast<const uint32_t *>(code.data());
	std::vector<VkDescriptorSetLayoutBinding> lb(binds.size());
	for (uint32_t i = 0; i < binds.size(); ++i) {
		lb[i] = { i, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
	}
	VkDescriptorSetLayoutCreateInfo li{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	li.bindingCount = uint32_t(lb.size());
	li.pBindings = lb.data();
	VkPipelineLayoutCreateInfo pi{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	if (!vk_ok(vkCreateShaderModule(g_device, &si, nullptr, &k.module), "vkCreateShaderModule") || !vk_ok(vkCreateDescriptorSetLayout(g_device, &li, nullptr, &k.set_layout), "vkCreateDescriptorSetLayout")) {
		return false;
	}
	pi.setLayoutCount = 1;
	pi.pSetLayouts = &k.set_layout;
	VkComputePipelineCreateInfo ci{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	ci.stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_COMPUTE_BIT, k.module, "main", nullptr };
	if (!vk_ok(vkCreatePipelineLayout(g_device, &pi, nullptr, &k.layout), "vkCreatePipelineLayout")) {
		return false;
	}
	ci.layout = k.layout;
	if (!vk_ok(vkCreateComputePipelines(g_device, VK_NULL_HANDLE, 1, &ci, nullptr, &k.pipeline), "vkCreateComputePipelines")) {
		return false;
	}
	VkDescriptorSetAllocateInfo ai{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	ai.descriptorPool = g_dpool;
	ai.descriptorSetCount = 1;
	ai.pSetLayouts = &k.set_layout;
	if (!vk_ok(vkAllocateDescriptorSets(g_device, &ai, &k.set), "vkAllocateDescriptorSets")) {
		return false;
	}
	std::vector<VkDescriptorBufferInfo> info(binds.size());
	std::vector<VkWriteDescriptorSet> w(binds.size());
	for (uint32_t i = 0; i < binds.size(); ++i) {
		info[i] = { binds[i]->buffer, 0, VK_WHOLE_SIZE };
		w[i] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, k.set, i, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &info[i], nullptr };
	}
	vkUpdateDescriptorSets(g_device, uint32_t(w.size()), w.data(), 0, nullptr);
	return true;
}
} // namespace

bool gpu_pheno_init(const char *spv_dir) {
	const char *inst_ext[] = { VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME };
	VkApplicationInfo app{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	app.apiVersion = VK_API_VERSION_1_2;
	VkInstanceCreateInfo ii{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	ii.pApplicationInfo = &app;
	ii.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	ii.enabledExtensionCount = 2;
	ii.ppEnabledExtensionNames = inst_ext;
	if (vkCreateInstance(&ii, nullptr, &g_instance) != VK_SUCCESS) {
		return false;
	}
	uint32_t n = 0;
	vkEnumeratePhysicalDevices(g_instance, &n, nullptr);
	if (n == 0) {
		return false;
	}
	std::vector<VkPhysicalDevice> devs(n);
	vkEnumeratePhysicalDevices(g_instance, &n, devs.data());
	g_phys = devs[0];
	vkGetPhysicalDeviceMemoryProperties(g_phys, &g_mem);
	uint32_t nq = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(g_phys, &nq, nullptr);
	std::vector<VkQueueFamilyProperties> qp(nq);
	vkGetPhysicalDeviceQueueFamilyProperties(g_phys, &nq, qp.data());
	uint32_t qf = UINT32_MAX;
	for (uint32_t i = 0; i < nq; ++i) {
		if (qp[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
			qf = i;
			break;
		}
	}
	if (qf == UINT32_MAX) {
		return false;
	}
	uint32_t ne = 0;
	vkEnumerateDeviceExtensionProperties(g_phys, nullptr, &ne, nullptr);
	std::vector<VkExtensionProperties> ep(ne);
	vkEnumerateDeviceExtensionProperties(g_phys, nullptr, &ne, ep.data());
	std::vector<const char *> dev_ext;
	for (const VkExtensionProperties &e : ep) {
		if (std::string(e.extensionName) == "VK_KHR_portability_subset") {
			dev_ext.push_back("VK_KHR_portability_subset");
		}
	}
	float prio = 1.0f;
	VkDeviceQueueCreateInfo qi{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	qi.queueFamilyIndex = qf;
	qi.queueCount = 1;
	qi.pQueuePriorities = &prio;
	VkPhysicalDeviceVulkan12Features f12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	f12.scalarBlockLayout = VK_TRUE; // the kernels are compiled with -force-glsl-scalar-layout: float3 is 12 bytes, as on the CPU
	VkDeviceCreateInfo di{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	di.pNext = &f12;
	di.queueCreateInfoCount = 1;
	di.pQueueCreateInfos = &qi;
	di.enabledExtensionCount = uint32_t(dev_ext.size());
	di.ppEnabledExtensionNames = dev_ext.data();
	if (!vk_ok(vkCreateDevice(g_phys, &di, nullptr, &g_device), "vkCreateDevice")) {
		return false;
	}
	vkGetDeviceQueue(g_device, qf, 0, &g_queue);
	VkCommandPoolCreateInfo pi{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	pi.queueFamilyIndex = qf;
	pi.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VkDescriptorPoolSize ps{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64 };
	VkDescriptorPoolCreateInfo dp{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	dp.maxSets = 8;
	dp.poolSizeCount = 1;
	dp.pPoolSizes = &ps;
	VkFenceCreateInfo fi{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	if (!vk_ok(vkCreateCommandPool(g_device, &pi, nullptr, &g_pool), "vkCreateCommandPool") || !vk_ok(vkCreateDescriptorPool(g_device, &dp, nullptr, &g_dpool), "vkCreateDescriptorPool") ||
			!vk_ok(vkCreateFence(g_device, &fi, nullptr, &g_fence), "vkCreateFence")) {
		return false;
	}
	VkCommandBufferAllocateInfo ci{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	ci.commandPool = g_pool;
	ci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	ci.commandBufferCount = 1;
	if (!vk_ok(vkAllocateCommandBuffers(g_device, &ci, &g_cmd), "vkAllocateCommandBuffers")) {
		return false;
	}
	// Buffers: the asset's bytes, and the intermediates.
	static Buf templ, blend, vanny, fsrc, p3, ftet, fids, bary, crow, col, val, jpos, bsh, bw, sw, par;
	const size_t P = PH_P, V = PH_V, J = PH_J, NFS = PH_NFS, NNZ = PH_NNZ;
	bool ok = make_buffer(templ, P * 12, g_templ) && make_buffer(blend, size_t(PH_NB) * P * 12, g_blend) && make_buffer(g_coeffs, size_t(PH_NB) * 4, nullptr) && make_buffer(vanny, P * 12, nullptr) &&
			make_buffer(fsrc, NFS * 12, g_F_src) && make_buffer(p3, NFS * 12, nullptr) && make_buffer(ftet, NFS * 16, g_F_tet) && make_buffer(fids, V * 4, g_fids) && make_buffer(bary, V * 16, g_bary) &&
			make_buffer(g_v0, V * 12, nullptr) && make_buffer(crow, (J + 1) * 4, g_rbf_crow) && make_buffer(col, NNZ * 4, g_rbf_col) && make_buffer(val, NNZ * 4, g_rbf_val) && make_buffer(jpos, J * 12, nullptr) &&
			make_buffer(bsh, V * 12, g_BSh) && make_buffer(bw, J * 64, g_BW) && make_buffer(sw, V * J * 4, g_SW) && make_buffer(par, J * 4, g_par) && make_buffer(g_outbw, J * 64, nullptr);
	if (!ok) {
		std::fprintf(stderr, "gpu_pheno: buffers\n");
		return false;
	}
	const std::string d = spv_dir;
	const struct {
		const char *name;
		uint32_t threads;
		std::vector<const Buf *> binds;
	} plan[5] = {
		{ "pheno_blendshape", uint32_t(P), { &templ, &blend, &g_coeffs, &vanny } },
		{ "pheno_bary_tet", uint32_t(NFS), { &vanny, &fsrc, &p3 } },
		{ "pheno_bary_gather", uint32_t(V), { &vanny, &p3, &ftet, &fids, &bary, &g_v0 } },
		{ "pheno_rbf", uint32_t(J), { &g_v0, &crow, &col, &val, &jpos } },
		{ "pheno_skeleton_fit", 1, { &g_v0, &bsh, &bw, &jpos, &sw, &par, &g_outbw } },
	};
	for (int i = 0; i < 5; ++i) {
		g_k[i].threads = plan[i].threads;
		if (!make_kernel(g_k[i], d + "/" + plan[i].name + ".spv", plan[i].binds)) {
			return false;
		}
	}
	return true;
}

void gpu_pheno_eval_local(const float ident[11], const float *local, float *v0, float *bw) {
	pheno_weights(ident, local, static_cast<float *>(g_coeffs.mapped));
	VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vkResetCommandBuffer(g_cmd, 0);
	vkBeginCommandBuffer(g_cmd, &bi);
	for (int i = 0; i < 5; ++i) {
		vkCmdBindPipeline(g_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, g_k[i].pipeline);
		vkCmdBindDescriptorSets(g_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, g_k[i].layout, 0, 1, &g_k[i].set, 0, nullptr);
		vkCmdDispatch(g_cmd, (g_k[i].threads + 63) / 64, 1, 1);
		VkMemoryBarrier mb{ VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_HOST_READ_BIT };
		vkCmdPipelineBarrier(g_cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
	}
	vkEndCommandBuffer(g_cmd);
	VkSubmitInfo si{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	si.commandBufferCount = 1;
	si.pCommandBuffers = &g_cmd;
	vkResetFences(g_device, 1, &g_fence);
	vkQueueSubmit(g_queue, 1, &si, g_fence);
	vkWaitForFences(g_device, 1, &g_fence, VK_TRUE, UINT64_MAX);
	std::memcpy(v0, g_v0.mapped, size_t(PH_V) * 12);
	std::memcpy(bw, g_outbw.mapped, size_t(PH_J) * 64);
}

void gpu_pheno_eval(const float ident[11], float *v0, float *bw) {
	gpu_pheno_eval_local(ident, nullptr, v0, bw);
}

void gpu_pheno_shutdown() {
	if (g_device) {
		vkDeviceWaitIdle(g_device);
	}
}
