// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "lbfgsb.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <sstream>

using namespace lbv;

namespace {

constexpr double kInf = std::numeric_limits<double>::infinity();

// The kernels spell an absent bound as FLT_MAX and "no bound along d" as
// FLT_MAX (lb_step_max); anything that large is infinite to the driver.
inline float toBound(float b) {
	if (b >= 1e30f) {
		return FLT_MAX;
	}
	if (b <= -1e30f) {
		return -FLT_MAX;
	}
	return b;
}

} // namespace

// ---------------------------------------------------------------- params

bool LbfgsbParams::parse(const std::string &kv, std::string &err) {
	std::istringstream ss(kv);
	std::string k, v;
	while (ss >> k) {
		if (!(ss >> v)) {
			err = "param " + k + " has no value";
			return false;
		}
		const double d = std::strtod(v.c_str(), nullptr);
		if (k == "m") {
			m = int(d);
		} else if (k == "epsilon") {
			epsilon = d;
		} else if (k == "epsilon_rel") {
			epsilon_rel = d;
		} else if (k == "past") {
			past = int(d);
		} else if (k == "delta") {
			delta = d;
		} else if (k == "max_iterations") {
			max_iterations = int(d);
		} else if (k == "max_submin") {
			max_submin = int(d);
		} else if (k == "max_linesearch") {
			max_linesearch = int(d);
		} else if (k == "min_step") {
			min_step = d;
		} else if (k == "max_step") {
			max_step = d;
		} else if (k == "ftol") {
			ftol = d;
		} else if (k == "wolfe") {
			wolfe = d;
		} else {
			err = "unknown param " + k;
			return false;
		}
	}
	return true;
}

std::string LbfgsbParams::dump() const {
	char b[400];
	std::snprintf(b, sizeof b,
			"m %d epsilon %g epsilon_rel %g past %d delta %g max_iterations %d max_submin %d max_linesearch %d "
			"min_step %g max_step %g ftol %g wolfe %g",
			m, epsilon, epsilon_rel, past, delta, max_iterations, max_submin, max_linesearch, min_step, max_step, ftol,
			wolfe);
	return b;
}

const char *Lbfgsb::status_name(Status s) {
	switch (s) {
		case BUSY:
			return "BUSY";
		case NEED_EVAL:
			return "NEED_EVAL";
		case TRY:
			return "TRY";
		case ACCEPT:
			return "ACCEPT";
		case CONVERGED:
			return "CONVERGED";
		case FAIL:
			return "FAIL";
	}
	return "?";
}

// ---------------------------------------------------------------- plumbing

Lbfgsb::Status Lbfgsb::fail(const std::string &why) {
	err_ = why;
	await_ = A_DONE;
	phase_ = P_NONE;
	return FAIL;
}

Lbfgsb::Status Lbfgsb::submit(Phase ph, const std::vector<Op> &ops) {
	if (!v_.run(ops)) {
		return fail("vec: " + v_.error());
	}
	phase_ = ph;
	return BUSY;
}

bool Lbfgsb::readSc() {
	return v_.read(SC, sc_.data(), kScWords);
}

bool Lbfgsb::readX(std::vector<float> &x) {
	x.resize(n_);
	return v_.read(X, x.data(), n_);
}

bool Lbfgsb::setGradient(const float *g) {
	return v_.upload(G, g, n_);
}

void Lbfgsb::appendTail(std::vector<Op> &ops) const {
	// minimize(): m_xp = x; m_gradp = grad; dg = grad.dot(drt);
	// step_max = max_step_size(x, drt, lb, ub).
	ops.push_back(copy(XP, X));
	ops.push_back(copy(GP, G));
	ops.push_back(dot(G, D, SC_DG));
	ops.push_back(step_max(X, D, SC_SMAX));
}

void Lbfgsb::appendPost(std::vector<Op> &ops) const {
	// proj_grad_norm(x, grad), x.norm(), and for the next add_correction:
	// vecs = x - xp, vecy = grad - gradp and BFGSMat's products (S's and Y's
	// columns against s, then s.y, y.y, s.s: lb_compact's `dots` layout).
	const uint32_t mc = v_.mcap();
	ops.push_back(pg_inf(X, G, SC_PG));
	ops.push_back(dot(X, X, SC_XX));
	ops.push_back(saxpby(SV, X, XP, 1.0f, -1.0f));
	ops.push_back(saxpby(YV, G, GP, 1.0f, -1.0f));
	ops.push_back(multi_dot(SM, mc, SV, DOTS, 0));
	ops.push_back(multi_dot(YM, mc, SV, DOTS, 2 * mc));
	ops.push_back(multi_dot(YV, 1, SV, DOTS, 4 * mc));
	ops.push_back(multi_dot(YV, 1, YV, DOTS, 4 * mc + 2));
	ops.push_back(multi_dot(SV, 1, SV, DOTS, 4 * mc + 4));
}

// ---------------------------------------------------------------- minimize()

Lbfgsb::Status Lbfgsb::start(uint32_t n, const float *x0, const float *lb, const float *ub, const LbfgsbParams &p) {
	p_ = p;
	n_ = n;
	err_.clear();
	reason_.clear();
	k_ = nfev_ = 0;
	patho_ = cholResets_ = restores_ = extraEvals_ = 0;
	fx_ = pg_ = xnorm_ = 0.0;
	await_ = A_NONE;
	phase_ = P_NONE;
	if (n == 0) {
		return fail("n must be positive");
	}
	if (p.m < 1 || uint32_t(p.m) > kMaxHistory) {
		return fail("m must be in 1..16 (the kernels' history cap)");
	}
	if (p.max_submin < 0 || p.max_linesearch < 0 || !(p.ftol > 0 && p.ftol < 0.5) || !(p.wolfe > p.ftol && p.wolfe < 1)) {
		return fail("parameters out of LBFGSpp's range: " + p.dump());
	}
	std::string err;
	if (!v_.setup(n, uint32_t(p.m), err)) {
		return fail(err);
	}
	std::vector<float> l(n), u(n);
	for (uint32_t i = 0; i < n; ++i) {
		l[i] = toBound(lb[i]);
		u[i] = toBound(ub[i]);
		if (l[i] > u[i]) {
			return fail("lb > ub at coordinate " + std::to_string(i));
		}
	}
	if (!v_.upload(LB, l.data(), n) || !v_.upload(UB, u.data(), n) || !v_.upload(X, x0, n)) {
		return fail("vec: " + v_.error());
	}
	fxHist_.assign(size_t(std::max(p.past, 0)), 0.0);
	// force_bounds(x, lb, ub); reset(n).
	return submit(P_INIT, { box_project(X), compact(1) });
}

Lbfgsb::Status Lbfgsb::next(double f) {
	switch (await_) {
		case A_DONE:
			return err_.empty() ? CONVERGED : FAIL;
		case A_EVAL0: {
			// fx = f(x, m_grad); m_projgnorm; m_fx[0] = fx. Then, speculatively,
			// the generalized Cauchy point and xcp - x (the initial direction).
			await_ = A_NONE;
			fx_ = f;
			++nfev_;
			if (!fxHist_.empty()) {
				fxHist_[0] = f;
			}
			return submit(P_X0,
					{ pg_inf(X, G, SC_PG), dot(X, X, SC_XX), cauchy(), saxpby(DCP, XCP, X, 1.0f, -1.0f),
							dot(DCP, DCP, SC_NN) });
		}
		case A_TRIAL: {
			// The line search's fx = f(x, grad); dg = grad.dot(drt), and what an
			// accepted point needs, in the same submit.
			await_ = A_NONE;
			fx_ = f;
			++nfev_;
			std::vector<Op> ops{ dot(G, D, SC_DG) };
			appendPost(ops);
			return submit(P_POST, ops);
		}
		case A_ACCEPT:
			await_ = A_NONE;
			return direction(false);
		case A_NONE:
			break;
	}
	// A phase completed (on the GPU: a tick ago); read it and go on.
	switch (phase_) {
		case P_NONE:
			return fail("next() before start()");
		case P_INIT:
			phase_ = P_NONE;
			await_ = A_EVAL0;
			return NEED_EVAL;
		case P_X0: {
			if (!readSc()) {
				return fail("vec: " + v_.error());
			}
			pg_ = sc_[SC_PG];
			xnorm_ = std::sqrt(std::max(0.0, dotAt(SC_XX)));
			k_ = 1;
			// Early exit if the initial x is already a minimizer (returns 1).
			if (pg_ <= p_.epsilon || pg_ <= p_.epsilon_rel * xnorm_) {
				reason_ = "grad";
				await_ = A_DONE;
				return CONVERGED;
			}
			// m_drt = xcp - x; m_drt.normalize() (Eigen: no-op on a zero vector).
			const double nn = dotAt(SC_NN);
			const float scale = nn > 0.0 ? float(1.0 / std::sqrt(nn)) : 1.0f;
			std::vector<Op> ops{ saxpby(D, DCP, X, scale, 0.0f, true) };
			appendTail(ops);
			return submit(P_PREP, ops);
		}
		case P_DIR: {
			uint32_t st[8] = {};
			if (!readSc() || !v_.read(ST, st, 8)) {
				return fail("vec: " + v_.error());
			}
			if (st[4] == 0u) {
				// T's Cholesky met a non-positive pivot: reset the memory and
				// redo the Cauchy point and direction from this x.
				++cholResets_;
				return direction(true);
			}
			return lineSearchStart(false);
		}
		case P_PREP:
		case P_DIR_RESET:
			if (!readSc()) {
				return fail("vec: " + v_.error());
			}
			return lineSearchStart(false);
		case P_PATHO:
			if (!readSc()) {
				return fail("vec: " + v_.error());
			}
			return lineSearchStart(true);
		case P_TRIAL:
			phase_ = P_NONE;
			await_ = A_TRIAL;
			return TRY;
		case P_POST:
			if (!readSc()) {
				return fail("vec: " + v_.error());
			}
			dg_ = dotAt(SC_DG);
			pg_ = sc_[SC_PG];
			xnorm_ = std::sqrt(std::max(0.0, dotAt(SC_XX)));
			return afterEval();
		case P_RESTORE:
			if (!readSc()) {
				return fail("vec: " + v_.error());
			}
			pg_ = sc_[SC_PG];
			xnorm_ = std::sqrt(std::max(0.0, dotAt(SC_XX)));
			return accept();
	}
	return fail("bad state");
}

Lbfgsb::Status Lbfgsb::direction(bool reset) {
	// End of iteration k: add_correction (lb_compact tests s'y > eps*y'y on
	// the device; lb_ring_store stores only an accepted pair), force_bounds,
	// the Cauchy point, the subspace minimization; then the next iteration's
	// head (xp, gp, g.d, step_max) and xcp - x for the pathological reset.
	std::vector<Op> ops;
	if (reset) {
		ops.push_back(compact(1));
	} else {
		++k_;
		ops.push_back(compact(0));
		ops.push_back(ring_store());
		ops.push_back(box_project(X));
	}
	ops.push_back(cauchy());
	ops.push_back(subspace(uint32_t(p_.max_submin)));
	ops.push_back(saxpby(DCP, XCP, X, 1.0f, -1.0f));
	appendTail(ops);
	return submit(reset ? P_DIR_RESET : P_DIR, ops);
}

Lbfgsb::Status Lbfgsb::lineSearchStart(bool afterPatho) {
	dg_ = dotAt(SC_DG);
	double sm = sc_[SC_SMAX];
	if (sm >= 3.0e38) {
		sm = kInf;
	}
	if (!afterPatho && (dg_ >= 0.0 || sm <= p_.min_step)) {
		// A pathological direction: m_drt = xcp - x, reset the BFGS matrix,
		// recompute dg and step_max.
		++patho_;
		return submit(P_PATHO, { copy(D, DCP), compact(1), dot(G, D, SC_DG), step_max(X, D, SC_SMAX) });
	}
	stepMax_ = std::min(p_.max_step, sm);
	step_ = std::min(1.0, stepMax_);
	// LineSearchMoreThuente::LineSearch's argument checks.
	if (step_ <= 0.0) {
		return fail("'step' must be positive");
	}
	fxInit_ = fx_;
	dgInit_ = dg_;
	if (dgInit_ >= 0.0) {
		return fail("the moving direction does not decrease the objective function value");
	}
	testDecr_ = p_.ftol * dgInit_;
	testCurv_ = -p_.wolfe * dgInit_;
	iLo_ = 0.0;
	iHi_ = kInf;
	fiLo_ = 0.0;
	fiHi_ = kInf;
	giLo_ = (1.0 - p_.ftol) * dgInit_;
	giHi_ = kInf;
	fxLo_ = fxInit_;
	dgLo_ = dgInit_;
	lo_ = LO_START;
	lsIter_ = -1;
	return trial();
}

Lbfgsb::Status Lbfgsb::trial() {
	// x = xp + step * drt, after x_lo takes the current point if the last
	// bracket update said so (LBFGSpp's x_lo.swap(x)).
	std::vector<Op> ops;
	if (lo_ == LO_CURRENT) {
		ops.push_back(copy(XLO, X));
		ops.push_back(copy(GLO, G));
		lo_ = LO_BUF;
	}
	ops.push_back(saxpby(X, D, XP, float(step_), 1.0f, true));
	// In double, LBFGSpp's step_max puts a bound-limited trial exactly on the
	// bound; in float32, xp + step*d can land one ulp past it (mu 0.00999999
	// against lb 0.01 in the sphere demo's first trial), and the objective
	// would be evaluated outside the box. Clamp the trial to the box: a no-op
	// for every interior coordinate.
	ops.push_back(box_project(X));
	return submit(P_TRIAL, ops);
}

Lbfgsb::Status Lbfgsb::afterEval() {
	// Convergence test (the strong Wolfe conditions).
	if (fx_ <= fxInit_ + step_ * testDecr_ && std::fabs(dg_) <= testCurv_) {
		return accept();
	}
	if (lsIter_ >= 0) {
		// Inside the loop, after its evaluation: the step_max exit.
		if (step_ >= stepMax_) {
			const double ftBound = fx_ - fxInit_ - step_ * testDecr_;
			if (ftBound <= fiLo_) {
				return accept();
			}
		}
		++lsIter_;
	} else {
		lsIter_ = 0;
	}
	if (lsIter_ >= p_.max_linesearch) {
		// Out of iterations: the last step if better than I_lo, else I_lo.
		const double ft = fx_ - fxInit_ - step_ * testDecr_;
		if (ft <= fiLo_) {
			return accept();
		}
		if (iLo_ <= 0.0) {
			return fail("the line search routine is unable to sufficiently decrease the function value");
		}
		step_ = iLo_;
		fx_ = fxLo_;
		dg_ = dgLo_;
		return restoreLo();
	}
	return bodyA();
}

Lbfgsb::Status Lbfgsb::bodyA() {
	// One pass of LineSearch's for-loop up to the next evaluation.
	const double ft = fx_ - fxInit_ - step_ * testDecr_;
	const double gt = dg_ - p_.ftol * dgInit_;
	double newStep;
	if (ft > fiLo_) {
		// Case 1: ft > fl.
		newStep = stepSelection(iLo_, iHi_, step_, fiLo_, fiHi_, ft, giLo_, giHi_, gt);
		if (newStep <= p_.min_step) {
			newStep = (iLo_ + step_) / 2.0;
		}
		iHi_ = step_;
		fiHi_ = ft;
		giHi_ = gt;
	} else if (gt * (iLo_ - step_) > 0.0) {
		// Case 2: ft <= fl, gt * (al - at) > 0: extrapolate.
		newStep = std::min(stepMax_, step_ + 1.1 * (step_ - iLo_));
		iLo_ = step_;
		fiLo_ = ft;
		giLo_ = gt;
		lo_ = LO_CURRENT;
		fxLo_ = fx_;
		dgLo_ = dg_;
	} else {
		// Case 3: ft <= fl, gt * (al - at) <= 0.
		newStep = stepSelection(iLo_, iHi_, step_, fiLo_, fiHi_, ft, giLo_, giHi_, gt);
		iHi_ = iLo_;
		fiHi_ = fiLo_;
		giHi_ = giLo_;
		iLo_ = step_;
		fiLo_ = ft;
		giLo_ = gt;
		lo_ = LO_CURRENT;
		fxLo_ = fx_;
		dgLo_ = dg_;
	}
	if (step_ == stepMax_ && newStep >= stepMax_) {
		// x, grad := x_lo, grad_lo; LBFGSpp leaves fx and dg as they are.
		return restoreLo();
	}
	step_ = newStep;
	if (step_ < p_.min_step) {
		return fail("the line search step became smaller than the minimum value allowed");
	}
	if (step_ > p_.max_step) {
		return fail("the line search step became larger than the maximum value allowed");
	}
	++extraEvals_;
	return trial();
}

Lbfgsb::Status Lbfgsb::restoreLo() {
	if (lo_ == LO_CURRENT) {
		// x_lo is the point just evaluated: the post-evaluation quantities stand.
		return accept();
	}
	++restores_;
	const Buf lx = lo_ == LO_START ? XP : XLO;
	const Buf lg = lo_ == LO_START ? GP : GLO;
	std::vector<Op> ops{ copy(X, lx), copy(G, lg) };
	appendPost(ops);
	return submit(P_RESTORE, ops);
}

Lbfgsb::Status Lbfgsb::accept() {
	// minimize() after the line search: the gradient test, the delta test
	// over `past` iterations, the iteration cap.
	if (pg_ <= p_.epsilon || pg_ <= p_.epsilon_rel * xnorm_) {
		reason_ = "grad";
		await_ = A_DONE;
		return CONVERGED;
	}
	if (p_.past > 0) {
		const double fxd = fxHist_[size_t(k_ % p_.past)];
		if (k_ >= p_.past &&
				std::fabs(fxd - fx_) <= p_.delta * std::max(std::max(std::fabs(fx_), std::fabs(fxd)), 1.0)) {
			reason_ = "delta";
			await_ = A_DONE;
			return CONVERGED;
		}
		fxHist_[size_t(k_ % p_.past)] = fx_;
	}
	if (p_.max_iterations != 0 && k_ >= p_.max_iterations) {
		reason_ = "max_iterations";
		await_ = A_DONE;
		return CONVERGED;
	}
	await_ = A_ACCEPT;
	return ACCEPT;
}

// ---------------------------------------------------------------- More-Thuente
// LineSearchMoreThuente.h's static helpers, verbatim in double.

double Lbfgsb::quadMin3(double a, double b, double fa, double ga, double fb) {
	const double ba = b - a;
	const double w = 0.5 * ba * ga / (fa - fb + ba * ga);
	return a + w * ba;
}

double Lbfgsb::quadMin2(double a, double b, double ga, double gb) {
	const double w = ga / (ga - gb);
	return a + w * (b - a);
}

double Lbfgsb::cubicMin(double a, double b, double fa, double fb, double ga, double gb, bool &exists) {
	const double apb = a + b;
	const double ba = b - a;
	const double ba2 = ba * ba;
	const double fba = fb - fa;
	const double gba = gb - ga;
	const double z3 = (ga + gb) * ba - 2.0 * fba;
	const double z2 = 0.5 * (gba * ba2 - 3.0 * apb * z3);
	const double z1 = fba * ba2 - apb * z2 - (a * apb + b * b) * z3;
	const double eps = std::numeric_limits<double>::epsilon();
	if (std::fabs(z3) < eps * std::fabs(z2) || std::fabs(z3) < eps * std::fabs(z1)) {
		exists = (z2 * ba > 0.0);
		return exists ? (-0.5 * z1 / z2) : b;
	}
	const double u = z2 / (3.0 * z3), v = z1 / z2;
	const double vu = v / u;
	exists = (vu <= 1.0);
	if (!exists) {
		return b;
	}
	double r1 = 0.0, r2 = 0.0;
	if (std::fabs(u) >= std::fabs(v)) {
		const double w = 1.0 + std::sqrt(1.0 - vu);
		r1 = -u * w;
		r2 = -v / w;
	} else {
		const double sqrtd = std::sqrt(std::fabs(u)) * std::sqrt(std::fabs(v)) * std::sqrt(1 - u / v);
		r1 = -u - sqrtd;
		r2 = -u + sqrtd;
	}
	return (z3 * ba > 0.0) ? std::max(r1, r2) : std::min(r1, r2);
}

double Lbfgsb::stepSelection(double al, double au, double at, double fl, double fu, double ft, double gl, double gu,
		double gt) {
	if (al == au) {
		return al;
	}
	if (!std::isfinite(ft) || !std::isfinite(gt)) {
		return (al + at) / 2.0;
	}
	bool acExists;
	const double ac = cubicMin(al, at, fl, ft, gl, gt, acExists);
	const double aq = quadMin3(al, at, fl, gl, ft);
	if (ft > fl) {
		if (!acExists) {
			return aq;
		}
		return (std::fabs(ac - al) < std::fabs(aq - al)) ? ac : ((aq + ac) / 2.0);
	}
	const double as = quadMin2(al, at, gl, gt);
	if (gt * gl < 0.0) {
		return (std::fabs(ac - at) >= std::fabs(as - at)) ? ac : as;
	}
	const double deltal = 1.1, deltau = 0.66;
	if (std::fabs(gt) < std::fabs(gl)) {
		const double res = (acExists && (ac - at) * (at - al) > 0.0 && std::fabs(ac - at) < std::fabs(as - at)) ? ac : as;
		return (at > al) ? std::min(at + deltau * (au - at), res) : std::max(at + deltau * (au - at), res);
	}
	if ((!std::isfinite(au)) || (!std::isfinite(fu)) || (!std::isfinite(gu))) {
		return at + deltal * (at - al);
	}
	bool aeExists;
	const double ae = cubicMin(at, au, ft, fu, gt, gu, aeExists);
	return (at > al) ? std::min(at + deltau * (au - at), ae) : std::max(at + deltau * (au - at), ae);
}
