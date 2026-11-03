// SPDX-License-Identifier: MIT
// Native SOMA phenotype→shape: recompute the ANNY bind mesh + bind skeleton from
// the 11-dim phenotype, with no Python and no ONNX runtime.  Faithful port of
// soma's identity path (blendshapes + barycentric transfer) and skeleton_transfer
// (RBF joint regression + align_vectors).  Constants load from soma_pheno.bin.
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

// Load soma_pheno.bin (returns 0 on success, nonzero on failure).
int pheno_load(const char *bin_path);

// ident: 11 phenotype params in order
//   [gender, age, muscle, weight, height, proportions, cupsize, firmness,
//    african, asian, caucasian].
// v0_out:        PH_V*3 floats — the new bind/rest mesh (SOMA frame, metres).
// bindworld_out: PH_J*16 floats — new bind world transforms, 4x4 row-major.
void pheno_eval(const float ident[11], float *v0_out, float *bindworld_out);
// With the local-change sliders (PH_NLOC values in [-1, 1], ANNY local_change_labels order); local may be null.
void pheno_eval_local(const float ident[11], const float *local, float *v0_out, float *bindworld_out);

#ifdef __cplusplus
}
#endif
