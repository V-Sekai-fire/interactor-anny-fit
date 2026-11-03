// L-BFGS-B oracle generator: host-native LBFGSpp 0.3.0 (unmodified headers),
// writes component fixtures and whole-problem traces as text (%.9g).
// Usage: gen <outdir>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <Eigen/Core>
// Reach LBFGSBSolver's private statics (max_step_size, proj_grad_norm) and
// BFGSMat's ring state without copying them: the headers stay byte-identical.
#define private public
#include <LBFGSB.h>
#undef private

using Vec = Eigen::VectorXd;
using IndexSet = std::vector<int>;
using Solver = LBFGSpp::LBFGSBSolver<double>;
using BMat = LBFGSpp::BFGSMat<double, true>;
static const double INF = std::numeric_limits<double>::infinity();

// ---------------------------------------------------------------- output
static void fnum(FILE* f, double v) {
    if (v == INF) std::fputs(" inf", f);
    else if (v == -INF) std::fputs(" -inf", f);
    else std::fprintf(f, " %.9g", v);
}
static void wvec(FILE* f, const char* key, const Vec& v) {
    std::fprintf(f, "%s %d", key, (int)v.size());
    for (int i = 0; i < v.size(); i++) fnum(f, v[i]);
    std::fputc('\n', f);
}
static void wset(FILE* f, const char* key, const IndexSet& s) {
    std::fprintf(f, "%s %d", key, (int)s.size());
    for (int i : s) std::fprintf(f, " %d", i);
    std::fputc('\n', f);
}
static void wscal(FILE* f, const char* key, double v) {
    std::fprintf(f, "%s", key);
    fnum(f, v);
    std::fputc('\n', f);
}
static IndexSet sorted(IndexSet s) { std::sort(s.begin(), s.end()); return s; }

// ---------------------------------------------------------------- rng
// splitmix64 -> uniform [-1,1); portable, so fixtures can be regenerated anywhere.
struct Rng {
    uint64_t s;
    explicit Rng(uint64_t seed) : s(seed) {}
    uint64_t next() {
        uint64_t z = (s += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }
    double u01() { return (next() >> 11) * (1.0 / 9007199254740992.0); }
    double u() { return 2.0 * u01() - 1.0; }
};
// every input is rounded to float32, so the %.9g text holds it exactly
static double fr(double v) { return std::isinf(v) ? v : (double)(float)v; }

// ---------------------------------------------------------------- params
static LBFGSpp::LBFGSBParam<double> make_param(int m, double delta) {
    LBFGSpp::LBFGSBParam<double> p;  // defaults: eps 1e-5, eps_rel 1e-5, past 1, max_submin 10, ftol 1e-4, wolfe 0.9
    p.m = m;
    p.delta = delta;
    p.max_linesearch = 20;
    return p;
}
static void wparam(FILE* f, const LBFGSpp::LBFGSBParam<double>& p) {
    std::fprintf(f, "param m %d epsilon %.9g epsilon_rel %.9g past %d delta %.9g max_iterations %d max_submin %d "
                    "max_linesearch %d min_step %.9g max_step %.9g ftol %.9g wolfe %.9g\n",
                 p.m, p.epsilon, p.epsilon_rel, p.past, p.delta, p.max_iterations, p.max_submin,
                 p.max_linesearch, p.min_step, p.max_step, p.ftol, p.wolfe);
}

// ================================================================ components
static void component(const std::string& dir, int id) {
    const int ns[3] = {5, 37, 300};
    const int n = ns[id % 3];
    const int m = (id % 4 < 2) ? 10 : 5;
    const int npat[7] = {3, -1, 1, -2, -3, 7, 2};  // -1:m  -2:m+3  -3:2m+1
    int npairs = npat[id % 7];
    if (npairs == -1) npairs = m;
    if (npairs == -2) npairs = m + 3;
    if (npairs == -3) npairs = 2 * m + 1;
    if (id == 19) npairs = 0;  // empty history: theta=1, W empty
    const uint64_t seed = 0x5EED0000ull + (uint64_t)id * 7919ull;
    Rng r(seed);

    // bounds and x: ~15% fixed (lb==ub), ~5% free (inf,inf), ~5% one-sided,
    // rest finite; ~20% of x at lb, ~20% at ub, rest interior.
    Vec lb(n), ub(n), x(n), g(n);
    // ids 3, 9, 15 are "tight boxes": finite bounds, nonzero g, large |g|, so the
    // Cauchy search can cross every breakpoint (the crossed_all branch).
    const bool tight = (id % 6 == 3);
    const double gscale = tight ? 50.0 : (id % 5 == 0) ? 20.0 : (id % 5 == 1 ? 0.2 : 2.0);
    for (int i = 0; i < n; i++) {
        const double c = fr(r.u() * 2.0);
        const double w = fr(0.1 + 1.5 * r.u01());
        double kind = r.u01();
        if (tight && kind >= 0.15 && kind < 0.25) kind = 0.5;
        if (kind < 0.15) { lb[i] = ub[i] = c; }
        else if (kind < 0.20) { lb[i] = -INF; ub[i] = INF; }
        else if (kind < 0.225) { lb[i] = -INF; ub[i] = c; }
        else if (kind < 0.25) { lb[i] = c; ub[i] = INF; }
        else { lb[i] = fr(c - w); ub[i] = fr(c + w); }
        const double pos = r.u01();
        if (lb[i] == ub[i]) x[i] = lb[i];
        else if (pos < 0.2 && std::isfinite(lb[i])) x[i] = lb[i];
        else if (pos < 0.4 && std::isfinite(ub[i])) x[i] = ub[i];
        else {
            const double lo = std::isfinite(lb[i]) ? lb[i] : (std::isfinite(ub[i]) ? ub[i] - 2.0 : -1.0);
            const double hi = std::isfinite(ub[i]) ? ub[i] : lo + 2.0;
            double t = fr(lo + (hi - lo) * (0.05 + 0.9 * r.u01()));
            x[i] = std::min(std::max(t, lb[i]), ub[i]);
        }
        g[i] = fr(gscale * r.u());
        if (r.u01() < 0.03 && !tight) g[i] = 0.0;  // zero-gradient coordinates -> brk = inf
    }

    // Convex quadratic H = diag(d) + u1 u1' + u2 u2', d in [0.5, 5]; y = H s.
    Vec d(n), u1(n), u2(n);
    for (int i = 0; i < n; i++) { d[i] = 0.5 + 4.5 * r.u01(); u1[i] = r.u() / std::sqrt((double)n); u2[i] = r.u() / std::sqrt((double)n); }
    std::vector<Vec> S, Y;
    BMat bfgs;
    bfgs.reset(n, m);
    const double eps = std::numeric_limits<double>::epsilon();
    for (int p = 0; p < npairs; p++) {
        Vec s(n), y(n);
        for (int i = 0; i < n; i++) s[i] = fr(0.3 * r.u());
        Vec hs = d.cwiseProduct(s) + u1 * u1.dot(s) + u2 * u2.dot(s);
        for (int i = 0; i < n; i++) y[i] = fr(hs[i]);
        if (!(s.dot(y) > eps * y.squaredNorm())) throw std::runtime_error("s'y filter would reject a pair");
        S.push_back(s); Y.push_back(y);
        bfgs.add_correction(s, y);
    }
    const int ncorr = bfgs.num_corrections();

    char path[512];
    std::snprintf(path, sizeof path, "%s/comp_%02d.txt", dir.c_str(), id);
    FILE* f = std::fopen(path, "w");
    std::fprintf(f, "# lbfgsb component fixture %02d (LBFGSpp 0.3.0 BFGSMat/Cauchy/SubspaceMin, double)\n", id);
    std::fprintf(f, "seed %llu\nn %d\nm %d\nnpairs %d\nncorr %d\nptr %d\n", (unsigned long long)seed, n, m, npairs, ncorr, bfgs.m_ptr);
    wscal(f, "theta", bfgs.theta());
    wvec(f, "lb", lb); wvec(f, "ub", ub); wvec(f, "x", x); wvec(f, "g", g);
    for (int p = 0; p < npairs; p++) {
        char k[32];
        std::snprintf(k, sizeof k, "s %d slot %d", p, p % m); wvec(f, k, S[p]);
        std::snprintf(k, sizeof k, "y %d slot %d", p, p % m); wvec(f, k, Y[p]);
    }

    // M (2ncorr x 2ncorr) by columns of apply_Mv on unit vectors; then 3 random products
    const int K = 2 * ncorr;
    Eigen::MatrixXd M(K, K);
    for (int j = 0; j < K; j++) {
        Vec e = Vec::Zero(K); e[j] = 1.0;
        Vec c; bfgs.apply_Mv(e, c); M.col(j) = c;
    }
    std::fprintf(f, "M %d %d", K, K);
    for (int i = 0; i < K; i++) for (int j = 0; j < K; j++) fnum(f, M(i, j));
    std::fputc('\n', f);
    for (int t = 0; t < 3; t++) {
        Vec v(K), mv;
        for (int i = 0; i < K; i++) v[i] = fr(r.u());
        bfgs.apply_Mv(v, mv);
        char k[32];
        std::snprintf(k, sizeof k, "mv_in %d", t); wvec(f, k, v);
        std::snprintf(k, sizeof k, "mv_out %d", t); wvec(f, k, mv);
    }

    wscal(f, "pg_inf", Solver::proj_grad_norm(x, g, lb, ub));

    // generalized Cauchy point
    Vec xcp, vecc;
    IndexSet newact, fv;
    LBFGSpp::Cauchy<double>::get_cauchy_point(bfgs, x, g, lb, ub, xcp, vecc, newact, fv);
    IndexSet brk0;  // coordinates with breakpoint 0: neither newly active nor free
    {
        std::vector<char> seen(n, 0);
        for (int i : newact) seen[i] = 1;
        for (int i : fv) seen[i] = 1;
        for (int i = 0; i < n; i++) if (!seen[i]) brk0.push_back(i);
    }
    wvec(f, "xcp", xcp);
    wvec(f, "vecc", vecc);
    wset(f, "brk0", brk0);
    wset(f, "newact", newact);
    wset(f, "newact_sorted", sorted(newact));
    wset(f, "fv", fv);
    wset(f, "fv_sorted", sorted(fv));

    // first-iteration direction (xcp - x, normalized) and its step_max
    Vec drt0 = xcp - x;
    drt0.normalize();
    wvec(f, "drt0", drt0);
    wscal(f, "step_max0", Solver::max_step_size(x, drt0, lb, ub));

    // subspace minimization (max_submin = 10)
    Vec drt;
    LBFGSpp::SubspaceMin<double>::subspace_minimize(bfgs, x, xcp, g, lb, ub, vecc, newact, fv, 10, drt);
    wvec(f, "drt", drt);
    IndexSet sL, sU, sP;
    for (int i : fv) {
        if (drt[i] == lb[i] - x[i]) sL.push_back(i);
        else if (drt[i] == ub[i] - x[i]) sU.push_back(i);
        else sP.push_back(i);
    }
    wset(f, "sub_L", sorted(sL));
    wset(f, "sub_U", sorted(sU));
    wset(f, "sub_P", sorted(sP));
    wscal(f, "dg", g.dot(drt));
    wscal(f, "step_max", Solver::max_step_size(x, drt, lb, ub));
    std::fclose(f);
    std::printf("comp_%02d n=%d m=%d npairs=%d ncorr=%d |newact|=%zu |fv|=%zu |brk0|=%zu sub L/U/P=%zu/%zu/%zu\n",
                id, n, m, npairs, ncorr, newact.size(), fv.size(), brk0.size(), sL.size(), sU.size(), sP.size());
}

// ================================================================ problems
struct Problem {
    std::string name;
    int n;
    Vec lb, ub, x0, xstar;  // xstar empty unless known
    std::function<double(const Vec&, Vec&)> fg;
};

// classic chained Rosenbrock: sum_{i<n-1} 100 (x_{i+1}-x_i^2)^2 + (1-x_i)^2
static double rosen(const Vec& x, Vec& g) {
    const int n = x.size();
    double f = 0.0;
    g.setZero();
    for (int i = 0; i + 1 < n; i++) {
        const double a = x[i + 1] - x[i] * x[i], b = 1.0 - x[i];
        f += 100.0 * a * a + b * b;
        g[i] += -400.0 * x[i] * a - 2.0 * b;
        g[i + 1] += 200.0 * a;
    }
    return f;
}
static Problem make_rosen(int n) {
    Problem p;
    p.name = "rosen_n" + std::to_string(n);
    p.n = n;
    p.lb.resize(n); p.ub.resize(n); p.x0.resize(n);
    for (int i = 0; i < n; i++) {
        p.lb[i] = -1.5; p.ub[i] = 1.5;
        if (i % 3 == 1) p.ub[i] = 0.6;
        if (n >= 10 && i % 5 == 4) { p.lb[i] = -INF; p.ub[i] = INF; }
        p.x0[i] = (i % 2 == 0) ? -1.2 : 1.0;
    }
    if (n >= 10) { p.lb[n / 2] = p.ub[n / 2] = 0.5; }
    p.fg = rosen;
    return p;
}
// LBFGSpp examples/example-rosenbrock-box.cpp (roptim variant), n=25, verbatim setup
static double rosen_roptim(const Vec& x, Vec& grad) {
    const int n = x.size();
    double fx = (x[0] - 1.0) * (x[0] - 1.0);
    grad[0] = 2 * (x[0] - 1) + 16 * (x[0] * x[0] - x[1]) * x[0];
    for (int i = 1; i < n; i++) {
        fx += 4 * std::pow(x[i] - x[i - 1] * x[i - 1], 2);
        if (i == n - 1) grad[i] = 8 * (x[i] - x[i - 1] * x[i - 1]);
        else grad[i] = 8 * (x[i] - x[i - 1] * x[i - 1]) + 16 * (x[i] * x[i] - x[i + 1]) * x[i];
    }
    return fx;
}
static Problem make_upstream() {
    Problem p;
    const int n = 25;
    p.name = "rosenbox_upstream_n25";
    p.n = n;
    p.lb = Vec::Constant(n, 2.0); p.ub = Vec::Constant(n, 4.0);
    p.lb[2] = -INF; p.ub[2] = INF;
    p.x0 = Vec::Constant(n, 3.0);
    p.x0[0] = p.x0[1] = 2.0; p.x0[5] = p.x0[7] = 4.0;
    p.fg = rosen_roptim;
    return p;
}
// box QP, n=1000: f = 0.5 x'Ax - b'x, A tridiagonal (diag d_i, off-diagonal -1),
// constructed from a known KKT point x* with half the coordinates at a bound
// and strict complementarity (|g*_i| >= 0.25). All data are dyadic -> exact.
static Vec qp_d;
static double qp_off = -1.0;
static Vec qp_b;
static Vec qp_Ax(const Vec& x) {
    const int n = x.size();
    Vec y(n);
    for (int i = 0; i < n; i++) {
        double v = qp_d[i] * x[i];
        if (i > 0) v += qp_off * x[i - 1];
        if (i + 1 < n) v += qp_off * x[i + 1];
        y[i] = v;
    }
    return y;
}
static double qp_fg(const Vec& x, Vec& g) {
    Vec ax = qp_Ax(x);
    g = ax - qp_b;
    return 0.5 * x.dot(ax) - qp_b.dot(x);
}
static Problem make_qp() {
    const int n = 1000;
    Problem p;
    p.name = "boxqp_n1000";
    p.n = n;
    p.lb = Vec::Constant(n, -1.0); p.ub = Vec::Constant(n, 1.0);
    p.x0 = Vec::Zero(n);
    qp_d.resize(n);
    Vec xs(n), gs(n);
    for (int i = 0; i < n; i++) {
        qp_d[i] = 2.5 + 0.5 * ((i * 37) % 11);  // 2.5 .. 7.5, diagonally dominant
        const int r = i % 4;
        if (r == 0) { xs[i] = -1.0; gs[i] = 0.25 + 0.25 * (i % 5); }        // at lb, g* > 0
        else if (r == 1) { xs[i] = 1.0; gs[i] = -(0.25 + 0.25 * (i % 3)); } // at ub, g* < 0
        else { xs[i] = (((i * 13) % 17) - 8) / 16.0; gs[i] = 0.0; }          // free, in [-0.5, 0.5]
    }
    qp_b = qp_Ax(xs) - gs;
    p.xstar = xs;
    p.fg = qp_fg;
    return p;
}

static void write_problem(const std::string& dir, const Problem& p) {
    FILE* f = std::fopen((dir + "/" + p.name + ".txt").c_str(), "w");
    std::fprintf(f, "# problem %s\nn %d\n", p.name.c_str(), p.n);
    wvec(f, "lb", p.lb); wvec(f, "ub", p.ub); wvec(f, "x0", p.x0);
    if (p.name == "boxqp_n1000") {
        wvec(f, "A_diag", qp_d);
        wscal(f, "A_offdiag", qp_off);
        wvec(f, "b", qp_b);
        wvec(f, "xstar", p.xstar);
        Vec g(p.n);
        double fs = qp_fg(p.xstar, g);
        wscal(f, "fstar", fs);
        wvec(f, "gstar", g);
    }
    std::fclose(f);
}

// ================================================================ traces
struct Counted {
    const Problem* p;
    int nfev = 0;
    double operator()(const Vec& x, Vec& g) { nfev++; return p->fg(x, g); }
};
struct RunOut { bool ok; std::string err; int ret; Vec x; double fx, pg; int nfev; };
static RunOut run(const Problem& p, LBFGSpp::LBFGSBParam<double> prm, int maxit) {
    prm.max_iterations = maxit;
    Solver solver(prm);
    Counted fun{&p};
    RunOut o;
    o.x = p.x0;
    try {
        o.ret = solver.minimize(fun, o.x, o.fx, p.lb, p.ub);
        o.ok = true;
        o.pg = solver.final_grad_norm();
    } catch (const std::exception& e) {
        o.ok = false; o.err = e.what(); o.ret = -1; o.pg = NAN;
    }
    o.nfev = fun.nfev;
    return o;
}
static void active(const Problem& p, const Vec& x, IndexSet& L, IndexSet& U) {
    L.clear(); U.clear();
    for (int i = 0; i < p.n; i++) {
        const double tl = 1e-9 * std::max(1.0, std::fabs(p.lb[i]));
        const double tu = 1e-9 * std::max(1.0, std::fabs(p.ub[i]));
        if (std::isfinite(p.lb[i]) && x[i] <= p.lb[i] + tl) L.push_back(i);
        else if (std::isfinite(p.ub[i]) && x[i] >= p.ub[i] - tu) U.push_back(i);
    }
}
static void trace(const std::string& dir, const Problem& p, int m, double delta, const char* tag) {
    const auto prm = make_param(m, delta);
    char path[512];
    std::snprintf(path, sizeof path, "%s/%s_m%d_%s.txt", dir.c_str(), p.name.c_str(), m, tag);
    struct Rec { int k, nfev; double f, pg; Vec x; };
    std::vector<Rec> recs;
    // iteration 0: x0 projected into the box, as minimize() does
    {
        Vec x0 = p.x0.cwiseMax(p.lb).cwiseMin(p.ub), g(p.n);
        double f0 = p.fg(x0, g);
        recs.push_back({0, 1, f0, Solver::proj_grad_norm(x0, g, p.lb, p.ub), x0});
    }
    // iterate k = state returned by minimize() with max_iterations = k
    // (deterministic; unmodified solver). Stop when ret < k (converged at k-1)
    // or the solver throws.
    std::string status = "max-cap";
    const int cap = 20000;
    RunOut full = run(p, prm, 0);
    if (full.ok && full.nfev == 1) status = "converged";  // x0 already optimal: no iteration
    else
        for (int k = 1; k <= cap; k++) {
            RunOut o = run(p, prm, k);
            if (!o.ok) { status = "exception: " + o.err; break; }
            if (o.ret < k) { status = "converged"; break; }
            recs.push_back({k, o.nfev, o.fx, o.pg, o.x});
        }
    const int niter = recs.back().k;
    if (full.ok && full.ret != niter) std::printf("WARN %s: full run ret %d != trace %d\n", path, full.ret, niter);
    if (full.ok && (full.x - recs.back().x).cwiseAbs().maxCoeff() != 0.0)
        std::printf("WARN %s: full run x differs from last prefix iterate\n", path);
    std::string reason = status;
    if (status == "converged") {
        const Rec& e = recs.back();
        reason = (e.pg <= prm.epsilon || e.pg <= prm.epsilon_rel * e.x.norm()) ? "converged-grad" : "converged-delta";
    }

    FILE* f = std::fopen(path, "w");
    std::fprintf(f, "# lbfgsb trace %s (LBFGSpp 0.3.0 LBFGSBSolver<double>, MoreThuente)\n", p.name.c_str());
    std::fprintf(f, "problem %s\nn %d\n", p.name.c_str(), p.n);
    wparam(f, prm);
    std::fprintf(f, "niter %d\nnfev %d\nstatus %s\n", full.ok ? full.ret : niter, full.nfev, reason.c_str());
    wscal(f, "f_final", recs.back().f);
    wscal(f, "pg_final", recs.back().pg);
    if (p.xstar.size()) {
        wscal(f, "xstar_err_inf", (recs.back().x - p.xstar).cwiseAbs().maxCoeff());
    }
    for (const Rec& rc : recs) {
        IndexSet L, U;
        active(p, rc.x, L, U);
        std::fprintf(f, "iter %d nfev %d f", rc.k, rc.nfev);
        fnum(f, rc.f);
        std::fputs(" pginf", f);
        fnum(f, rc.pg);
        std::fputc('\n', f);
        wvec(f, "x", rc.x);
        wset(f, "L", L);
        wset(f, "U", U);
    }
    std::fclose(f);
    IndexSet L, U;
    active(p, recs.back().x, L, U);
    std::printf("%-44s niter=%4d nfev=%5d f=%.9g pg=%.3g |L|=%zu |U|=%zu %s\n", path + dir.size() + 1, niter,
                full.nfev, recs.back().f, recs.back().pg, L.size(), U.size(), reason.c_str());
}

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: gen <outdir>\n"); return 2; }
    const std::string out = argv[1];
    for (int id = 0; id < 20; id++) component(out + "/components", id);
    std::vector<Problem> probs = {make_rosen(2), make_rosen(10), make_rosen(100), make_upstream(), make_qp()};
    for (const auto& p : probs) write_problem(out + "/problems", p);
    for (const auto& p : probs)
        for (int m : {10, 5}) {
            trace(out + "/traces", p, m, 1e-3, "dc");
            trace(out + "/traces", p, m, 1e-10, "tight");
        }
    return 0;
}
