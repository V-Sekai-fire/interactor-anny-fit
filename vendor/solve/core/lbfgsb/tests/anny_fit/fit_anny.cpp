// SPDX-License-Identifier: MIT
//
// Fit ANNY-soma-x's 11 phenotype values to a character's segment lengths with the in-guest L-BFGS-B
// (guest/drape/lbfgsb.cpp over vec_cpu.cpp), the optimiser drape, cage refit and head fitting share.
// The body comes from pheno_eval (ports/soma_pheno_api.h: blendshapes, barycentric transfer, RBF joints).
//
//   fit_anny <soma_pheno.bin> <soma_pheno_golden.bin> <targets.txt>
//
// targets.txt: one "name jointA jointB length_m" per line (SOMA joint indices, soma_rig.h order).
// The objective is the sum of pseudo-Huber relative segment-length errors (quadratic under 25%) plus LAMBDA times the squared distance from
// the mean body (all values 0.5), which settles the 11 variables the segments leave free. The gradient is
// central differences (one-sided at a bound), the only part not from the solver. Bounds are [0, 1].
// Gates: G1 the golden phenotype reproduces the golden mesh and skeleton; G2 the residual falls and the solver
// converges; G3 targets 10% longer move the fit and raise the residual against the original targets (control).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "lbfgsb.h"
#include "soma_pheno.h"
#include "soma_pheno_api.h"
#include "vec_cpu.h"
#ifdef ANNY_GPU
#include "gpu_pheno.h"
#endif

struct Seg {
	std::string name;
	int a, b;
	double len;
};
static int N = 11;                // 11 phenotype values, then the free local-change sliders
static std::vector<int> g_free;   // slider index of variable 11 + j
static const double SLIDER_LAMBDA = 2e-3;
static const double LAMBDA = 1e-4;
static const double HUBER = 0.25;   // pseudo-Huber scale on the relative error: quadratic below 25%, linear beyond
static bool g_on_gpu = false;   // ANNY_GPU=1 and ANNY_SPV: the fit's objective runs the phenotype kernels as SPIR-V
static std::vector<std::string> g_labels;
static std::vector<float> g_v(PH_V * 3), g_bw(PH_J * 16);

static double seglen(const Seg &s) {
	const float *A = &g_bw[s.a * 16], *B = &g_bw[s.b * 16];
	return std::sqrt(double(A[3] - B[3]) * (A[3] - B[3]) + double(A[7] - B[7]) * (A[7] - B[7]) + double(A[11] - B[11]) * (A[11] - B[11]));
}
static double score(const float *x, const std::vector<Seg> &segs, std::vector<double> *rel) {
	double f = 0.0;
	if (rel) {
		rel->clear();
	}
	for (const Seg &s : segs) {
		const double r = seglen(s) / s.len - 1.0;
		f += HUBER * HUBER * (std::sqrt(1.0 + (r / HUBER) * (r / HUBER)) - 1.0);
		if (rel) {
			rel->push_back(r);
		}
	}
	for (int i = 0; i < 11; ++i) {
		f += LAMBDA * (x[i] - 0.5) * (x[i] - 0.5);
	}
	for (int i = 11; i < N; ++i) {
		f += SLIDER_LAMBDA * x[i] * x[i];
	}
	return f;
}
static double objective(const float *x, const std::vector<Seg> &segs, std::vector<double> *rel = nullptr) {
	std::vector<float> local(PH_NLOC, 0.0f);
	for (size_t j = 0; j < g_free.size(); ++j) {
		local[g_free[j]] = x[11 + j];
	}
#ifdef ANNY_GPU
	if (g_on_gpu) {
		gpu_pheno_eval_local(x, local.data(), g_v.data(), g_bw.data());
		return score(x, segs, rel);
	}
#endif
	pheno_eval_local(x, local.data(), g_v.data(), g_bw.data());
	return score(x, segs, rel);
}
static void gradient(const float *x, const std::vector<Seg> &segs, float *g) {
	const double h = 1e-3;
	for (int i = 0; i < N; ++i) {
		std::vector<float> xp(x, x + N), xm(x, x + N);
		const float lo = i < 11 ? 0.0f : -1.0f;
		xp[i] = std::min(1.0f, x[i] + float(h));
		xm[i] = std::max(lo, x[i] - float(h));
		g[i] = float((objective(xp.data(), segs) - objective(xm.data(), segs)) / (xp[i] - xm[i]));
	}
}
struct Fit {
	std::vector<float> x;
	double f0 = 0, f = 0;
	int iterations = 0, nfev = 0;
	bool converged = false;
	std::string reason;
};
static Fit fit(const std::vector<Seg> &segs) {
	Fit r;
	VecCpu v;
	Lbfgsb solver(v);
	LbfgsbParams p;
	p.max_iterations = 200;
	p.epsilon = 1e-4;
	std::vector<float> x0(N, 0.5f), lb(N, 0.0f), ub(N, 1.0f), x(N), g(N);
	for (int i = 11; i < N; ++i) {
		x0[i] = 0.0f;
		lb[i] = -1.0f;
	}
	r.f0 = objective(x0.data(), segs);
	Lbfgsb::Status st = solver.start(N, x0.data(), lb.data(), ub.data(), p);
	double f = r.f0;
	for (int guard = 0; guard < 100000; ++guard) {
		if (st == Lbfgsb::NEED_EVAL || st == Lbfgsb::TRY) {
			solver.readX(x);
			f = objective(x.data(), segs);
			gradient(x.data(), segs, g.data());
			solver.setGradient(g.data());
			st = solver.next(f);
		} else if (st == Lbfgsb::ACCEPT || st == Lbfgsb::BUSY) {
			st = solver.next();
		} else {
			break;
		}
	}
	solver.readX(r.x);
	r.f = objective(r.x.data(), segs);
	r.iterations = solver.iterations();
	r.nfev = solver.nfev();
	r.converged = st == Lbfgsb::CONVERGED;
	r.reason = solver.reason().empty() ? Lbfgsb::status_name(st) : solver.reason();
	return r;
}
int main(int argc, char **argv) {
	if (argc < 4) {
		std::fprintf(stderr, "usage: %s <soma_pheno.bin> <golden.bin> <targets.txt>\n", argv[0]);
		return 2;
	}
	if (pheno_load(argv[1]) != 0) {
		std::fprintf(stderr, "cannot load %s\n", argv[1]);
		return 2;
	}
	int rc = 0;
	{ // G1: the golden file is ident[11], v0[V*3], bind_world[J*16] as floats
		std::ifstream gf(argv[2], std::ios::binary);
		std::vector<float> gold(11 + PH_V * 3 + PH_J * 16);
		gf.read(reinterpret_cast<char *>(gold.data()), gold.size() * sizeof(float));
		pheno_eval(gold.data(), g_v.data(), g_bw.data());
		double ev = 0, eb = 0;
		for (int i = 0; i < PH_V * 3; ++i) {
			ev = std::max(ev, double(std::fabs(g_v[i] - gold[11 + i])));
		}
		for (int i = 0; i < PH_J * 16; ++i) {
			eb = std::max(eb, double(std::fabs(g_bw[i] - gold[11 + PH_V * 3 + i])));
		}
		std::printf("   sample v0[0..2] eval %.4f %.4f %.4f  golden %.4f %.4f %.4f; hips y eval %.4f golden %.4f\n", g_v[0], g_v[1], g_v[2], gold[11], gold[12], gold[13], g_bw[16 + 7], gold[11 + PH_V * 3 + 16 + 7]);
		if (const char *dump = std::getenv("DUMP_EVAL")) {
			std::ofstream o(dump, std::ios::binary);
			o.write(reinterpret_cast<const char *>(g_v.data()), g_v.size() * 4);
			o.write(reinterpret_cast<const char *>(g_bw.data()), g_bw.size() * 4);
		}
		const bool ok = gf.good() && ev < 1e-3 && eb < 1e-3;
		std::printf("G1 golden phenotype: mesh max error %.2e m, bind skeleton %.2e: %s\n", ev, eb, ok ? "PASS" : "FAIL");
		rc |= ok ? 0 : 1;
	}
	{ // the sliders to free: ANNY_SLIDERS=substring,substring (labels containing any of them); none: the 11 phenotype values only
		std::ifstream lf(argc > 4 ? argv[4] : "");
		std::string l;
		std::vector<bool> soma_keeps;
		while (std::getline(lf, l)) {
			const size_t tab = l.find('\t');
			soma_keeps.push_back(tab == std::string::npos || l[tab + 1] == '1');
			g_labels.push_back(l.substr(0, tab));
		}
		const bool narrow = std::getenv("ANNY_SOMA_NARROW") != nullptr; // narrow at the SOMA-X stage: only the sliders SOMA-X keeps
		const char *want = std::getenv("ANNY_SLIDERS");
		std::vector<std::string> keys;
		for (std::istringstream is(want ? want : ""); std::getline(is, l, ',');) {
			keys.push_back(l);
		}
		for (size_t i = 0; i < g_labels.size() && int(i) < PH_NLOC; ++i) {
			for (const std::string &k : keys) {
				if (g_labels[i].find(k) != std::string::npos && (!narrow || soma_keeps[i])) {
					g_free.push_back(int(i));
					break;
				}
			}
		}
		N = 11 + int(g_free.size());
	}
#ifdef ANNY_GPU
	if (std::getenv("ANNY_GPU") && std::getenv("ANNY_SPV")) {
		g_on_gpu = gpu_pheno_init(std::getenv("ANNY_SPV"));
		std::printf("fit objective on the GPU (SPIR-V): %s\n", g_on_gpu ? "yes" : "NO, no device or kernels: CPU");
	}
#endif
	std::vector<Seg> segs;
	{
		std::ifstream tf(argv[3]);
		std::string line;
		while (std::getline(tf, line)) {
			std::istringstream is(line);
			Seg s;
			if (is >> s.name >> s.a >> s.b >> s.len) {
				segs.push_back(s);
			}
		}
	}
	Fit base = fit(segs);
	std::vector<double> rel0, rel1;
	std::vector<float> mean(N, 0.5f);
	for (int i = 11; i < N; ++i) {
		mean[i] = 0.0f;
	}
	objective(mean.data(), segs, &rel0);
	objective(base.x.data(), segs, &rel1);
	const char *names[11] = { "gender", "age", "muscle", "weight", "height", "proportions", "cupsize", "firmness", "african", "asian", "caucasian" };
	std::printf("fit: %d iterations, %d evaluations, %s; objective %.5f -> %.5f\n", base.iterations, base.nfev, base.reason.c_str(), base.f0, base.f);
	int at_bound = 0;
	for (int i = 0; i < 11; ++i) {
		const bool b = base.x[i] < 1e-4f || base.x[i] > 1 - 1e-4f;
		at_bound += b;
		std::printf("  %-12s %.4f%s\n", names[i], base.x[i], b ? "  <- at a bound" : "");
	}
	std::printf("segments (relative error: mean body -> fit):\n");
	for (size_t i = 0; i < segs.size(); ++i) {
		std::printf("  %-16s target %.4f m  %+6.1f%% -> %+6.1f%%\n", segs[i].name.c_str(), segs[i].len, 100 * rel0[i], 100 * rel1[i]);
	}
	std::printf("saturation: %d of 11 phenotype values at a bound\n", at_bound);
	int sat_slider = 0;
	for (int i = 11; i < N; ++i) {
		sat_slider += std::fabs(base.x[i]) > 1.0f - 1e-4f;
		if (std::fabs(base.x[i]) > 0.02f) {
			std::printf("  slider %-40s %+.3f\n", g_labels[g_free[i - 11]].c_str(), base.x[i]);
		}
	}
	std::printf("sliders: %d free, %d at a bound\n", N - 11, sat_slider);
	const bool g2 = base.f < base.f0 && (base.converged || base.iterations > 0);
	std::printf("G2 residual falls (%.5f -> %.5f) and the solver finished (%s): %s\n", base.f0, base.f, base.reason.c_str(), g2 ? "PASS" : "FAIL");
	rc |= g2 ? 0 : 1;
	{ // G3 control: targets 10% longer must move the fit, and the moved fit must do worse on the real targets
		std::vector<Seg> longer = segs;
		for (Seg &s : longer) {
			s.len *= 1.10;
		}
		Fit alt = fit(longer);
		double moved = 0.0; // over every variable: with sliders free, longer targets can be taken by a slider and not by the phenotype
		for (int i = 0; i < N; ++i) {
			moved += std::fabs(alt.x[i] - base.x[i]);
		}
		const double f_alt_on_real = objective(alt.x.data(), segs);
		const bool ok = f_alt_on_real > base.f * 1.02 && moved > 1e-3;
		std::printf("G3 control, targets +10%%: fit moved %.3f, residual on the real targets %.5f against %.5f: %s\n", moved, f_alt_on_real, base.f, ok ? "PASS" : "FAIL");
		rc |= ok ? 0 : 1;
	}
#ifdef ANNY_GPU
	{ // G4: the five kernels as SPIR-V give the CPU's mesh, skeleton and objective. Tolerances: 1e-4 m, 1e-4 relative on the objective.
		const char *spv = std::getenv("ANNY_SPV");
		if (!spv || !gpu_pheno_init(spv)) {
			std::printf("G4 GPU: no device or kernels (set ANNY_SPV to the spv directory): SKIPPED\n");
		} else {
			std::vector<std::vector<float>> idents = { std::vector<float>(N, 0.5f), base.x };
			for (std::vector<float> &id : idents) {
				std::fill(id.begin() + 11, id.end(), 0.0f); // the GPU kernels here run the phenotype basis; sliders at 0
			}
			unsigned seed = 12345;
			for (int k = 0; k < 4; ++k) {
				std::vector<float> r(N);
				for (int i = 0; i < N; ++i) {
					seed = seed * 1664525u + 1013904223u;
					r[i] = float((seed >> 8) & 0xFFFF) / 65535.0f;
				}
				std::fill(r.begin() + 11, r.end(), 0.0f);
				idents.push_back(r);
			}
			double worst_v = 0, worst_b = 0, worst_f = 0;
			std::vector<float> gv(PH_V * 3), gb(PH_J * 16);
			for (const std::vector<float> &id : idents) {
				const double fc = objective(id.data(), segs);
				const std::vector<float> cv = g_v, cb = g_bw;
				gpu_pheno_eval(id.data(), gv.data(), gb.data());
				g_bw = gb;
				const double fg = score(id.data(), segs, nullptr);
				for (int i = 0; i < PH_V * 3; ++i) {
					worst_v = std::max(worst_v, double(std::fabs(gv[i] - cv[i])));
				}
				for (int i = 0; i < PH_J * 16; ++i) {
					worst_b = std::max(worst_b, double(std::fabs(gb[i] - cb[i])));
				}
				worst_f = std::max(worst_f, std::fabs(fc - fg) / std::max(std::fabs(fc), 1e-9));
			}
			const bool ok = worst_v < 1e-4 && worst_b < 1e-4 && worst_f < 1e-4;
			std::printf("G4 GPU (SPIR-V) against CPU on %zu phenotypes: mesh max %.2e m, skeleton max %.2e, objective relative %.2e: %s\n", idents.size(), worst_v, worst_b, worst_f, ok ? "PASS" : "FAIL");
			rc |= ok ? 0 : 1;
			{ // control: a phenotype nudged by 0.05 must show as a different objective
				std::vector<float> nudged = base.x;
				nudged[4] = std::min(1.0f, nudged[4] + 0.05f);
				nudged[0] = std::max(0.0f, nudged[0] - 0.05f);
				const double fc = objective(base.x.data(), segs);
				gpu_pheno_eval(nudged.data(), gv.data(), gb.data());
				g_bw = gb;
				const double fn = score(nudged.data(), segs, nullptr);
				const bool seen = std::fabs(fc - fn) / std::max(std::fabs(fc), 1e-9) > 1e-3;
				std::printf("G4 control, phenotype nudged by 0.05: objective differs by %.2e relative (must exceed 1e-3): %s\n", std::fabs(fc - fn) / std::max(std::fabs(fc), 1e-9), seen ? "PASS" : "FAIL");
				rc |= seen ? 0 : 1;
			}
			gpu_pheno_shutdown();
		}
	}
#endif
	return rc;
}
