// SPDX-License-Identifier: MIT
// Copyright (c) 2026-present K. S. Ernest (iFire) Lee
//
// CPU host harness for the Lean-emitted phenotype kernels.  Runs slangc's
// -target cpp output of the four Sinew.SlangCodegen.Pheno kernels over the baked
// SOMA assets (soma_pheno.bin) and checks the resulting bind mesh v0 against the
// SOMA golden — proof that the Lean → Slang phenotype deform is numerically
// correct on the CPU path.  The sequential align_vectors skeleton fit is not a
// kernel and is out of scope here.
//
// Each kernel's generated C++ goes in its own namespace (same GlobalParams_0 name,
// different layouts); the slang prelude is force-included once globally (run.sh's
// -include) so the per-namespace re-includes are header-guarded out.
//
// Build + run: spec/tests/pheno_cpu/run.sh
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "soma_pheno.h"  // PH_* dims + PH_OFF_* byte offsets (from viz_native, -I)

namespace kblend {
#include "pheno_blendshape.gen.cpp"
}
namespace ktet {
#include "pheno_bary_tet.gen.cpp"
}
namespace kgather {
#include "pheno_bary_gather.gen.cpp"
}
namespace krbf {
#include "pheno_rbf.gen.cpp"
}
namespace kskel {
#include "pheno_skeleton_fit.gen.cpp"
}

// One workgroup range covering n threads (numthreads = 64), templated on each
// namespace's own ComputeVaryingInput type.
template <class CVI>
static CVI dispatch(uint32_t n) {
	CVI vi{};
	vi.startGroupID.x = vi.startGroupID.y = vi.startGroupID.z = 0;
	vi.endGroupID.x = (n + 63) / 64;
	vi.endGroupID.y = 1;
	vi.endGroupID.z = 1;
	return vi;
}

static const float ANC_age[5] = {-1.f / 3.f, 0.f, 1.f / 3.f, 2.f / 3.f, 1.f};
static const float ANC_2[2] = {0.f, 1.f};
static const float ANC_3[3] = {0.f, 0.5f, 1.f};

static void interp(float v, const float *a, int n, float *w) {
	for (int i = 0; i < n; i++) {
		w[i] = 0;
	}
	int idx = 0;
	while (idx < n && a[idx] < v) {
		idx++;
	}
	if (idx < 1) {
		idx = 1;
	}
	if (idx > n - 1) {
		idx = n - 1;
	}
	float al = (v - a[idx - 1]) / (a[idx] - a[idx - 1]);
	if (al < 0) {
		al = 0;
	}
	if (al > 1) {
		al = 1;
	}
	w[idx - 1] = 1 - al;
	w[idx] = al;
}

int main(int argc, char **argv) {
	const char *binp = argc > 1 ? argv[1] : "soma_pheno.bin";
	const char *goldp = argc > 2 ? argv[2] : "soma_pheno_golden.bin";
	FILE *f = fopen(binp, "rb");
	if (!f) {
		printf("missing %s\n", binp);
		return 1;
	}
	uint8_t *blob = (uint8_t *)malloc(PH_BIN_BYTES);
	if (fread(blob, 1, PH_BIN_BYTES, f) != (size_t)PH_BIN_BYTES) {
		printf("short read %s\n", binp);
		return 1;
	}
	fclose(f);
	const float *templ = (const float *)(blob + PH_OFF_template);
	const float *blend = (const float *)(blob + PH_OFF_blend);
	const float *mask = (const float *)(blob + PH_OFF_mask);
	const int32_t *Fsrc = (const int32_t *)(blob + PH_OFF_F_src);
	const int32_t *Ftet = (const int32_t *)(blob + PH_OFF_F_tet);
	const float *bary = (const float *)(blob + PH_OFF_bary);
	const int32_t *fids = (const int32_t *)(blob + PH_OFF_fids);
	const int32_t *crow = (const int32_t *)(blob + PH_OFF_rbf_crow);
	const int32_t *col = (const int32_t *)(blob + PH_OFF_rbf_col);
	const float *val = (const float *)(blob + PH_OFF_rbf_val);
	const float *BSh = (const float *)(blob + PH_OFF_BSh);  // base bind shape
	const float *BW = (const float *)(blob + PH_OFF_BW);    // base bind world
	const float *SW = (const float *)(blob + PH_OFF_SW);    // skinning weights
	const int32_t *par = (const int32_t *)(blob + PH_OFF_par);

	// golden: ident(11) + v0(PH_V*3) + bind_world(PH_J*16); we use ident + v0.
	float ident[11];
	float *v0gold = (float *)malloc(sizeof(float) * PH_V * 3);
	FILE *g = fopen(goldp, "rb");
	if (!g) {
		printf("missing %s\n", goldp);
		return 1;
	}
	float *bwgold = (float *)malloc(sizeof(float) * PH_J * 16);
	size_t rd = fread(ident, sizeof(float), 11, g);
	rd += fread(v0gold, sizeof(float), PH_V * 3, g);
	rd += fread(bwgold, sizeof(float), PH_J * 16, g);
	fclose(g);

	// coeffs (host) — same per-feature interp + multilinear mask product as the C ref
	float wg[2], wa[5], wm[3], ww[3], wh[2], wp[2], wc[3], wfm[3];
	interp(ident[0], ANC_2, 2, wg);
	interp(ident[1], ANC_age, 5, wa);
	interp(ident[2], ANC_3, 3, wm);
	interp(ident[3], ANC_3, 3, ww);
	interp(ident[4], ANC_2, 2, wh);
	interp(ident[5], ANC_2, 2, wp);
	interp(ident[6], ANC_3, 3, wc);
	interp(ident[7], ANC_3, 3, wfm);
	float race[3] = {ident[8], ident[9], ident[10]};
	float rs = race[0] + race[1] + race[2];
	if (rs > 0) {
		for (int i = 0; i < 3; i++) {
			race[i] /= rs;
		}
	} else {
		race[0] = race[1] = race[2] = 1.f / 3.f;
	}
	float phens[26];
	int o = 0;
	for (int i = 0; i < 3; i++) {
		phens[o++] = race[i];
	}
	for (int i = 0; i < 2; i++) {
		phens[o++] = wg[i];
	}
	for (int i = 0; i < 5; i++) {
		phens[o++] = wa[i];
	}
	for (int i = 0; i < 3; i++) {
		phens[o++] = wm[i];
	}
	for (int i = 0; i < 3; i++) {
		phens[o++] = ww[i];
	}
	for (int i = 0; i < 2; i++) {
		phens[o++] = wh[i];
	}
	for (int i = 0; i < 2; i++) {
		phens[o++] = wp[i];
	}
	for (int i = 0; i < 3; i++) {
		phens[o++] = wc[i];
	}
	for (int i = 0; i < 3; i++) {
		phens[o++] = wfm[i];
	}
	float *wi = (float *)malloc(sizeof(float) * PH_NB);
	for (int k = 0; k < PH_NB; k++) {
		float pr = 1.f;
		const float *mr = mask + k * PH_NVAR;
		for (int j = 0; j < PH_NVAR; j++) {
			pr *= mr[j] != 0.f ? phens[j] * mr[j] + (1.f - mr[j]) : 1.f;
		}
		wi[k] = pr;
	}

	float *vanny = (float *)malloc(sizeof(float) * PH_P * 3);
	float *p3 = (float *)malloc(sizeof(float) * PH_NFS * 3);
	float *v0 = (float *)malloc(sizeof(float) * PH_V * 3);
	float *jpos = (float *)malloc(sizeof(float) * PH_J * 3);
	float *outbw = (float *)malloc(sizeof(float) * PH_J * 16);

	{  // blendshape: v_anny = templ + Σ_c wi[c]·blend[c]
		using namespace kblend;
		GlobalParams_0 gp{};
		gp.templ_0.data = (Vector<float, 3> *)templ;
		gp.templ_0.count = PH_P;
		gp.blend_0.data = (Vector<float, 3> *)blend;
		gp.blend_0.count = (size_t)PH_NB * PH_P;
		gp.coeffs_0.data = wi;
		gp.coeffs_0.count = PH_NB;
		gp.vanny_0.data = (Vector<float, 3> *)vanny;
		gp.vanny_0.count = PH_P;
		ComputeVaryingInput vi = dispatch<ComputeVaryingInput>(PH_P);
		blendshape(&vi, nullptr, &gp);
	}
	{  // bary_tet: fabricate the normal-tip vertex per source triangle
		using namespace ktet;
		GlobalParams_0 gp{};
		gp.vanny_0.data = (Vector<float, 3> *)vanny;
		gp.vanny_0.count = PH_P;
		gp.Fsrc_0.data = (uint32_t *)Fsrc;
		gp.Fsrc_0.count = (size_t)PH_NFS * 3;
		gp.p3_0.data = (Vector<float, 3> *)p3;
		gp.p3_0.count = PH_NFS;
		ComputeVaryingInput vi = dispatch<ComputeVaryingInput>(PH_NFS);
		bary_tet(&vi, nullptr, &gp);
	}
	{  // bary_gather: barycentric transfer Anny→SOMA + axis remap → v0
		using namespace kgather;
		GlobalParams_0 gp{};
		gp.vanny_0.data = (Vector<float, 3> *)vanny;
		gp.vanny_0.count = PH_P;
		gp.p3_0.data = (Vector<float, 3> *)p3;
		gp.p3_0.count = PH_NFS;
		gp.Ftet_0.data = (uint32_t *)Ftet;
		gp.Ftet_0.count = (size_t)PH_NFS * 4;
		gp.fids_0.data = (uint32_t *)fids;
		gp.fids_0.count = PH_V;
		gp.bary_0.data = (float *)bary;
		gp.bary_0.count = (size_t)PH_V * 4;
		gp.v0_0.data = (Vector<float, 3> *)v0;
		gp.v0_0.count = PH_V;
		ComputeVaryingInput vi = dispatch<ComputeVaryingInput>(PH_V);
		bary_gather(&vi, nullptr, &gp);
	}
	{  // rbf: sparse joint regression Jpos = M · v0
		using namespace krbf;
		GlobalParams_0 gp{};
		gp.v0_0.data = (Vector<float, 3> *)v0;
		gp.v0_0.count = PH_V;
		gp.crow_0.data = (uint32_t *)crow;
		gp.crow_0.count = PH_J + 1;
		gp.col_0.data = (uint32_t *)col;
		gp.col_0.count = PH_NNZ;
		gp.val_0.data = (float *)val;
		gp.val_0.count = PH_NNZ;
		gp.jpos_0.data = (Vector<float, 3> *)jpos;
		gp.jpos_0.count = PH_J;
		ComputeVaryingInput vi = dispatch<ComputeVaryingInput>(PH_J);
		rbf(&vi, nullptr, &gp);
	}
	{  // skeleton_fit: sequential align_vectors → bind world (single thread)
		using namespace kskel;
		GlobalParams_0 gp{};
		gp.v0_0.data = v0;
		gp.v0_0.count = (size_t)PH_V * 3;
		gp.bsh_0.data = (float *)BSh;
		gp.bsh_0.count = (size_t)PH_V * 3;
		gp.bw_0.data = (float *)BW;
		gp.bw_0.count = (size_t)PH_J * 16;
		gp.jpos_0.data = jpos;
		gp.jpos_0.count = (size_t)PH_J * 3;
		gp.sw_0.data = (float *)SW;
		gp.sw_0.count = (size_t)PH_V * PH_J;
		gp.parents_0.data = (int *)par;
		gp.parents_0.count = PH_J;
		gp.outbw_0.data = outbw;
		gp.outbw_0.count = (size_t)PH_J * 16;
		ComputeVaryingInput vi = dispatch<ComputeVaryingInput>(1);
		skeleton_fit(&vi, nullptr, &gp);
	}

	double ev = 0;
	for (int i = 0; i < PH_V * 3; i++) {
		ev = fmax(ev, fabs((double)v0[i] - v0gold[i]));
	}
	double eb = 0;
	for (int i = 0; i < PH_J * 16; i++) {
		eb = fmax(eb, fabs((double)outbw[i] - bwgold[i]));
	}

	// rbf correctness: compare the kernel's jpos to a naive CSR matmul over v0
	double ej = 0;
	for (int i = 0; i < PH_J; i++) {
		double acc[3] = {0, 0, 0};
		for (int k = crow[i]; k < crow[i + 1]; k++) {
			for (int d = 0; d < 3; d++) {
				acc[d] += (double)val[k] * v0[col[k] * 3 + d];
			}
		}
		for (int d = 0; d < 3; d++) {
			ej = fmax(ej, fabs(acc[d] - jpos[i * 3 + d]));
		}
	}

	printf("slang-cpu v0 vs golden         : max|err| = %.3e m\n", ev);
	printf("slang-cpu rbf vs naive         : max|err| = %.3e\n", ej);
	printf("slang-cpu bind_world vs golden : max|err| = %.3e\n", eb);
	bool ok = ev < 1e-4 && ej < 1e-4 && eb < 1e-3;
	printf("%s\n", ok ? "OK" : "FAIL");
	return ok ? 0 : 1;
}
