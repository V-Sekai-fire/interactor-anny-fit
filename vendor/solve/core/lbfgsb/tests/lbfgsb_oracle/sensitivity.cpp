// SPDX-License-Identifier: Apache-2.0 OR MIT
//
// G2's flat control: LBFGSpp itself (double, unmodified) on each oracle
// problem with its inputs (x0, lb, ub) rounded to float32, which is all the
// in-guest L-BFGS-B can hold. Where this run's f_final already sits outside
// G2's 1e-6 band from the trace, the problem (not the float32 driver) is what
// moves the result: a delta-test stop in mid-descent is a function of the
// path, and the path of these problems amplifies a 1e-8 input rounding.
//
//   sensitivity.exe <gates/5-drape/oracle>     (tests/lbfgsb_oracle/build.sh)

#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

#include <Eigen/Core>
#include <LBFGSB.h>

using Vec = Eigen::VectorXd;

static std::string slurp(const std::string &p) {
	std::ifstream in(p, std::ios::binary);
	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

static std::vector<double> field(const std::string &text, const std::string &key, bool scalar = false) {
	std::istringstream in(text);
	std::string ln;
	while (std::getline(in, ln)) {
		std::istringstream ss(ln);
		std::string k;
		ss >> k;
		if (k != key) {
			continue;
		}
		std::vector<double> v;
		std::string t;
		if (!scalar) {
			ss >> t; // count
		}
		while (ss >> t) {
			v.push_back(std::strtod(t.c_str(), nullptr));
		}
		return v;
	}
	return {};
}

static std::string line(const std::string &text, const std::string &key) {
	std::istringstream in(text);
	std::string ln;
	while (std::getline(in, ln)) {
		if (ln.rfind(key + " ", 0) == 0) {
			return ln.substr(key.size() + 1);
		}
	}
	return "";
}

static double fr(double v) { return std::isinf(v) ? v : double(float(v)); }

int main(int argc, char **argv) {
	const std::string dir = argc > 1 ? argv[1] : ".";
	const char *probs[] = { "rosen_n2", "rosen_n10", "rosen_n100", "rosenbox_upstream_n25", "boxqp_n1000" };
	std::printf("# LBFGSpp (double) from float32-rounded x0/lb/ub vs the trace (exact inputs). Arm inputs: that\n"
			"# only. Arm interface: also f and g evaluated at float32 x, g returned as float32 (the guest view).\n");
	for (const char *pn : probs) {
		const std::string pt = slurp(dir + "/problems/" + pn + ".txt");
		const std::vector<double> lb0 = field(pt, "lb"), ub0 = field(pt, "ub"), x00 = field(pt, "x0");
		const std::vector<double> ad = field(pt, "A_diag"), bb = field(pt, "b");
		const int n = int(x00.size());
		const std::string name = pn;
		std::function<double(const Vec &, Vec &)> fg;
		if (name.rfind("rosenbox", 0) == 0) {
			fg = [n](const Vec &x, Vec &grad) {
				double fx = (x[0] - 1.0) * (x[0] - 1.0);
				grad[0] = 2 * (x[0] - 1) + 16 * (x[0] * x[0] - x[1]) * x[0];
				for (int i = 1; i < n; i++) {
					fx += 4 * std::pow(x[i] - x[i - 1] * x[i - 1], 2);
					grad[i] = (i == n - 1) ? 8 * (x[i] - x[i - 1] * x[i - 1])
										   : 8 * (x[i] - x[i - 1] * x[i - 1]) + 16 * (x[i] * x[i] - x[i + 1]) * x[i];
				}
				return fx;
			};
		} else if (name.rfind("rosen", 0) == 0) {
			fg = [n](const Vec &x, Vec &g) {
				double f = 0.0;
				g.setZero();
				for (int i = 0; i + 1 < n; i++) {
					const double a = x[i + 1] - x[i] * x[i], b = 1.0 - x[i];
					f += 100.0 * a * a + b * b;
					g[i] += -400.0 * x[i] * a - 2.0 * b;
					g[i + 1] += 200.0 * a;
				}
				return f;
			};
		} else {
			fg = [n, ad, bb](const Vec &x, Vec &g) {
				double xax = 0.0, bx = 0.0;
				for (int i = 0; i < n; i++) {
					double ax = ad[i] * x[i] - (i > 0 ? x[i - 1] : 0.0) - (i + 1 < n ? x[i + 1] : 0.0);
					g[i] = ax - bb[i];
					xax += x[i] * ax;
					bx += bb[i] * x[i];
				}
				return 0.5 * xax - bx;
			};
		}
		for (const char *m : { "10", "5" }) {
			for (const char *tag : { "dc", "tight" }) {
				const std::string tn = name + "_m" + m + "_" + tag;
				const std::string tt = slurp(dir + "/traces/" + tn + ".txt");
				LBFGSpp::LBFGSBParam<double> prm;
				std::istringstream ps(line(tt, "param"));
				std::string k;
				double v;
				while (ps >> k >> v) {
					if (k == "m") prm.m = int(v);
					else if (k == "epsilon") prm.epsilon = v;
					else if (k == "epsilon_rel") prm.epsilon_rel = v;
					else if (k == "past") prm.past = int(v);
					else if (k == "delta") prm.delta = v;
					else if (k == "max_iterations") prm.max_iterations = int(v);
					else if (k == "max_submin") prm.max_submin = int(v);
					else if (k == "max_linesearch") prm.max_linesearch = int(v);
					else if (k == "min_step") prm.min_step = v;
					else if (k == "max_step") prm.max_step = v;
					else if (k == "ftol") prm.ftol = v;
					else if (k == "wolfe") prm.wolfe = v;
				}
				const double fref = field(tt, "f_final", true).at(0);
				const int kref = int(field(tt, "niter", true).at(0));
				Vec lb(n), ub(n), x(n);
				for (int i = 0; i < n; i++) {
					lb[i] = fr(lb0[i]);
					ub[i] = fr(ub0[i]);
					x[i] = fr(x00[i]);
				}
				// Arm 2 (the float32 interface): also evaluate f and g at x
				// rounded to float32 and hand g back rounded to float32, which
				// is what the in-guest objective sees (readX, setGradient); the
				// solver's own arithmetic stays LBFGSpp's double.
				const std::function<double(const Vec &, Vec &)> fgF = [&fg, n](const Vec &xx, Vec &g) {
					Vec xf(n);
					for (int i = 0; i < n; i++) {
						xf[i] = double(float(xx[i]));
					}
					const double f = fg(xf, g);
					for (int i = 0; i < n; i++) {
						g[i] = double(float(g[i]));
					}
					return f;
				};
				for (int arm = 0; arm < 2; ++arm) {
					Vec xa = x;
					LBFGSpp::LBFGSBSolver<double> solver(prm);
					double fx = 0.0;
					int it = -1;
					try {
						it = solver.minimize(arm == 0 ? fg : fgF, xa, fx, lb, ub);
					} catch (const std::exception &e) {
						std::printf("%-32s exception %s\n", tn.c_str(), e.what());
						continue;
					}
					const double err = std::fabs(fx - fref), tol = 1e-6 * (1.0 + std::fabs(fref));
					std::printf("%-32s %-9s iters %d/%d f %.9g/%.9g err %.1e (%s 1e-6 abs+rel)\n", tn.c_str(),
							arm == 0 ? "inputs" : "interface", it, kref, fx, fref, err, err <= tol ? "within" : "OUTSIDE");
				}
			}
		}
	}
	return 0;
}
