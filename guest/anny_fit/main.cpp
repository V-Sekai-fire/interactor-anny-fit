// SPDX-License-Identifier: MIT
//
// anny_fit.elf -- the ANNY-SOMA-X body fit as a godot-sandbox guest.
//
// The phenotype body (the five Lean-emitted Slang kernels as SPIR-V) runs on the GPU through rd_compute, and the in-guest L-BFGS-B
// (vendor/solve/core/lbfgsb) takes its vector steps on the GPU too (VecRd) or on the CPU (VecCpu). The objective is the host harness's
// (sinew-mocap/solve core/lbfgsb/tests/anny_fit/fit_anny.cpp): pseudo-Huber relative segment-length error, a pull to the mean body, bounds
// [0, 1] for the 11 phenotype values and [-1, 1] for the local-change sliders.
//
// One host frame is one step (AGENTS.md rule 4: a GPU phase is read on a later tick than the one that submitted it). Every
// ADD_API_FUNCTION has a no-argument wrapper in the project script.
//   anny_data_put("pheno", bytes) in chunks, then "targets" and "labels" as text; anny_fit_start(config); anny_fit_tick() each frame;
//   anny_fit_report().
#include <api.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "lbfgsb.h"
#include "pheno_spv.h"
#include "pheno_weights.h"
#include "rd_compute.h"
#include "soma_pheno.h"
#include "vec_cpu.h"
#include "vec_rd.h"

namespace {

struct Seg {
	std::string name;
	int a = 0, b = 0;
	double len = 0;
};

constexpr double LAMBDA = 1e-4;
constexpr double SLIDER_LAMBDA = 2e-3;
constexpr double HUBER = 0.25;

rdc::Device g_dev;
std::vector<uint8_t> g_blob;
std::string g_targets, g_labels;

struct Gpu {
	::RID coeffs, outbw;
	::RID shader[5], pipeline[5], set[5];
	uint32_t groups[5] = {};
	std::vector<::RID> all_buffers;
};
Gpu g_gpu;

struct Job {
	bool active = false, done = false;
	std::string error;
	int n = 11;
	std::vector<int> free_slot; // slider index of variable 11 + j
	std::vector<std::string> labels;
	std::vector<Seg> segs;
	std::vector<float> mask; // PH_NPH x PH_NVAR
	std::unique_ptr<lbv::Vec> vec;
	std::unique_ptr<Lbfgsb> solver;
	Lbfgsb::Status st = Lbfgsb::BUSY;
	std::vector<float> x, x0, lb, ub;
	std::vector<std::vector<float>> pts;
	std::vector<double> fv;
	size_t evi = 0;
	bool in_grad = false, pending = false;
	std::vector<float> bw = std::vector<float>(PH_J * 16);
	double f0 = 0, f_last = 0;
	int ticks = 0, evals = 0;
	std::string backend;
} g_job;

Variant text(const std::string &s) {
	return Variant(String(s));
}

bool buf(::RID &out, size_t bytes, const void *data) {
	out = g_dev.storage_buffer(bytes, data);
	if (out.index != 0) {
		g_gpu.all_buffers.push_back(out);
	}
	return out.index != 0;
}

double seglen(const Seg &s, const std::vector<float> &bw) {
	const float *A = &bw[s.a * 16], *B = &bw[s.b * 16];
	return std::sqrt(double(A[3] - B[3]) * (A[3] - B[3]) + double(A[7] - B[7]) * (A[7] - B[7]) + double(A[11] - B[11]) * (A[11] - B[11]));
}

double score(const std::vector<float> &x, const std::vector<float> &bw, const std::vector<Seg> &segs, int n) {
	double f = 0.0;
	for (const Seg &s : segs) {
		const double r = seglen(s, bw) / s.len - 1.0;
		f += HUBER * HUBER * (std::sqrt(1.0 + (r / HUBER) * (r / HUBER)) - 1.0);
	}
	for (int i = 0; i < 11; ++i) {
		f += LAMBDA * (x[i] - 0.5) * (x[i] - 0.5);
	}
	for (int i = 11; i < n; ++i) {
		f += SLIDER_LAMBDA * x[i] * x[i];
	}
	return f;
}

// Upload the asset's arrays, make the five pipelines. After this the blob is dropped (the mask stays on the host).
bool gpu_setup(std::string &err) {
	if (!g_dev.open()) {
		err = g_dev.error();
		return false;
	}
	const size_t P = PH_P, V = PH_V, J = PH_J, NFS = PH_NFS, NNZ = PH_NNZ;
	const uint8_t *b = g_blob.data();
	::RID templ, blend, vanny, fsrc, p3, ftet, fids, bary, v0, crow, col, val, jpos, bsh, bwb, sw, par;
	const bool ok = buf(templ, P * 12, b + PH_OFF_template) && buf(blend, size_t(PH_NB) * P * 12, b + PH_OFF_blend) && buf(g_gpu.coeffs, size_t(PH_NB) * 4, nullptr) &&
			buf(vanny, P * 12, nullptr) && buf(fsrc, NFS * 12, b + PH_OFF_F_src) && buf(p3, NFS * 12, nullptr) && buf(ftet, NFS * 16, b + PH_OFF_F_tet) &&
			buf(fids, V * 4, b + PH_OFF_fids) && buf(bary, V * 16, b + PH_OFF_bary) && buf(v0, V * 12, nullptr) && buf(crow, (J + 1) * 4, b + PH_OFF_rbf_crow) &&
			buf(col, NNZ * 4, b + PH_OFF_rbf_col) && buf(val, NNZ * 4, b + PH_OFF_rbf_val) && buf(jpos, J * 12, nullptr) && buf(bsh, V * 12, b + PH_OFF_BSh) &&
			buf(bwb, J * 64, b + PH_OFF_BW) && buf(sw, V * J * 4, b + PH_OFF_SW) && buf(par, J * 4, b + PH_OFF_par) && buf(g_gpu.outbw, J * 64, nullptr);
	if (!ok) {
		err = "buffers: " + g_dev.error();
		return false;
	}
	const uint8_t *spv[5] = { pheno_blendshape_spv, pheno_bary_tet_spv, pheno_bary_gather_spv, pheno_rbf_spv, pheno_skeleton_fit_spv };
	const size_t spv_bytes[5] = { sizeof pheno_blendshape_spv, sizeof pheno_bary_tet_spv, sizeof pheno_bary_gather_spv, sizeof pheno_rbf_spv, sizeof pheno_skeleton_fit_spv };
	const uint32_t threads[5] = { uint32_t(P), uint32_t(NFS), uint32_t(V), uint32_t(J), 1 };
	const std::vector<::RID> binds[5] = { { templ, blend, g_gpu.coeffs, vanny }, { vanny, fsrc, p3 }, { vanny, p3, ftet, fids, bary, v0 }, { v0, crow, col, val, jpos },
		{ v0, bsh, bwb, jpos, sw, par, g_gpu.outbw } };
	for (int k = 0; k < 5; ++k) {
		g_gpu.shader[k] = g_dev.shader_from_spirv(spv[k], spv_bytes[k]);
		g_gpu.pipeline[k] = g_dev.compute_pipeline(g_gpu.shader[k]);
		std::vector<rdc::Binding> bd;
		for (size_t i = 0; i < binds[k].size(); ++i) {
			bd.push_back({ uint32_t(i), rdc::UNIFORM_TYPE_STORAGE_BUFFER, binds[k][i] });
		}
		g_gpu.set[k] = g_dev.uniform_set(g_gpu.shader[k], bd);
		g_gpu.groups[k] = (threads[k] + 63) / 64;
		if (!g_gpu.pipeline[k].index == 0 || g_gpu.set[k].index == 0) {
			err = "pipeline " + std::to_string(k) + ": " + g_dev.error();
			return false;
		}
	}
	return true;
}

// Record and submit one body evaluation; the result is read on a later tick.
bool eval_begin(const std::vector<float> &x) {
	std::vector<float> local(PH_NLOC, 0.0f);
	for (size_t j = 0; j < g_job.free_slot.size(); ++j) {
		local[g_job.free_slot[j]] = x[11 + j];
	}
	std::vector<float> wi(PH_NB);
	anny_pheno_weights(g_job.mask.data(), x.data(), local.data(), wi.data());
	if (!g_dev.buffer_update(g_gpu.coeffs, 0, wi.size() * 4, wi.data())) { // before list_begin: Godot refuses it inside a list
		g_job.error = "coefficients: " + g_dev.error();
		return false;
	}
	if (!g_dev.list_begin()) {
		g_job.error = "list_begin: " + g_dev.error();
		return false;
	}
	for (int k = 0; k < 5; ++k) {
		g_dev.bind_pipeline(g_gpu.pipeline[k]);
		g_dev.bind_uniform_set(g_gpu.set[k]);
		g_dev.dispatch(g_gpu.groups[k]);
		if (k < 4) {
			g_dev.barrier();
		}
	}
	g_dev.list_end();
	g_dev.submit();
	return true;
}

bool eval_end() {
	g_dev.sync();
	return g_dev.buffer_get_into(g_gpu.outbw, 0, PH_J * 64, g_job.bw.data());
}

std::string progress() {
	std::ostringstream o;
	o << "iter=" << g_job.solver->iterations() << " evals=" << g_job.evals << " f=" << g_job.f_last << " ticks=" << g_job.ticks;
	return o.str();
}

void finish() {
	g_job.done = true;
	g_job.active = false;
	g_job.solver->readX(g_job.x);
}

Variant anny_data_clear() {
	g_blob.clear();
	g_blob.shrink_to_fit();
	g_targets.clear();
	g_labels.clear();
	return Variant(int64_t(0));
}

// "pheno": the baked soma_pheno.bin, in chunks of at most 16 MiB, in order. "targets", "labels": text.
Variant anny_data_put(String key_s, PackedArray<uint8_t> bytes) {
	const std::string key = key_s.utf8();
	const std::vector<uint8_t> v = bytes.fetch();
	if (key == "pheno") {
		g_blob.insert(g_blob.end(), v.begin(), v.end());
	} else if (key == "targets") {
		g_targets.append(v.begin(), v.end());
	} else if (key == "labels") {
		g_labels.append(v.begin(), v.end());
	} else {
		return Variant(int64_t(-1));
	}
	return Variant(int64_t(g_blob.size()));
}

// config: "sliders=a,b;narrow=1;vec=rd;iters=100": slider label substrings to free, narrow to the SOMA-X set, the vector backend, iteration cap.
Variant anny_fit_start(String config_s) {
	g_job = Job();
	std::string sliders, vec = "rd";
	bool narrow = false;
	int iters = 100;
	std::istringstream cs(config_s.utf8());
	for (std::string kv; std::getline(cs, kv, ';');) {
		const size_t eq = kv.find('=');
		const std::string k = kv.substr(0, eq), v = eq == std::string::npos ? "" : kv.substr(eq + 1);
		if (k == "sliders") {
			sliders = v;
		} else if (k == "narrow") {
			narrow = v == "1";
		} else if (k == "vec") {
			vec = v;
		} else if (k == "iters") {
			iters = std::atoi(v.c_str());
		}
	}
	if (g_blob.size() != size_t(PH_BIN_BYTES)) {
		return text("FAIL pheno asset is " + std::to_string(g_blob.size()) + " bytes, expected " + std::to_string(PH_BIN_BYTES));
	}
	g_job.mask.assign(reinterpret_cast<const float *>(g_blob.data() + PH_OFF_mask), reinterpret_cast<const float *>(g_blob.data() + PH_OFF_mask) + size_t(PH_NPH) * PH_NVAR);
	std::string err;
	if (!gpu_setup(err)) {
		return text("FAIL " + err);
	}
	g_blob.clear();
	g_blob.shrink_to_fit();
	// targets
	for (std::istringstream ts(g_targets); ts;) {
		Seg s;
		if (ts >> s.name >> s.a >> s.b >> s.len) {
			g_job.segs.push_back(s);
		} else {
			break;
		}
	}
	// sliders
	std::vector<std::string> keys;
	for (std::istringstream ss(sliders); ss;) {
		std::string k;
		if (!std::getline(ss, k, ',')) {
			break;
		}
		keys.push_back(k);
	}
	std::istringstream ls(g_labels);
	int idx = 0;
	for (std::string l; std::getline(ls, l) && idx < PH_NLOC; ++idx) {
		const size_t tab = l.find('\t');
		const bool keeps = tab == std::string::npos || l[tab + 1] == '1';
		const std::string label = l.substr(0, tab);
		g_job.labels.push_back(label);
		for (const std::string &k : keys) {
			if (label.find(k) != std::string::npos && (!narrow || keeps)) {
				g_job.free_slot.push_back(idx);
				break;
			}
		}
	}
	g_job.n = 11 + int(g_job.free_slot.size());
	g_job.x0.assign(g_job.n, 0.5f);
	g_job.lb.assign(g_job.n, 0.0f);
	g_job.ub.assign(g_job.n, 1.0f);
	for (int i = 11; i < g_job.n; ++i) {
		g_job.x0[i] = 0.0f;
		g_job.lb[i] = -1.0f;
	}
	g_job.backend = vec;
	if (vec == "rd") {
		auto v = std::make_unique<VecRd>(g_dev);
		if (!v->ok()) {
			return text("FAIL VecRd: " + v->error());
		}
		g_job.vec = std::move(v);
	} else {
		g_job.vec = std::make_unique<VecCpu>();
	}
	g_job.solver = std::make_unique<Lbfgsb>(*g_job.vec);
	LbfgsbParams p;
	p.max_iterations = iters;
	p.epsilon = 1e-4;
	g_job.st = g_job.solver->start(uint32_t(g_job.n), g_job.x0.data(), g_job.lb.data(), g_job.ub.data(), p);
	g_job.x = g_job.x0;
	g_job.active = true;
	return text("STARTED vec=" + vec + " segments=" + std::to_string(g_job.segs.size()) + " variables=" + std::to_string(g_job.n) + " device=" + g_dev.device_name());
}

// One host frame: finish the evaluation submitted last frame, or take the next step.
Variant anny_fit_tick() {
	if (!g_job.active) {
		return text(g_job.done ? "DONE" : "IDLE");
	}
	++g_job.ticks;
	if (g_job.pending) {
		if (!eval_end()) {
			g_job.active = false;
			return text("FAIL read: " + g_dev.error());
		}
		g_job.fv[g_job.evi] = score(g_job.pts[g_job.evi], g_job.bw, g_job.segs, g_job.n);
		++g_job.evi;
		++g_job.evals;
		g_job.pending = false;
		return text("RUNNING " + progress());
	}
	if (g_job.in_grad) {
		if (g_job.evi < g_job.pts.size()) {
			if (!eval_begin(g_job.pts[g_job.evi])) {
				g_job.active = false;
				return text("FAIL " + g_job.error);
			}
			g_job.pending = true;
			return text("RUNNING " + progress());
		}
		std::vector<float> g(g_job.n);
		for (int j = 0; j < g_job.n; ++j) {
			const float den = g_job.pts[1 + 2 * j][j] - g_job.pts[2 + 2 * j][j];
			g[j] = float((g_job.fv[1 + 2 * j] - g_job.fv[2 + 2 * j]) / den);
		}
		g_job.f_last = g_job.fv[0];
		if (g_job.ticks == g_job.evals + 1 || g_job.f0 == 0) {
			g_job.f0 = g_job.fv[0];
		}
		g_job.solver->setGradient(g.data());
		g_job.in_grad = false;
		g_job.st = g_job.solver->next(g_job.fv[0]);
		return text("RUNNING " + progress());
	}
	switch (g_job.st) {
		case Lbfgsb::BUSY:
		case Lbfgsb::ACCEPT:
			g_job.st = g_job.solver->next();
			break;
		case Lbfgsb::NEED_EVAL:
		case Lbfgsb::TRY: {
			g_job.solver->readX(g_job.x);
			const double h = 1e-3;
			g_job.pts.assign(1, g_job.x);
			for (int j = 0; j < g_job.n; ++j) {
				std::vector<float> xp = g_job.x, xm = g_job.x;
				xp[j] = std::min(g_job.ub[j], g_job.x[j] + float(h));
				xm[j] = std::max(g_job.lb[j], g_job.x[j] - float(h));
				g_job.pts.push_back(xp);
				g_job.pts.push_back(xm);
			}
			g_job.fv.assign(g_job.pts.size(), 0.0);
			g_job.evi = 0;
			g_job.in_grad = true;
			break;
		}
		case Lbfgsb::CONVERGED:
		case Lbfgsb::FAIL:
			finish();
			return text(std::string("DONE ") + Lbfgsb::status_name(g_job.st) + " " + progress());
	}
	return text("RUNNING " + progress());
}

Variant anny_fit_report() {
	std::ostringstream o;
	const char *names[11] = { "gender", "age", "muscle", "weight", "height", "proportions", "cupsize", "firmness", "african", "asian", "caucasian" };
	o << "backend=" << g_job.backend << " iterations=" << g_job.solver->iterations() << " evaluations=" << g_job.evals << " ticks=" << g_job.ticks << " objective " << g_job.f0
	  << " -> " << g_job.f_last << " (" << g_job.solver->reason() << ")\n";
	for (int i = 0; i < 11; ++i) {
		o << "  " << names[i] << " " << g_job.x[i] << "\n";
	}
	for (int j = 0; j < int(g_job.free_slot.size()); ++j) {
		if (std::fabs(g_job.x[11 + j]) > 0.02f) {
			o << "  slider " << g_job.labels[g_job.free_slot[j]] << " " << g_job.x[11 + j] << "\n";
		}
	}
	return text(o.str());
}

} // namespace

int main() {
	ADD_API_FUNCTION(anny_data_clear, "int", "", "Drop the uploaded asset and texts");
	ADD_API_FUNCTION(anny_data_put, "int", "String key, PackedByteArray bytes", "Append pheno (soma_pheno.bin in order), targets or labels; the byte count held for pheno");
	ADD_API_FUNCTION(anny_fit_start, "String", "String config", "Start a fit: sliders=a,b;narrow=0|1;vec=rd|cpu;iters=N");
	ADD_API_FUNCTION(anny_fit_tick, "String", "", "One host frame of the fit: RUNNING, DONE or FAIL");
	ADD_API_FUNCTION(anny_fit_report, "String", "", "The fitted variables and the counts");
	return 0;
}
