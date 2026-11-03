// SPDX-License-Identifier: MIT
// Per-frame Vulkan compute host for the Lean-emitted LBS SPIR-V kernel
// (spec/Sinew/SlangCodegen/Lbs.lean → lbs.spv).  The viewer runs the per-frame
// deform on the GPU: upload the per-joint bone affines, dispatch lbs, read the
// skinned vertices back for polyscope.  If no Vulkan device is present
// (vk_lbs_init returns nonzero) the caller falls back to the CPU path.
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

// Bring up Vulkan, upload constant weights + the initial bind mesh, build the
// pipeline.  Returns 0 on success (GPU path active), nonzero if unavailable.
//   bind_v3:       V*3 rest vertices (tight)
//   weights_dense: V*J row-major skinning weights
int vk_lbs_init(const char *spv_path, const float *bind_v3, const float *weights_dense, unsigned V,
                unsigned J);

// Re-upload the bind mesh (call when phenotype reshapes the body).
void vk_lbs_set_bind(const float *bind_v3);

// Dispatch one frame: bone_j4x4 is J*16 (4x4 row-major per joint; the [R|t] rows
// are taken); out_v3 receives V*3 skinned vertices.
void vk_lbs_dispatch(const float *bone_j4x4, float *out_v3);

void vk_lbs_shutdown(void);

#ifdef __cplusplus
}
#endif
