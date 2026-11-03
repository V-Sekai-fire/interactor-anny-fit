// lbfgsb_gate -- Gate 5 G1 and G2 for the in-guest L-BFGS-B, as resumable
// runners that both the drape.elf jobs (lbfgsb_jobs.cpp, frame-driven) and
// the host-native test (tests/lbfgsb_driver) step.
//
//  - ComponentRun (G1): one LBFGSpp component fixture
//    (gates/5-drape/oracle/components/comp_NN.txt) replayed through an
//    lbv::Vec in the order the driver dispatches (lbfgsb.cpp): reset; per
//    history pair the five products, lb_compact, lb_ring_store; then M by
//    columns and M*v, pg_inf, the Cauchy point, drt0 = normalize(xcp - x)
//    and its step_max, the subspace step, g.drt and step_max. Checks as
//    tests/drape_kernels/test.cpp: rel <= 1e-4 (drt, dg, step_max <= 5e-4),
//    the sets identical as sets. `corrupt` scales theta by 1.25 (the control).
//  - ProblemRun (G2): one oracle trace (gates/5-drape/oracle/traces) run by
//    Lbfgsb over an lbv::Vec, the objective evaluated here in double, and the
//    result compared with LBFGSpp's: f within 1e-6 (abs + rel), x within
//    1e-3 (rel, inf-norm), the final active sets identical, iterations within
//    max(2, 25%).
//
// step() does at most one submit and returns; call it again (on the GPU on a
// later tick) until done(). No api.hpp here: the host test compiles it too.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "lbfgsb.h"
#include "lbfgsb_vec.h"

namespace lbg {

// "key <count> v..." / "key v" lines; inf/-inf read as +-inf.
struct KeyVals {
	std::map<std::string, std::vector<double>> vec;
	std::map<std::string, std::string> line; // the raw rest of each line (last wins)
	std::vector<std::vector<double>> s, y, mvIn, mvOut;
	std::vector<double> M;
	int K = 0;
	bool parseFixture(const std::string &text);
	double scalar(const std::string &k, double dflt = 0.0) const;
	const std::vector<double> &v(const std::string &k) const;
};

// The worst relative error per quantity over a set of fixtures.
struct Worst {
	std::map<std::string, double> e;
	void add(const std::string &k, double x) {
		double &w = e[k];
		if (x > w || w != w) {
			w = x;
		}
	}
	std::string table() const;
};

class ComponentRun {
public:
	ComponentRun(std::string name, std::string text, bool corrupt) :
			name_(std::move(name)), text_(std::move(text)), corrupt_(corrupt) {}
	// One stage: at most one submit. False once done (or on error).
	bool step(lbv::Vec &v);
	bool done() const { return done_; }
	bool pass() const { return pass_; }
	const std::string &line() const { return line_; }
	void addWorst(Worst &w) const;

private:
	void check(lbv::Vec &v);
	std::string name_, text_;
	bool corrupt_;
	KeyVals f_;
	int stage_ = 0;
	size_t pair_ = 0;
	uint32_t n_ = 0, m_ = 0, K_ = 0;
	bool curvOk_ = true;
	bool done_ = false, pass_ = false;
	std::string line_;
	std::map<std::string, double> err_;
};

// A G2 problem, from gates/5-drape/oracle/problems/<name>.txt.
struct Problem {
	std::string name;
	uint32_t n = 0;
	std::vector<double> lb, ub, x0, aDiag, b;
	double aOff = -1.0;
	bool parse(const std::string &text, std::string &err);
	// f(x) and g(x) in double (the oracle's own formulas, tests/lbfgsb_oracle/gen.cpp).
	double fg(const std::vector<float> &x, std::vector<double> &g) const;
};

// A G2 reference, from gates/5-drape/oracle/traces/<problem>_m<M>_<tag>.txt.
struct Trace {
	std::string problem, params, status;
	int niter = 0, nfev = 0;
	double fFinal = 0.0, pgFinal = 0.0;
	std::vector<double> x;
	std::vector<int> L, U;
	// Per iterate k: f and the projected-gradient norm.
	std::vector<double> iterF, iterPg;
	bool parse(const std::string &text, std::string &err);
};

class ProblemRun {
public:
	ProblemRun(std::string name, const Problem &p, const Trace &t, lbv::Vec &v) :
			name_(std::move(name)), p_(p), t_(t), drv_(v), v_(v) {}
	bool step();
	bool done() const { return done_; }
	bool pass() const { return pass_; }
	const std::string &line() const { return line_; }
	const Lbfgsb &driver() const { return drv_; }
	// Worst-case pass margins (measured / limit) for the summary.
	double fRatio = 0, xRatio = 0, itRatio = 0;
	// Per accepted iterate k (1..): f and pg; [0] is x0's.
	std::vector<double> iterF, iterPg;
	// The first iterate whose f differs from the trace's by more than 1e-6
	// (abs + rel), or -1.
	int firstDivergence() const;

private:
	void finish(bool converged);
	std::string name_;
	const Problem &p_;
	const Trace &t_;
	Lbfgsb drv_;
	lbv::Vec &v_;
	bool started_ = false, done_ = false, pass_ = false;
	std::vector<float> x_;
	std::vector<double> g_;
	std::vector<float> gf_;
	std::string line_;
};

} // namespace lbg
