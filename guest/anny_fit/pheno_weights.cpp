// SPDX-License-Identifier: MIT
#include "pheno_weights.h"

#include "soma_pheno.h"

static const float ANC_age[5] = { -1.f / 3.f, 0.f, 1.f / 3.f, 2.f / 3.f, 1.f };
static const float ANC_2[2] = { 0.f, 1.f };
static const float ANC_3[3] = { 0.f, 0.5f, 1.f };

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

void anny_pheno_weights(const float *mask, const float ident[11], const float *local, float *wi) {
	float wg[2], wa[5], wm[3], ww[3], wh[2], wp[2], wc[3], wfm[3];
	interp(ident[0], ANC_2, 2, wg);
	interp(ident[1], ANC_age, 5, wa);
	interp(ident[2], ANC_3, 3, wm);
	interp(ident[3], ANC_3, 3, ww);
	interp(ident[4], ANC_2, 2, wh);
	interp(ident[5], ANC_2, 2, wp);
	interp(ident[6], ANC_3, 3, wc);
	interp(ident[7], ANC_3, 3, wfm);
	float race[3] = { ident[8], ident[9], ident[10] };
	const float rs = race[0] + race[1] + race[2];
	if (rs > 0) {
		for (int i = 0; i < 3; i++) {
			race[i] /= rs;
		}
	} else {
		race[0] = race[1] = race[2] = 1.f / 3.f;
	}
	float phens[PH_NVAR];
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
	for (int k = 0; k < PH_NB; k++) {
		wi[k] = 0.f;
	}
	for (int k = 0; k < PH_NPH; k++) {
		float pr = 1.f;
		const float *mr = mask + k * PH_NVAR;
		for (int j = 0; j < PH_NVAR; j++) {
			pr *= mr[j] != 0.f ? phens[j] * mr[j] + (1.f - mr[j]) : 1.f;
		}
		wi[k] = pr;
	}
	for (int i = 0; local && i < PH_NLOC; i++) {
		wi[PH_NPH + 2 * i] = local[i] > 0.f ? local[i] : 0.f;
		wi[PH_NPH + 2 * i + 1] = local[i] < 0.f ? -local[i] : 0.f;
	}
}
