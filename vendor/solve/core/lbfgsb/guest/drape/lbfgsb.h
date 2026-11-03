// lbfgsb -- the in-guest L-BFGS-B driver: LBFGSpp 0.3.0's LBFGSBSolver::
// minimize (LBFGSB.h) and LineSearchMoreThuente (LineSearchMoreThuente.h)
// as scalar control flow over the Lean-emitted kernels, in reverse
// communication.
//
// Only scalars live here: f, g.d, step sizes, the line search's bracket, the
// convergence tests. Every vector and matrix operation (the BFGS update, the
// Cauchy point, the subspace minimisation, projections, copies, dots) is a
// kernel dispatched through an lbv::Vec (vec_cpu: the slangc cpp emits;
// vec_rd: the SPIR-V over rdc::Device), grouped into phases: one list of ops,
// one submit on the GPU (AGENTS.md rules 2 and 4).
//
// The protocol. start() and next() return a Status:
//   BUSY       a phase was submitted; call next() again on a later tick (on
//              the CPU at once). Nothing to do.
//   NEED_EVAL  evaluate f and g at x (readX), setGradient(g), next(f): the
//              initial point.
//   TRY        the same, at a line-search trial point.
//   ACCEPT     an iterate was accepted (iterations(), fx(), pgNorm()); call
//              next() to go on (the next direction phase).
//   CONVERGED  done: x holds the result; reason() says which test
//              (grad, delta, max_iterations).
//   FAIL       error() says why (LBFGSpp would have thrown).
// g.d at a trial point is a device dot (it is a vector op), so next() takes
// only f.
//
// Phases per iteration: the direction phase (the BFGS update, force_bounds,
// Cauchy point, subspace minimisation, g.d and the feasible step, one
// submit), the trial point x = xp + step*d, and the post-evaluation phase
// (g.d, the projected gradient, |x|, and s, y and their products for the
// next update, computed speculatively while the line search decides). A
// first trial that passes the strong Wolfe test costs three submits.
//
// Where it departs from LBFGSpp:
//  - float32 vectors (df32 sums), double scalars;
//  - T's Cholesky (lb_compact) replaces Bunch-Kaufman. A non-positive pivot
//    (st[4] = 0), which Bunch-Kaufman would absorb, resets the BFGS memory and
//    recomputes the Cauchy point and direction from x, the reset LBFGSpp
//    itself applies on a pathological direction;
//  - LBFGSpp's line search swaps x/grad with x_lo/grad_lo; here "lo := the
//    current point" is a copy into XLO/GLO recorded ahead of the next trial,
//    and "x := lo" a copy back (the RESTORE phase), with the same fx/dg
//    bookkeeping (including the step_max exit, which keeps the trial's fx).
// SPDX-License-Identifier: Apache-2.0 OR MIT
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "lbfgsb_vec.h"

// LBFGSpp's LBFGSBParam, with its defaults.
struct LbfgsbParams {
	int m = 6;
	double epsilon = 1e-5;
	double epsilon_rel = 1e-5;
	int past = 1;
	double delta = 1e-10;
	int max_iterations = 0;
	int max_submin = 10;
	int max_linesearch = 20;
	double min_step = 1e-20;
	double max_step = 1e20;
	double ftol = 1e-4;
	double wolfe = 0.9;
	// "key value ..." (the oracle traces' `param` line); unknown keys refused.
	bool parse(const std::string &kv, std::string &err);
	std::string dump() const;
};

class Lbfgsb {
public:
	enum Status { BUSY, NEED_EVAL, TRY, ACCEPT, CONVERGED, FAIL };
	static const char *status_name(Status s);

	explicit Lbfgsb(lbv::Vec &v) :
			v_(v) {}

	// x0, lb, ub of length n; +-inf bounds may be given as +-inf or +-FLT_MAX.
	Status start(uint32_t n, const float *x0, const float *lb, const float *ub, const LbfgsbParams &p);
	Status next(double f = 0.0);

	// After NEED_EVAL / TRY: the point to evaluate (waits out a pending
	// submit, which by then is a later tick's); after CONVERGED: the result.
	bool readX(std::vector<float> &x);
	// After NEED_EVAL / TRY, before next(f).
	bool setGradient(const float *g);
	// A phase is in flight on the GPU.
	bool pending() const { return v_.pending(); }

	int iterations() const { return k_; }
	int nfev() const { return nfev_; }
	double fx() const { return fx_; }
	double pgNorm() const { return pg_; }
	double lastStep() const { return step_; }
	const std::string &reason() const { return reason_; }
	const std::string &error() const { return err_; }
	// Counters: BFGS resets on a pathological direction, on a Cholesky
	// failure, restores of x_lo, line-search evaluations beyond the first.
	int pathological() const { return patho_; }
	int cholResets() const { return cholResets_; }
	int restores() const { return restores_; }
	int extraEvals() const { return extraEvals_; }
	uint32_t n() const { return n_; }

private:
	enum Phase { P_NONE, P_INIT, P_X0, P_PREP, P_DIR, P_DIR_RESET, P_PATHO, P_TRIAL, P_POST, P_RESTORE };
	enum Await { A_NONE, A_EVAL0, A_TRIAL, A_ACCEPT, A_DONE };

	Status submit(Phase ph, const std::vector<lbv::Op> &ops);
	Status fail(const std::string &why);
	bool readSc();
	double dotAt(uint32_t slot) const { return double(sc_[slot]) + double(sc_[slot + 1]); }
	// The tail every direction phase ends with: xp = x, gp = g, g.d, step_max.
	void appendTail(std::vector<lbv::Op> &ops) const;
	// The quantities an accepted point needs (post-evaluation / restore).
	void appendPost(std::vector<lbv::Op> &ops) const;
	Status lineSearchStart(bool afterPatho);
	Status trial();
	Status afterEval();
	Status bodyA();
	Status restoreLo();
	Status accept();
	Status direction(bool reset);
	static double quadMin3(double a, double b, double fa, double ga, double fb);
	static double quadMin2(double a, double b, double ga, double gb);
	static double cubicMin(double a, double b, double fa, double fb, double ga, double gb, bool &exists);
	static double stepSelection(double al, double au, double at, double fl, double fu, double ft, double gl,
			double gu, double gt);

	lbv::Vec &v_;
	LbfgsbParams p_;
	uint32_t n_ = 0;
	Phase phase_ = P_NONE;
	Await await_ = A_NONE;
	std::vector<float> sc_ = std::vector<float>(lbv::kScWords, 0.0f);
	std::string err_, reason_;

	// minimize()'s scalars.
	int k_ = 0, nfev_ = 0;
	double fx_ = 0.0, pg_ = 0.0, xnorm_ = 0.0;
	std::vector<double> fxHist_;
	double dg_ = 0.0, stepMax_ = 0.0;

	// The line search's scalars (LineSearchMoreThuente::LineSearch).
	int lsIter_ = -1;
	double step_ = 0.0, fxInit_ = 0.0, dgInit_ = 0.0, testDecr_ = 0.0, testCurv_ = 0.0;
	double iLo_ = 0.0, iHi_ = 0.0, fiLo_ = 0.0, fiHi_ = 0.0, giLo_ = 0.0, giHi_ = 0.0;
	double fxLo_ = 0.0, dgLo_ = 0.0;
	// Where x_lo lives: XP/GP (the start point, before any case 2/3), the
	// current X/G (just set, the copy not yet recorded), or XLO/GLO.
	enum LoAt { LO_START, LO_CURRENT, LO_BUF } lo_ = LO_START;

	int patho_ = 0, cholResets_ = 0, restores_ = 0, extraEvals_ = 0;
};
