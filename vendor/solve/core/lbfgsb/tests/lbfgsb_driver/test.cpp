// SPDX-License-Identifier: Apache-2.0 OR MIT
//
// Host-native run of the in-guest L-BFGS-B (guest/drape/lbfgsb.cpp over
// guest/drape/vec_cpu.cpp, the same sources drape.elf compiles) through the
// Gate 5 runners (guest/drape/lbfgsb_gate.cpp): G1 on the 20 component
// fixtures and the corrupted-theta control, G2 on the 20 oracle traces.
// The flat control for the guest's G1/G2: the same code outside the sandbox.
//
//   test.exe <gates/5-drape/oracle>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "lbfgsb_gate.h"
#include "vec_cpu.h"

static std::string slurp(const std::string &p) {
	std::ifstream in(p, std::ios::binary);
	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

int main(int argc, char **argv) {
	if (argc < 2) {
		std::fprintf(stderr, "usage: %s <oracle dir>\n", argv[0]);
		return 2;
	}
	const std::string dir = argv[1];
	int rc = 0;
	VecCpu v;
	for (int corrupt = 0; corrupt < 2; ++corrupt) {
		int passed = 0;
		lbg::Worst w;
		for (int id = 0; id < 20; ++id) {
			char name[32];
			std::snprintf(name, sizeof name, "comp_%02d", id);
			lbg::ComponentRun r(name, slurp(dir + "/components/" + name + ".txt"), corrupt != 0);
			while (r.step(v)) {
			}
			passed += r.pass() ? 1 : 0;
			r.addWorst(w);
			std::printf("%s%s\n", corrupt ? "control " : "", r.line().c_str());
		}
		std::printf("G1 %s: %d/20 pass; worst:%s\n", corrupt ? "control (theta x1.25)" : "fixtures", passed,
				w.table().c_str());
		if (corrupt ? passed != 0 : passed != 20) {
			rc = 1;
		}
	}
	const char *probs[] = { "rosen_n2", "rosen_n10", "rosen_n100", "rosenbox_upstream_n25", "boxqp_n1000" };
	int passed = 0, total = 0;
	for (const char *pn : probs) {
		lbg::Problem p;
		std::string err;
		if (!p.parse(slurp(dir + "/problems/" + pn + ".txt"), err)) {
			std::printf("FAIL %s: %s\n", pn, err.c_str());
			rc = 1;
			continue;
		}
		for (const char *m : { "10", "5" }) {
			for (const char *tag : { "dc", "tight" }) {
				const std::string tn = std::string(pn) + "_m" + m + "_" + tag;
				lbg::Trace t;
				if (!t.parse(slurp(dir + "/traces/" + tn + ".txt"), err)) {
					std::printf("FAIL %s: %s\n", tn.c_str(), err.c_str());
					rc = 1;
					continue;
				}
				lbg::ProblemRun r(tn, p, t, v);
				while (r.step()) {
				}
				++total;
				passed += r.pass() ? 1 : 0;
				std::printf("%s first_f_divergence=%d\n", r.line().c_str(), r.firstDivergence());
				if (!r.pass() || std::getenv("LBFGSB_ITERS")) {
					for (size_t k = 0; k < r.iterF.size(); ++k) {
						std::printf("    iter %2zu f %.12g ref %.12g  pg %.4g ref %.4g\n", k, r.iterF[k],
								k < t.iterF.size() ? t.iterF[k] : 0.0, r.iterPg[k],
								k < t.iterPg.size() ? t.iterPg[k] : 0.0);
					}
				}
			}
		}
	}
	std::printf("G2: %d/%d pass\n", passed, total);
	if (passed != total) {
		rc = 1;
	}
	return rc;
}
