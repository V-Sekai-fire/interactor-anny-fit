// The rotation-star kernels against the Lean oracle, on the CPU (the C++ emission) or on the GPU
// (outputs a windowed Godot RenderingDevice run wrote, see gpu.sh).
//
//   rs_kernel_test <oracle.txt>                 run the C++ emission and gate it
//   rs_kernel_test <oracle.txt> --write <dir>   also write the float32 inputs for gpu_run.gd
//   rs_kernel_test <oracle.txt> --gpu <dir>     gate the GPU's outputs in <dir> instead
//
// The gate: every output entry within TOL of the oracle's double (angles compared modulo 2π).
// TOL = 2e-5: the kernels run in float32 (unit roundoff 2^-24 ≈ 6e-8) through a few hundred
// dependent operations (centroids, covariance, 8 Jacobi sweeps, completion). Measured worst:
// 1.2e-6 on the CPU emission and on the M2 Pro's GPU, so 2e-5 leaves a margin of ~16; the
// controls move outputs by 1e-1 or more.
//
// Controls, each the same gate on a planted error, each of which must FAIL it:
//   star_kabsch: the target star's +x end moved by (0, 0.05, 0.03);
//   swing_twist: the axis index advanced by one (x -> y -> z -> x).
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

extern "C" void star_kabsch_run(uint32_t N, const float *tgt, const float *src, float *rot);
extern "C" void swing_twist_run(uint32_t N, const float *rot, const uint32_t *axis, float *swing,
		float *twist, float *angle);

static const double TOL = 2e-5;

static double hexf(const std::string &t) {
	uint64_t bits = std::strtoull(t.c_str(), nullptr, 16);
	double d;
	std::memcpy(&d, &bits, sizeof d);
	return d;
}

struct Oracle {
	std::vector<float> ktgt, ksrc;
	std::vector<double> krot;
	std::vector<float> srot;
	std::vector<uint32_t> sax;
	std::vector<double> sswing, stwist, sang;
	uint32_t nk = 0, ns = 0;
};

static Oracle load(const char *path) {
	Oracle o;
	std::ifstream f(path);
	if (!f) {
		std::fprintf(stderr, "cannot read %s\n", path);
		std::exit(2);
	}
	std::string line;
	while (std::getline(f, line)) {
		std::istringstream ss(line);
		std::vector<std::string> t;
		for (std::string w; ss >> w;) t.push_back(w);
		if (t.empty()) continue;
		if (t[0] == "K" && t.size() == 52) {
			for (int i = 0; i < 21; i++) o.ktgt.push_back((float)hexf(t[1 + i]));
			for (int i = 0; i < 21; i++) o.ksrc.push_back((float)hexf(t[22 + i]));
			for (int i = 0; i < 9; i++) o.krot.push_back(hexf(t[43 + i]));
			o.nk++;
		} else if (t[0] == "S" && t.size() == 30) {
			for (int i = 0; i < 9; i++) o.srot.push_back((float)hexf(t[1 + i]));
			o.sax.push_back((uint32_t)std::atoi(t[10].c_str()));
			for (int i = 0; i < 9; i++) o.sswing.push_back(hexf(t[11 + i]));
			for (int i = 0; i < 9; i++) o.stwist.push_back(hexf(t[20 + i]));
			o.sang.push_back(hexf(t[29]));
			o.ns++;
		}
	}
	return o;
}

struct Gate {
	const char *name;
	double worst = 0;
	int cases = 0, over = 0;
	void see(double e) {
		cases++;
		if (!(e <= TOL)) over++;
		if (!(e <= worst)) worst = e;
	}
	bool report(bool want_pass) const {
		bool passed = cases > 0 && over == 0;
		bool ok = passed == want_pass;
		std::printf("%s %s: %d cases, %d over %.0e, worst %.3e (%s)\n", ok ? "PASS" : "FAIL", name,
				cases, over, TOL, worst, want_pass ? "gate holds" : "control fails the gate");
		return ok;
	}
};

static double diff(const float *got, const double *want, int n) {
	double e = 0;
	for (int i = 0; i < n; i++) {
		double d = std::fabs((double)got[i] - want[i]);
		if (!(d <= e)) e = d;
	}
	return e;
}

static double angdiff(float got, double want) {
	double d = std::fmod(std::fabs((double)got - want), 2.0 * M_PI);
	return std::fmin(d, 2.0 * M_PI - d);
}

static Gate gate_kabsch(const char *name, const Oracle &o, const std::vector<float> &rot) {
	Gate g{ name };
	for (uint32_t i = 0; i < o.nk; i++) g.see(diff(&rot[9 * i], &o.krot[9 * i], 9));
	return g;
}

static Gate gate_split(const char *name, const Oracle &o, const std::vector<float> &sw,
		const std::vector<float> &tw, const std::vector<float> &an) {
	Gate g{ name };
	for (uint32_t i = 0; i < o.ns; i++) {
		double e = std::fmax(diff(&sw[9 * i], &o.sswing[9 * i], 9), diff(&tw[9 * i], &o.stwist[9 * i], 9));
		g.see(std::fmax(e, angdiff(an[i], o.sang[i])));
	}
	return g;
}

template <class T>
static void wr(const std::string &path, const std::vector<T> &v) {
	FILE *f = std::fopen(path.c_str(), "wb");
	if (!f || std::fwrite(v.data(), sizeof(T), v.size(), f) != v.size()) {
		std::fprintf(stderr, "cannot write %s\n", path.c_str());
		std::exit(2);
	}
	std::fclose(f);
}

static std::vector<float> rd(const std::string &path, size_t n) {
	std::vector<float> v(n);
	FILE *f = std::fopen(path.c_str(), "rb");
	if (!f || std::fread(v.data(), sizeof(float), n, f) != n) {
		std::fprintf(stderr, "cannot read %s (%zu floats)\n", path.c_str(), n);
		std::exit(2);
	}
	std::fclose(f);
	return v;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		std::fprintf(stderr, "usage: %s <oracle.txt> [--write <dir> | --gpu <dir>]\n", argv[0]);
		return 2;
	}
	Oracle o = load(argv[1]);
	std::string mode = argc >= 4 ? argv[2] : "";
	std::string dir = argc >= 4 ? argv[3] : "";

	// The planted inputs.
	std::vector<float> bent = o.ktgt;
	for (uint32_t i = 0; i < o.nk; i++) {
		bent[21 * i + 4] += 0.05f;
		bent[21 * i + 5] += 0.03f;
	}
	std::vector<uint32_t> wrong(o.ns);
	for (uint32_t i = 0; i < o.ns; i++) wrong[i] = (o.sax[i] + 1) % 3;

	std::vector<float> rot(9 * o.nk), rotb(9 * o.nk);
	std::vector<float> sw(9 * o.ns), tw(9 * o.ns), an(o.ns), sww(9 * o.ns), tww(9 * o.ns), anw(o.ns);
	const char *where = "cpu";
	if (mode == "--gpu") {
		where = "gpu";
		rot = rd(dir + "/k_rot.bin", 9 * o.nk);
		rotb = rd(dir + "/k_rot_bent.bin", 9 * o.nk);
		sw = rd(dir + "/s_swing.bin", 9 * o.ns);
		tw = rd(dir + "/s_twist.bin", 9 * o.ns);
		an = rd(dir + "/s_angle.bin", o.ns);
		sww = rd(dir + "/s_swing_wrong.bin", 9 * o.ns);
		tww = rd(dir + "/s_twist_wrong.bin", 9 * o.ns);
		anw = rd(dir + "/s_angle_wrong.bin", o.ns);
	} else {
		star_kabsch_run(o.nk, o.ktgt.data(), o.ksrc.data(), rot.data());
		star_kabsch_run(o.nk, bent.data(), o.ksrc.data(), rotb.data());
		swing_twist_run(o.ns, o.srot.data(), o.sax.data(), sw.data(), tw.data(), an.data());
		swing_twist_run(o.ns, o.srot.data(), wrong.data(), sww.data(), tww.data(), anw.data());
	}
	if (mode == "--write") {
		wr(dir + "/k_tgt.bin", o.ktgt);
		wr(dir + "/k_tgt_bent.bin", bent);
		wr(dir + "/k_src.bin", o.ksrc);
		wr(dir + "/s_rot.bin", o.srot);
		wr(dir + "/s_axis.bin", o.sax);
		wr(dir + "/s_axis_wrong.bin", wrong);
		FILE *f = std::fopen((dir + "/meta.json").c_str(), "w");
		std::fprintf(f, "{\"nk\": %u, \"ns\": %u}\n", o.nk, o.ns);
		std::fclose(f);
	}
	std::printf("== %s ==\n", where);
	bool ok = true;
	ok &= gate_kabsch("star_kabsch vs oracle", o, rot).report(true);
	ok &= gate_split("swing_twist vs oracle", o, sw, tw, an).report(true);
	ok &= gate_kabsch("control: perturbed star", o, rotb).report(false);
	ok &= gate_split("control: axis advanced by one", o, sww, tww, anw).report(false);
	std::printf(ok ? "OK\n" : "FAILED\n");
	return ok ? 0 : 1;
}
