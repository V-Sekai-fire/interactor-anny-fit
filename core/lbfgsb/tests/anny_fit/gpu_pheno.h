// SPDX-License-Identifier: MIT
// The phenotype body on the GPU: the same five Slang kernels as soma_pheno_slang.cpp, as SPIR-V through the Vulkan loader
// (MoltenVK on macOS). Buffers are the bytes of soma_pheno.bin, so the CPU and GPU paths read the same data.
#pragma once

// spv_dir holds pheno_{blendshape,bary_tet,bary_gather,rbf,skeleton_fit}.spv. False when there is no device or a kernel does not load.
bool gpu_pheno_init(const char *spv_dir);
// Same arguments and outputs as pheno_eval. pheno_load must have run.
void gpu_pheno_eval(const float ident[11], float *v0, float *bw);
// With the local-change sliders (PH_NLOC values, null for none).
void gpu_pheno_eval_local(const float ident[11], const float *local, float *v0, float *bw);
void gpu_pheno_shutdown();
