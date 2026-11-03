// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "lbfgsb_gate.h"

#include <algorithm>
#include <cfloat>
#include <cstdarg>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <sstream>

using namespace lbv;

namespace lbg {

namespace {

constexpr double kBig = 1e30; // at or beyond: infinite

std::string fmt(const char *f, ...) {
	char b[1024];
	va_list ap;
	va_start(ap, f);
	std::vsnprintf(b, sizeof b, f, ap);
	va_end(ap);
	return b;
}

double num(const std::string &x) {
	return std::strtod(x.c_str(), nullptr); // "inf" / "-inf" included
}

// A fixture's inf bound becomes FLT_MAX: the kernels treat |b| >= 1e30 as absent.
float toF(double v) {
	if (std::isinf(v)) {
		return v > 0 ? FLT_MAX : -FLT_MAX;
	}
	return float(v);
}
std::vector<float> toFV(const std::vector<double> &v) {
	std::vector<float> o;
	o.reserve(v.size());
	for (double d : v) {
		o.push_back(toF(d));
	}
	return o;
}

// max|got - ref| / max|ref|; infinite refs must match as infinite.
double relErr(const std::vector<float> &got, const std::vector<double> &ref, size_t n) {
	if (got.size() < n || ref.size() < n) {
		return INFINITY;
	}
	double nu = 0.0, de = 0.0;
	for (size_t i = 0; i < n; ++i) {
		const bool ri = std::fabs(ref[i]) >= kBig, gi = std::fabs(got[i]) >= kBig;
		if (ri || gi) {
			if (ri != gi || (ref[i] > 0) != (got[i] > 0)) {
				return INFINITY;
			}
			continue;
		}
		nu = std::fmax(nu, std::fabs(got[i] - ref[i]));
		de = std::fmax(de, std::fabs(ref[i]));
	}
	return de > 0.0 ? nu / de : nu;
}
double relScalar(double got, double ref) {
	if (std::fabs(ref) >= kBig || std::fabs(got) >= kBig) {
		return (std::fabs(ref) >= kBig && std::fabs(got) >= kBig && (ref > 0) == (got > 0)) ? 0.0 : INFINITY;
	}
	const double den = std::fabs(ref);
	return den > 0.0 ? std::fabs(got - ref) / den : std::fabs(got - ref);
}
std::set<int> asSet(const std::vector<double> &v) {
	std::set<int> s;
	for (double i : v) {
		s.insert(int(i));
	}
	return s;
}

const std::vector<double> kEmpty;

} // namespace

// ---------------------------------------------------------------- parsing

bool KeyVals::parseFixture(const std::string &text) {
	std::istringstream in(text);
	std::string ln;
	static const char *const scalars[] = { "seed", "n", "m", "npairs", "ncorr", "ptr", "theta", "pg_inf",
		"step_max0", "dg", "step_max", "fstar", "A_offdiag", "niter", "nfev", "f_final", "pg_final",
		"xstar_err_inf" };
	while (std::getline(in, ln)) {
		if (!ln.empty() && ln.back() == '\r') {
			ln.pop_back();
		}
		if (ln.empty() || ln[0] == '#') {
			continue;
		}
		std::istringstream ss(ln);
		std::vector<std::string> tok;
		std::string t;
		while (ss >> t) {
			tok.push_back(t);
		}
		if (tok.empty()) {
			continue;
		}
		const std::string &key = tok[0];
		line[key] = ln.size() > key.size() ? ln.substr(key.size() + 1) : std::string();
		if (key == "s" || key == "y") {
			std::vector<double> v; // s P slot J len v...
			for (size_t i = 5; i < tok.size(); ++i) {
				v.push_back(num(tok[i]));
			}
			(key == "s" ? s : y).push_back(v);
		} else if (key == "M") {
			K = std::atoi(tok.size() > 1 ? tok[1].c_str() : "0");
			for (size_t i = 3; i < tok.size(); ++i) {
				M.push_back(num(tok[i]));
			}
		} else if (key == "mv_in" || key == "mv_out") {
			std::vector<double> v;
			for (size_t i = 3; i < tok.size(); ++i) {
				v.push_back(num(tok[i]));
			}
			(key == "mv_in" ? mvIn : mvOut).push_back(v);
		} else {
			bool sc = false;
			for (const char *s : scalars) {
				sc = sc || key == s;
			}
			std::vector<double> v;
			for (size_t i = sc ? 1 : 2; i < tok.size(); ++i) {
				v.push_back(num(tok[i]));
			}
			vec[key] = v;
		}
	}
	return !vec.empty();
}

double KeyVals::scalar(const std::string &k, double dflt) const {
	auto it = vec.find(k);
	return (it == vec.end() || it->second.empty()) ? dflt : it->second[0];
}

const std::vector<double> &KeyVals::v(const std::string &k) const {
	auto it = vec.find(k);
	return it == vec.end() ? kEmpty : it->second;
}

std::string Worst::table() const {
	std::string s;
	for (const auto &kv : e) {
		s += fmt(" %s=%.2e", kv.first.c_str(), kv.second);
	}
	return s;
}

// ---------------------------------------------------------------- G1

bool ComponentRun::step(Vec &v) {
	if (done_) {
		return false;
	}
	if (stage_ == 0) {
		if (!f_.parseFixture(text_)) {
			line_ = "cannot parse " + name_;
			done_ = true;
			return false;
		}
		n_ = uint32_t(f_.scalar("n"));
		m_ = uint32_t(f_.scalar("m"));
		K_ = 2 * uint32_t(f_.scalar("ncorr"));
		std::string err;
		if (!v.setup(n_, m_, err)) {
			line_ = err;
			done_ = true;
			return false;
		}
		const std::vector<float> lb = toFV(f_.v("lb")), ub = toFV(f_.v("ub")), x = toFV(f_.v("x")),
								 g = toFV(f_.v("g"));
		if (lb.size() != n_ || x.size() != n_ || g.size() != n_ || ub.size() != n_) {
			line_ = "fixture vectors are not of length n";
			done_ = true;
			return false;
		}
		v.upload(LB, lb.data(), n_);
		v.upload(UB, ub.data(), n_);
		v.upload(X, x.data(), n_);
		v.upload(G, g.data(), n_);
		v.upload(XLO, x.data(), n_); // x is in the box: the projection must leave it bit-identical
		v.run({ box_project(XLO), compact(1) });
		stage_ = 1;
		return true;
	}
	if (stage_ == 1) {
		if (pair_ > 0) {
			uint32_t st[8];
			v.read(ST, st, 8);
			curvOk_ = curvOk_ && st[3] == 1u;
		}
		if (pair_ < f_.s.size()) {
			// The driver's add_correction sequence (Lbfgsb::appendPost's
			// products, then lb_compact and lb_ring_store).
			const std::vector<float> s = toFV(f_.s[pair_]), y = toFV(f_.y[pair_]);
			v.upload(SV, s.data(), n_);
			v.upload(YV, y.data(), n_);
			v.run({ multi_dot(SM, m_, SV, DOTS, 0), multi_dot(YM, m_, SV, DOTS, 2 * m_),
					multi_dot(YV, 1, SV, DOTS, 4 * m_), multi_dot(YV, 1, YV, DOTS, 4 * m_ + 2),
					multi_dot(SV, 1, SV, DOTS, 4 * m_ + 4), compact(0), ring_store() });
			++pair_;
			return true;
		}
		stage_ = 2;
		if (corrupt_) {
			// A wrong theta, refactored into T: zeroed products make lb_compact's
			// curvature test fail, so it only rebuilds L, T and the factor.
			float th;
			v.read(BF, &th, 1);
			th *= 1.25f;
			v.upload(BF, &th, 1);
			std::vector<float> z(buf_words(DOTS, n_, m_), 0.0f);
			v.upload(DOTS, z.data(), z.size());
			v.run({ compact(0) });
			return true;
		}
	}
	if (stage_ == 2) {
		std::vector<Op> ops;
		if (K_ > 0) {
			std::vector<float> vin(size_t(K_) * K_ + 3 * size_t(K_), 0.0f);
			for (uint32_t j = 0; j < K_; ++j) {
				vin[size_t(j) * K_ + j] = 1.0f;
			}
			for (size_t t = 0; t < f_.mvIn.size() && t < 3; ++t) {
				const std::vector<float> vi = toFV(f_.mvIn[t]);
				for (uint32_t j = 0; j < K_ && j < vi.size(); ++j) {
					vin[size_t(K_) * K_ + t * K_ + j] = vi[j];
				}
			}
			v.upload(VIN, vin.data(), vin.size());
			for (uint32_t j = 0; j < K_ + 3; ++j) {
				ops.push_back(apply_m(j * K_, j * K_));
			}
		}
		ops.push_back(pg_inf(X, G, SC_PG));
		ops.push_back(cauchy());
		ops.push_back(saxpby(DCP, XCP, X, 1.0f, -1.0f));
		ops.push_back(dot(DCP, DCP, SC_NN));
		v.run(ops);
		stage_ = 3;
		return true;
	}
	if (stage_ == 3) {
		float sc[kScWords];
		v.read(SC, sc, kScWords);
		const double nn = double(sc[SC_NN]) + double(sc[SC_NN + 1]);
		const float scale = nn > 0.0 ? float(1.0 / std::sqrt(nn)) : 1.0f;
		// drt0 into XP (a spare n-vector here), then the subspace step.
		v.run({ saxpby(XP, DCP, X, scale, 0.0f, true), step_max(X, XP, SC_SMAX0), subspace(10), dot(G, D, SC_AUX),
				step_max(X, D, SC_SMAX) });
		stage_ = 4;
		return true;
	}
	check(v);
	done_ = true;
	return false;
}

void ComponentRun::check(Vec &v) {
	struct R {
		int fails = 0;
		std::string text;
		void add(bool ok, const char *what, double e) {
			text += fmt(" %s=%.2e%s", what, e, ok ? "" : "(FAIL)");
			fails += ok ? 0 : 1;
		}
		void flag(bool ok, const char *what) {
			if (!ok) {
				text += std::string(" ") + what + "(FAIL)";
				++fails;
			}
		}
	} r;
	auto rec = [this](const char *k, double e) { err_[k] = e; };
	const uint32_t n = n_;
	uint32_t st[8];
	float theta, sc[kScWords];
	std::vector<float> xlo(n), xcp(n), drt0(n), drt(n), vecc(2 * m_), vout(buf_words(VOUT, n, m_));
	std::vector<uint32_t> iset(8 + 3 * size_t(n));
	bool ok = v.read(ST, st, 8) && v.read(BF, &theta, 1) && v.read(SC, sc, kScWords) && v.read(XLO, xlo.data(), n) &&
			v.read(XCP, xcp.data(), n) && v.read(XP, drt0.data(), n) && v.read(D, drt.data(), n) &&
			v.read(VECC, vecc.data(), vecc.size()) && v.read(ISET, iset.data(), iset.size()) &&
			v.read(VOUT, vout.data(), vout.size());
	if (!ok) {
		line_ = "FAIL " + name_ + " readback: " + v.error();
		pass_ = false;
		return;
	}
	const std::vector<float> x = toFV(f_.v("x")), g = toFV(f_.v("g")), lb = toFV(f_.v("lb")),
							 ub = toFV(f_.v("ub"));
	r.flag(xlo == x, "box_project");
	r.flag(curvOk_, "curvature");
	r.flag(st[0] == uint32_t(f_.scalar("ncorr")) && st[1] == uint32_t(f_.scalar("ptr")), "state");
	r.flag(st[4] == 1u, "chol");
	if (!corrupt_) {
		const double e = relScalar(theta, f_.scalar("theta"));
		r.add(e <= 1e-4, "theta", e);
		rec("theta", e);
	}
	const uint32_t K = K_;
	if (K > 0) {
		std::vector<float> Mg(size_t(K) * K);
		for (uint32_t j = 0; j < K; ++j) {
			for (uint32_t i = 0; i < K; ++i) {
				Mg[size_t(i) * K + j] = vout[size_t(j) * K + i];
			}
		}
		const double eM = relErr(Mg, f_.M, size_t(K) * K);
		r.add(eM <= 1e-4, "M", eM);
		rec("M", eM);
		double eMv = 0.0;
		for (size_t t = 0; t < f_.mvIn.size() && t < 3; ++t) {
			std::vector<float> vo(vout.begin() + size_t(K) * K + t * K, vout.begin() + size_t(K) * K + (t + 1) * K);
			eMv = std::fmax(eMv, relErr(vo, f_.mvOut[t], K));
		}
		r.add(eMv <= 1e-4, "Mv", eMv);
		rec("Mv", eMv);
	}
	const double ePg = relScalar(sc[SC_PG], f_.scalar("pg_inf"));
	r.add(ePg <= 1e-4, "pg_inf", ePg);
	rec("pg_inf", ePg);
	const double eXcp = relErr(xcp, f_.v("xcp"), n);
	r.add(eXcp <= 1e-4, "xcp", eXcp);
	rec("xcp", eXcp);
	if (K > 0) {
		const double eC = relErr(vecc, f_.v("vecc"), K);
		r.add(eC <= 1e-4, "vecc", eC);
		rec("vecc", eC);
	}
	std::set<int> nact, fv, brk0;
	for (uint32_t k = 0; k < iset[0] && k < n; ++k) {
		nact.insert(int(iset[8 + k]));
	}
	for (uint32_t k = 0; k < iset[1] && k < n; ++k) {
		fv.insert(int(iset[8 + n + k]));
	}
	for (uint32_t i = 0; i < n; ++i) {
		if (!nact.count(int(i)) && !fv.count(int(i))) {
			brk0.insert(int(i));
		}
	}
	r.flag(nact == asSet(f_.v("newact_sorted")), "newact");
	r.flag(fv == asSet(f_.v("fv_sorted")), "fv");
	r.flag(brk0 == asSet(f_.v("brk0")), "brk0");
	const double eD0 = relErr(drt0, f_.v("drt0"), n);
	r.add(eD0 <= 1e-4, "drt0", eD0);
	rec("drt0", eD0);
	const double eS0 = relScalar(sc[SC_SMAX0], f_.scalar("step_max0"));
	r.add(eS0 <= 1e-4, "step_max0", eS0);
	rec("step_max0", eS0);
	const double eDrt = relErr(drt, f_.v("drt"), n);
	r.add(eDrt <= 5e-4, "drt", eDrt);
	rec("drt", eDrt);
	std::set<int> sL, sU, sP;
	for (int i : fv) {
		if (drt[i] == lb[i] - x[i]) {
			sL.insert(i);
		} else if (drt[i] == ub[i] - x[i]) {
			sU.insert(i);
		} else {
			sP.insert(i);
		}
	}
	r.flag(sL == asSet(f_.v("sub_L")) && sU == asSet(f_.v("sub_U")) && sP == asSet(f_.v("sub_P")), "subLUP");
	const double eDg = relScalar(double(sc[SC_AUX]) + double(sc[SC_AUX + 1]), f_.scalar("dg"));
	r.add(eDg <= 5e-4, "dg", eDg);
	rec("dg", eDg);
	const double eSm = relScalar(sc[SC_SMAX], f_.scalar("step_max"));
	r.add(eSm <= 5e-4, "step_max", eSm);
	rec("step_max", eSm);
	pass_ = r.fails == 0;
	line_ = fmt("%s %s n=%u m=%u ncorr=%u |newact|=%u |fv|=%u sub_rounds=%u sub_path=%u:", pass_ ? "PASS" : "FAIL",
					name_.c_str(), n, m_, K / 2, iset[0], iset[1], iset[4], iset[5]) +
			r.text;
}

void ComponentRun::addWorst(Worst &w) const {
	for (const auto &kv : err_) {
		w.add(kv.first, kv.second);
	}
}

// ---------------------------------------------------------------- G2

bool Problem::parse(const std::string &text, std::string &err) {
	KeyVals kv;
	if (!kv.parseFixture(text)) {
		err = "empty problem";
		return false;
	}
	std::istringstream hs(text);
	std::string first;
	std::getline(hs, first);
	const size_t at = first.find("problem ");
	name = at == std::string::npos ? "" : first.substr(at + 8);
	while (!name.empty() && (name.back() == '\r' || name.back() == ' ')) {
		name.pop_back();
	}
	n = uint32_t(kv.scalar("n"));
	lb = kv.v("lb");
	ub = kv.v("ub");
	x0 = kv.v("x0");
	aDiag = kv.v("A_diag");
	b = kv.v("b");
	aOff = kv.scalar("A_offdiag", -1.0);
	if (n == 0 || lb.size() != n || ub.size() != n || x0.size() != n) {
		err = "problem " + name + ": n, lb, ub, x0 disagree";
		return false;
	}
	if (name.rfind("boxqp", 0) == 0 && (aDiag.size() != n || b.size() != n)) {
		err = "problem " + name + ": A_diag / b missing";
		return false;
	}
	return true;
}

double Problem::fg(const std::vector<float> &xf, std::vector<double> &g) const {
	const int nn = int(n);
	std::vector<double> x(xf.begin(), xf.end());
	g.assign(n, 0.0);
	if (name.rfind("rosenbox_upstream", 0) == 0) {
		// LBFGSpp examples/example-rosenbrock-box.cpp.
		double fx = (x[0] - 1.0) * (x[0] - 1.0);
		g[0] = 2 * (x[0] - 1) + 16 * (x[0] * x[0] - x[1]) * x[0];
		for (int i = 1; i < nn; i++) {
			fx += 4 * std::pow(x[i] - x[i - 1] * x[i - 1], 2);
			if (i == nn - 1) {
				g[i] = 8 * (x[i] - x[i - 1] * x[i - 1]);
			} else {
				g[i] = 8 * (x[i] - x[i - 1] * x[i - 1]) + 16 * (x[i] * x[i] - x[i + 1]) * x[i];
			}
		}
		return fx;
	}
	if (name.rfind("rosen", 0) == 0) {
		// Chained Rosenbrock.
		double f = 0.0;
		for (int i = 0; i + 1 < nn; i++) {
			const double a = x[i + 1] - x[i] * x[i], bb = 1.0 - x[i];
			f += 100.0 * a * a + bb * bb;
			g[i] += -400.0 * x[i] * a - 2.0 * bb;
			g[i + 1] += 200.0 * a;
		}
		return f;
	}
	// Box QP: 0.5 x'Ax - b'x, A tridiagonal (A_diag, off-diagonal aOff).
	double xax = 0.0, bx = 0.0;
	for (int i = 0; i < nn; i++) {
		double ax = aDiag[i] * x[i];
		if (i > 0) {
			ax += aOff * x[i - 1];
		}
		if (i + 1 < nn) {
			ax += aOff * x[i + 1];
		}
		g[i] = ax - b[i];
		xax += x[i] * ax;
		bx += b[i] * x[i];
	}
	return 0.5 * xax - bx;
}

bool Trace::parse(const std::string &text, std::string &err) {
	std::istringstream in(text);
	std::string ln;
	while (std::getline(in, ln)) {
		if (!ln.empty() && ln.back() == '\r') {
			ln.pop_back();
		}
		if (ln.empty() || ln[0] == '#') {
			continue;
		}
		std::istringstream ss(ln);
		std::string key;
		ss >> key;
		const std::string rest = ln.size() > key.size() ? ln.substr(key.size() + 1) : "";
		if (key == "problem") {
			problem = rest;
		} else if (key == "param") {
			params = rest;
		} else if (key == "niter") {
			niter = std::atoi(rest.c_str());
		} else if (key == "nfev") {
			nfev = std::atoi(rest.c_str());
		} else if (key == "status") {
			status = rest;
		} else if (key == "f_final") {
			fFinal = num(rest);
		} else if (key == "pg_final") {
			pgFinal = num(rest);
		} else if (key == "iter") {
			// iter k nfev N f F pginf P
			std::string t;
			double vals[4] = {};
			for (int i = 0; i < 7 && (ss >> t); ++i) {
				if (i == 4) {
					vals[0] = num(t);
				} else if (i == 6) {
					vals[1] = num(t);
				}
			}
			iterF.push_back(vals[0]);
			iterPg.push_back(vals[1]);
		} else if (key == "x" || key == "L" || key == "U") {
			int c = 0;
			ss >> c;
			std::vector<double> v;
			std::string t;
			while (ss >> t) {
				v.push_back(num(t));
			}
			if (key == "x") {
				x = v;
			} else {
				std::vector<int> &s = key == "L" ? L : U;
				s.clear();
				for (double d : v) {
					s.push_back(int(d));
				}
			}
		}
	}
	if (problem.empty() || params.empty() || x.empty()) {
		err = "trace without problem / param / iterates";
		return false;
	}
	return true;
}

bool ProblemRun::step() {
	if (done_) {
		return false;
	}
	Lbfgsb::Status s;
	if (!started_) {
		started_ = true;
		LbfgsbParams prm;
		std::string err;
		if (!prm.parse(t_.params, err)) {
			line_ = "FAIL " + name_ + ": " + err;
			done_ = true;
			return false;
		}
		const std::vector<float> x0 = toFV(p_.x0), lb = toFV(p_.lb), ub = toFV(p_.ub);
		s = drv_.start(p_.n, x0.data(), lb.data(), ub.data(), prm);
	} else {
		s = drv_.next();
	}
	for (;;) {
		switch (s) {
			case Lbfgsb::BUSY:
				return true;
			case Lbfgsb::NEED_EVAL:
			case Lbfgsb::TRY: {
				if (!drv_.readX(x_)) {
					line_ = "FAIL " + name_ + ": readX: " + v_.error();
					done_ = true;
					return false;
				}
				const double f = p_.fg(x_, g_);
				if (s == Lbfgsb::NEED_EVAL) {
					iterF.assign(1, f);
					iterPg.assign(1, 0.0);
				}
				gf_.assign(g_.begin(), g_.end());
				if (!drv_.setGradient(gf_.data())) {
					line_ = "FAIL " + name_ + ": setGradient: " + v_.error();
					done_ = true;
					return false;
				}
				s = drv_.next(f);
				break;
			}
			case Lbfgsb::ACCEPT:
				iterF.push_back(drv_.fx());
				iterPg.push_back(drv_.pgNorm());
				s = drv_.next();
				break;
			case Lbfgsb::CONVERGED:
				iterF.push_back(drv_.fx());
				iterPg.push_back(drv_.pgNorm());
				finish(true);
				return false;
			case Lbfgsb::FAIL:
				finish(false);
				return false;
		}
	}
}

int ProblemRun::firstDivergence() const {
	for (size_t k = 0; k < iterF.size() && k < t_.iterF.size(); ++k) {
		if (std::fabs(iterF[k] - t_.iterF[k]) > 1e-6 * (1.0 + std::fabs(t_.iterF[k]))) {
			return int(k);
		}
	}
	return -1;
}

void ProblemRun::finish(bool converged) {
	done_ = true;
	if (!converged) {
		line_ = fmt("FAIL %-32s driver: %s (after %d iterations, %d evaluations)", name_.c_str(),
				drv_.error().c_str(), drv_.iterations(), drv_.nfev());
		return;
	}
	std::vector<float> x;
	drv_.readX(x);
	std::vector<double> g;
	const double f = drv_.fx();
	const double fAtX = p_.fg(x, g);
	// f: |f - fref| <= 1e-6 (1 + |fref|).
	const double fErr = std::fabs(f - t_.fFinal);
	const double fTol = 1e-6 * (1.0 + std::fabs(t_.fFinal));
	// x: |x - xref|_inf <= 1e-3 |xref|_inf.
	double dx = 0.0, xr = 0.0;
	for (uint32_t i = 0; i < p_.n && i < t_.x.size(); ++i) {
		dx = std::max(dx, std::fabs(double(x[i]) - t_.x[i]));
		xr = std::max(xr, std::fabs(t_.x[i]));
	}
	const double xRel = xr > 0 ? dx / xr : dx;
	// Active sets: the trace's rule is 1e-9 max(1, |b|) in double; ours is
	// float32, whose steps that end at step_max sit up to an ulp or so off the
	// bound, so 1e-6 max(1, |b|).
	std::vector<int> L, U;
	for (uint32_t i = 0; i < p_.n; ++i) {
		const double l = p_.lb[i], u = p_.ub[i];
		if (std::isfinite(l) && x[i] <= l + 1e-6 * std::max(1.0, std::fabs(l))) {
			L.push_back(int(i));
		} else if (std::isfinite(u) && x[i] >= u - 1e-6 * std::max(1.0, std::fabs(u))) {
			U.push_back(int(i));
		}
	}
	const bool setsOk = L == t_.L && U == t_.U;
	const int dIt = std::abs(drv_.iterations() - t_.niter);
	const double itTol = std::max(2.0, 0.25 * t_.niter);
	fRatio = fErr / fTol;
	xRatio = xRel / 1e-3;
	itRatio = dIt / itTol;
	const bool fOk = fErr <= fTol, xOk = xRel <= 1e-3, itOk = dIt <= itTol;
	pass_ = fOk && xOk && setsOk && itOk;
	line_ = fmt("%s %-32s iters %d/%d%s nfev %d/%d f %.9g/%.9g (err %.1e%s) x rel %.1e%s |L| %zu/%zu |U| %zu/%zu%s "
				"stop %s/%s pg %.2e resets patho=%d chol=%d restores=%d",
			pass_ ? "PASS" : "FAIL", name_.c_str(), drv_.iterations(), t_.niter, itOk ? "" : "(FAIL)", drv_.nfev(),
			t_.nfev, f, t_.fFinal, fErr, fOk ? "" : "(FAIL)", xRel, xOk ? "" : "(FAIL)", L.size(), t_.L.size(),
			U.size(), t_.U.size(), setsOk ? "" : "(FAIL)", drv_.reason().c_str(), t_.status.c_str(), drv_.pgNorm(),
			drv_.pathological(), drv_.cholResets(), drv_.restores());
	(void)fAtX;
}

} // namespace lbg
