// SPDX-License-Identifier: MIT
// The 1132 blendshape weights of an 11-value phenotype and the local-change sliders, from the baked asset's mask: the same arithmetic as
// sinew-mocap/solve core/soma_pheno_slang.cpp (pheno_coefficients, pheno_weights), which the host fit and the golden check use.
#pragma once
#include <cstddef>
#include <cstdint>

// mask: PH_OFF_mask of the baked soma_pheno.bin (PH_NPH x PH_NVAR floats). wi: PH_NB floats.
void anny_pheno_weights(const float *mask, const float ident[11], const float *local, float *wi);
