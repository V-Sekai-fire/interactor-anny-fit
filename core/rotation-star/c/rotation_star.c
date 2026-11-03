/*
 * The C transcription of the Lean library RotationStar; see rotation_star.h for the map.
 *
 * The part from rs_det9 to rs_align is contract-sinew-align's vendor/sinew-align/sinew_align.c
 * (itself V-Sekai-fire/interactor-gyre's direct transcription of sinew's Align.lean) with only
 * the names changed. The star solver, the completion and the split below it are new here,
 * transcribed from RotationStar.Kabsch, .Completion and .SwingTwist.
 */

#include "rotation_star.h"

#include <math.h>
#include <string.h>

#define RS_EPS 1e-8

static double v3_dot(const double a[3], const double b[3])
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static void v3_cross(const double a[3], const double b[3], double out[3])
{
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

static double v3_norm(const double a[3])
{
    return sqrt(v3_dot(a, a));
}

static void v3_scale(const double a[3], double s, double out[3])
{
    out[0] = a[0] * s;
    out[1] = a[1] * s;
    out[2] = a[2] * s;
}

double rs_det9(const double m[9])
{
    return m[0] * (m[4] * m[8] - m[5] * m[7]) - m[1] * (m[3] * m[8] - m[5] * m[6]) +
           m[2] * (m[3] * m[7] - m[4] * m[6]);
}

void rs_mul9(const double a[9], const double b[9], double out[9])
{
    double c[9];
    for (int r = 0; r < 3; r++) {
        for (int col = 0; col < 3; col++) {
            c[r * 3 + col] = a[r * 3] * b[col] + a[r * 3 + 1] * b[3 + col] +
                              a[r * 3 + 2] * b[6 + col];
        }
    }
    memcpy(out, c, sizeof(c));
}

void rs_transpose9(const double m[9], double out[9])
{
    double o[9];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            o[i * 3 + j] = m[j * 3 + i];
        }
    }
    memcpy(out, o, sizeof(o));
}

void rs_rodrigues(const double a[3], const double b[3], double out_r[9])
{
    double an = v3_norm(a), bn = v3_norm(b);
    double au[3], bu[3];
    v3_scale(a, 1.0 / fmax(an, RS_EPS), au);
    v3_scale(b, 1.0 / fmax(bn, RS_EPS), bu);
    double d = v3_dot(au, bu);
    if (d < -1.0) d = -1.0;
    if (d > 1.0) d = 1.0;

    if (d < -1.0 + 1e-6) {
        double w[3] = { fabs(bu[0]) > 0.6 ? 0.0 : 1.0,
                         fabs(bu[0]) > 0.6 ? 1.0 : 0.0, 0.0 };
        double x0[3];
        v3_cross(bu, w, x0);
        double x[3];
        v3_scale(x0, 1.0 / v3_norm(x0), x);
        double aa[3] = { x[0], x[1], x[2] };
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                out_r[r * 3 + c] = 2.0 * aa[r] * aa[c] - (r == c ? 1.0 : 0.0);
            }
        }
        return;
    }

    double v[3];
    v3_cross(bu, au, v);
    double k[9] = { 0.0, -v[2], v[1], v[2], 0.0, -v[0], -v[1], v[0], 0.0 };
    double kk[9];
    rs_mul9(k, k, kk);
    double f = 1.0 / (1.0 + d);
    for (int i = 0; i < 9; i++) {
        out_r[i] = (i % 4 == 0 ? 1.0 : 0.0) + k[i] + f * kk[i];
    }
}

void rs_ns30(const double h[9], double out_r[9])
{
    double mrs = 0.0;
    for (int r = 0; r < 3; r++) {
        double row_abs_sum = fabs(h[r * 3]) + fabs(h[r * 3 + 1]) + fabs(h[r * 3 + 2]);
        mrs = fmax(mrs, row_abs_sum);
    }
    double r[9];
    for (int i = 0; i < 9; i++) {
        r[i] = h[i] / (mrs + RS_EPS);
    }
    for (int iter = 0; iter < 30; iter++) {
        double rtr[9];
        for (int a = 0; a < 3; a++) {
            for (int b = 0; b < 3; b++) {
                rtr[a * 3 + b] = r[a] * r[b] + r[3 + a] * r[3 + b] + r[6 + a] * r[6 + b];
            }
        }
        double term[9];
        for (int k = 0; k < 9; k++) {
            term[k] = (k % 4 == 0 ? 3.0 : 0.0) - rtr[k];
        }
        double rn[9];
        rs_mul9(r, term, rn);
        for (int k = 0; k < 9; k++) {
            r[k] = rn[k] * 0.5;
        }
    }
    if (rs_det9(r) < 0) {
        r[2] = -r[2];
        r[5] = -r[5];
        r[8] = -r[8];
    }
    memcpy(out_r, r, sizeof(r));
}

void rs_regularize(const double h[9], double out_h[9])
{
    double scale = 0.0;
    for (int r = 0; r < 3; r++) {
        double row_abs_sum = fabs(h[r * 3]) + fabs(h[r * 3 + 1]) + fabs(h[r * 3 + 2]);
        scale = fmax(scale, row_abs_sum);
    }
    if (scale < RS_EPS) scale = RS_EPS;
    double vol = fabs(rs_det9(h)) / (scale * scale * scale);
    double rw = fmax(0.0, fmin(1.0, (1e-6 - vol) / 1e-6));
    double add = 0.05 * rw * scale;
    memcpy(out_h, h, 9 * sizeof(double));
    out_h[0] += add;
    out_h[4] += add;
    out_h[8] += add;
}

int rs_valid9(const double r[9])
{
    double d = rs_det9(r);
    if (!(d > 0.0 && fabs(d - 1.0) <= 1e-2)) {
        return 0;
    }
    double e = 0.0;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            double v = r[i] * r[j] + r[3 + i] * r[3 + j] + r[6 + i] * r[6 + j] -
                       (i == j ? 1.0 : 0.0);
            e = fmax(e, fabs(v));
        }
    }
    return e <= 1e-2;
}

void rs_jacobi9(const double a_in[9], double out_eigenvalues[3], double out_v[9])
{
    double a[9];
    memcpy(a, a_in, sizeof(a));
    double v[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
    const int pp[3] = { 0, 0, 1 };
    const int qq[3] = { 1, 2, 2 };

    for (int iter = 0; iter < 50; iter++) {
        if (fabs(a[1]) + fabs(a[2]) + fabs(a[5]) < 1e-20) {
            break;
        }
        for (int k = 0; k < 3; k++) {
            int p = pp[k], q = qq[k];
            double apq = a[p * 3 + q];
            if (fabs(apq) < 1e-20) {
                continue;
            }
            double phi = 0.5 * atan2(2.0 * apq, a[q * 3 + q] - a[p * 3 + p]);
            double c = cos(phi), s = sin(phi);
            for (int i = 0; i < 3; i++) {
                double aip = a[i * 3 + p], aiq = a[i * 3 + q];
                a[i * 3 + p] = c * aip - s * aiq;
                a[i * 3 + q] = s * aip + c * aiq;
            }
            for (int i = 0; i < 3; i++) {
                double api = a[p * 3 + i], aqi = a[q * 3 + i];
                a[p * 3 + i] = c * api - s * aqi;
                a[q * 3 + i] = s * api + c * aqi;
            }
            for (int i = 0; i < 3; i++) {
                double vip = v[i * 3 + p], viq = v[i * 3 + q];
                v[i * 3 + p] = c * vip - s * viq;
                v[i * 3 + q] = s * vip + c * viq;
            }
        }
    }
    out_eigenvalues[0] = a[0];
    out_eigenvalues[1] = a[4];
    out_eigenvalues[2] = a[8];
    memcpy(out_v, v, sizeof(v));
}

void rs_kabsch_eig(const double h[9], double out_r[9])
{
    double s[9];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            s[i * 3 + j] = h[i] * h[j] + h[3 + i] * h[3 + j] + h[6 + i] * h[6 + j];
        }
    }
    double w[3], vc[9];
    rs_jacobi9(s, w, vc);

    int ord[3] = { 0, 1, 2 };
    for (int a = 0; a < 3; a++) {
        for (int b = a + 1; b < 3; b++) {
            if (w[ord[b]] > w[ord[a]]) {
                int t = ord[a];
                ord[a] = ord[b];
                ord[b] = t;
            }
        }
    }

    double v[9] = { 0 };
    double sig[3];
    for (int c = 0; c < 3; c++) {
        sig[c] = sqrt(fmax(w[ord[c]], 0.0));
        for (int r = 0; r < 3; r++) {
            v[r * 3 + c] = vc[r * 3 + ord[c]];
        }
    }

    double u[9] = { 0 };
    for (int c = 0; c < 3; c++) {
        double hvx = h[0] * v[c] + h[1] * v[3 + c] + h[2] * v[6 + c];
        double hvy = h[3] * v[c] + h[4] * v[3 + c] + h[5] * v[6 + c];
        double hvz = h[6] * v[c] + h[7] * v[3 + c] + h[8] * v[6 + c];
        double sc = sig[c] > 1e-12 ? 1.0 / sig[c] : 0.0;
        u[c] = hvx * sc;
        u[3 + c] = hvy * sc;
        u[6 + c] = hvz * sc;
    }
    for (int c = 0; c < 3; c++) {
        if (sig[c] <= 1e-12) {
            int a = (c + 1) % 3, b = (c + 2) % 3;
            double ua[3] = { u[a], u[3 + a], u[6 + a] };
            double ub[3] = { u[b], u[3 + b], u[6 + b] };
            double uc[3];
            v3_cross(ua, ub, uc);
            u[c] = uc[0];
            u[3 + c] = uc[1];
            u[6 + c] = uc[2];
        }
    }

    double vt[9];
    rs_transpose9(v, vt);
    double uvt[9];
    rs_mul9(u, vt, uvt);
    double sgn = rs_det9(uvt) < 0 ? -1.0 : 1.0;
    double ud[9];
    memcpy(ud, u, sizeof(u));
    ud[2] *= sgn;
    ud[5] *= sgn;
    ud[8] *= sgn;
    rs_mul9(ud, vt, out_r);
}

void rs_finish_align(const double h_in[9], int n,
                         const double a0[3], const double a1[3],
                         const double b0[3], const double b1[3],
                         double out_r[9])
{
    if (n == 1) {
        rs_rodrigues(a0, b0, out_r);
        return;
    }

    double h[9];
    memcpy(h, h_in, sizeof(h));

    double ns[3], nd[3];
    v3_cross(a0, a1, ns);
    v3_cross(b0, b1, nd);
    double ln = v3_norm(ns), ld = v3_norm(nd);
    if (ln > 1e-9 && ld > 1e-9) {
        double vs[3], vd[3];
        v3_scale(ns, v3_norm(a0) / (ln + RS_EPS), vs);
        v3_scale(nd, v3_norm(b0) / (ld + RS_EPS), vd);
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                h[i * 3 + j] += vs[i] * vd[j];
            }
        }
    }

    double h_reg[9];
    rs_regularize(h, h_reg);
    double r[9];
    rs_ns30(h_reg, r);
    if (!rs_valid9(r)) {
        rs_kabsch_eig(h_reg, out_r);
        return;
    }
    memcpy(out_r, r, sizeof(r));
}

void rs_align(const double *targets_xyz, const double *sources_xyz,
                  size_t count, double out_r[9])
{
    double h[9] = { 0 };
    for (size_t p = 0; p < count; p++) {
        const double *a = &targets_xyz[p * 3];
        const double *b = &sources_xyz[p * 3];
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                h[i * 3 + j] += a[i] * b[j];
            }
        }
    }

    static const double zero[3] = { 0, 0, 0 };
    const double *a0 = count > 0 ? &targets_xyz[0] : zero;
    const double *b0 = count > 0 ? &sources_xyz[0] : zero;
    const double *a1 = count > 1 ? &targets_xyz[3] : zero;
    const double *b1 = count > 1 ? &sources_xyz[3] : zero;

    rs_finish_align(h, (int)count, a0, a1, b0, b1, out_r);
}

/* ---- The star solver (RotationStar.Kabsch, .Completion, .SwingTwist) ---------------------- */

/* unitK: divide by the length, or keep a below a length of 1e-12. */
static void unit_k(const double a[3], double out[3])
{
    double l = sqrt(v3_dot(a, a));
    if (l > 1e-12) {
        out[0] = a[0] / l;
        out[1] = a[1] / l;
        out[2] = a[2] / l;
    } else {
        out[0] = a[0];
        out[1] = a[1];
        out[2] = a[2];
    }
}

/* rod k cs sn w = cs w + sn (k x w) + (k.w)(1 - cs) k */
static void rod(const double k[3], double cs, double sn, const double w[3], double out[3])
{
    double kx[3];
    v3_cross(k, w, kx);
    double kw = v3_dot(k, w);
    for (int i = 0; i < 3; i++) {
        out[i] = cs * w[i] + sn * kx[i] + kw * (1.0 - cs) * k[i];
    }
}

void rs_frame_of(const double v0[3], double v1[3], double v2[3])
{
    double ref[3] = { 0.0, 0.0, 0.0 };
    if (fabs(v0[1]) < 0.9) {
        ref[1] = 1.0;
    } else {
        ref[0] = 1.0;
    }
    double t[3];
    v3_cross(v0, ref, t);
    unit_k(t, v1);
    v3_cross(v0, v1, t);
    unit_k(t, v2);
}

/* Godot's Vector3::get_any_perpendicular() direction, normalised. */
static void any_perp(const double u0[3], double out[3])
{
    int px = fabs(u0[0]) <= fabs(u0[1]) && fabs(u0[0]) <= fabs(u0[2]);
    double pr[3] = { px ? 1.0 : 0.0, px ? 0.0 : 1.0, 0.0 };
    double t[3];
    v3_cross(u0, pr, t);
    unit_k(t, out);
}

void rs_canon_completion(const double v0[3], const double u0[3], const double v1[3],
                         const double v2[3], double c1[3], double c2[3])
{
    double ax[3];
    v3_cross(v0, u0, ax);
    double sn = sqrt(v3_dot(ax, ax));
    double cs = v3_dot(v0, u0);
    double m = fmax(sn, 1e-9);
    double k[3] = { ax[0] / m, ax[1] / m, ax[2] / m };
    if (sn < 1e-9) {
        if (cs > 0.0) {
            memcpy(c1, v1, 3 * sizeof(double));
            memcpy(c2, v2, 3 * sizeof(double));
        } else {
            double p1[3], t[3];
            any_perp(u0, p1);
            v3_cross(u0, p1, t);
            memcpy(c1, p1, sizeof(p1));
            unit_k(t, c2);
        }
        return;
    }
    rod(k, cs, sn, v1, c1);
    rod(k, cs, sn, v2, c2);
}

void rs_any_completion(const double u0[3], double c1[3], double c2[3])
{
    rs_frame_of(u0, c1, c2);
}

/* R = U V^T with U = [u0, c1, c2], V = [v0, v1, v2]. */
static void uvt(const double u0[3], const double c1[3], const double c2[3], const double v0[3],
                const double v1[3], const double v2[3], double r[9])
{
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            r[i * 3 + j] = u0[i] * v0[j] + c1[i] * v1[j] + c2[i] * v2[j];
        }
    }
}

/* det as row0 . (row1 x row2), RotationStar.M3.det's order. */
static double det_rows(const double m[9])
{
    double c[3];
    v3_cross(&m[3], &m[6], c);
    return v3_dot(&m[0], c);
}

void rs_star(const double r[9], const double o[3], double pts[21])
{
    memcpy(pts, o, 3 * sizeof(double));
    for (int axis = 0; axis < 3; axis++) {
        for (int i = 0; i < 3; i++) {
            pts[3 * (1 + 2 * axis) + i] = o[i] + r[i * 3 + axis];
            pts[3 * (2 + 2 * axis) + i] = o[i] - r[i * 3 + axis];
        }
    }
}

void rs_cov(const double *a, const double *b, size_t n, double h[9])
{
    double ca[3] = { 0.0, 0.0, 0.0 }, cb[3] = { 0.0, 0.0, 0.0 };
    for (size_t k = 0; k < n; k++) {
        for (int i = 0; i < 3; i++) {
            ca[i] = ca[i] + a[3 * k + i];
            cb[i] = cb[i] + b[3 * k + i];
        }
    }
    for (int i = 0; i < 3; i++) {
        ca[i] = ca[i] / (double)n;
        cb[i] = cb[i] / (double)n;
    }
    for (int k = 0; k < 9; k++) {
        h[k] = 0.0;
    }
    for (size_t k = 0; k < n; k++) {
        double p[3], q[3];
        for (int i = 0; i < 3; i++) {
            p[i] = a[3 * k + i] - ca[i];
            q[i] = b[3 * k + i] - cb[i];
        }
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                h[i * 3 + j] = h[i * 3 + j] + p[i] * q[j];
            }
        }
    }
}

void rs_svd3(const double h[9], int n_sweeps, double u_out[9], double v_out[9], double sig_out[3])
{
    static const int pairs[3][2] = { { 0, 1 }, { 0, 2 }, { 1, 2 } };
    double a[9], v[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
    memcpy(a, h, sizeof(a));
    for (int sweep = 0; sweep < n_sweeps; sweep++) {
        for (int pi = 0; pi < 3; pi++) {
            int p = pairs[pi][0], q = pairs[pi][1];
            double alpha = a[p] * a[p] + a[3 + p] * a[3 + p] + a[6 + p] * a[6 + p];
            double beta = a[q] * a[q] + a[3 + q] * a[3 + q] + a[6 + q] * a[6 + q];
            double gamma = a[p] * a[q] + a[3 + p] * a[3 + q] + a[6 + p] * a[6 + q];
            if (fabs(gamma) > RS_ORTH_TOL * sqrt(alpha * beta)) {
                double zeta = (beta - alpha) / (2.0 * gamma);
                double t = (zeta >= 0.0 ? 1.0 : -1.0) / (fabs(zeta) + sqrt(1.0 + zeta * zeta));
                double c = 1.0 / sqrt(1.0 + t * t);
                double s = c * t;
                for (int k = 0; k < 3; k++) {
                    double ap = a[k * 3 + p], aq = a[k * 3 + q];
                    a[k * 3 + p] = c * ap - s * aq;
                    a[k * 3 + q] = s * ap + c * aq;
                    double vp = v[k * 3 + p], vq = v[k * 3 + q];
                    v[k * 3 + p] = c * vp - s * vq;
                    v[k * 3 + q] = s * vp + c * vq;
                }
            }
        }
    }
    double sig[3];
    for (int c = 0; c < 3; c++) {
        sig[c] = sqrt(a[c] * a[c] + a[3 + c] * a[3 + c] + a[6 + c] * a[6 + c]);
    }
    int ord[3] = { 0, 1, 2 };
    for (int pi = 0; pi < 3; pi++) {
        int i = pairs[pi][0], j = pairs[pi][1];
        if (sig[ord[j]] > sig[ord[i]]) {
            int t = ord[i];
            ord[i] = ord[j];
            ord[j] = t;
        }
    }
    double s[3] = { sig[ord[0]], sig[ord[1]], sig[ord[2]] };
    double eps = RS_RANK_TOL * s[0];
    double ucol[3][3], vcol[3][3];
    for (int c = 0; c < 3; c++) {
        for (int r = 0; r < 3; r++) {
            vcol[c][r] = v[r * 3 + ord[c]];
            ucol[c][r] = s[c] > eps ? a[r * 3 + ord[c]] / s[c] : 0.0;
        }
    }
    if (s[0] <= eps) {
        for (int c = 0; c < 3; c++) {
            for (int r = 0; r < 3; r++) {
                ucol[c][r] = c == r ? 1.0 : 0.0;
            }
        }
    } else if (s[1] <= eps) {
        /* Rank 1: V's null-space columns are replaced by frameOf(v0) (see RotationStar.Kabsch). */
        rs_frame_of(vcol[0], vcol[1], vcol[2]);
        rs_canon_completion(vcol[0], ucol[0], vcol[1], vcol[2], ucol[1], ucol[2]);
    } else if (s[2] <= eps) {
        double t[3];
        v3_cross(ucol[0], ucol[1], t);
        unit_k(t, ucol[2]);
    }
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            u_out[r * 3 + c] = ucol[c][r];
            v_out[r * 3 + c] = vcol[c][r];
        }
    }
    memcpy(sig_out, s, sizeof(s));
}

void rs_kabsch(const double h[9], double r[9])
{
    double u[9], v[9], sig[3];
    rs_svd3(h, RS_SWEEPS, u, v, sig);
    double uc[3][3], vc[3][3];
    for (int c = 0; c < 3; c++) {
        for (int k = 0; k < 3; k++) {
            uc[c][k] = u[k * 3 + c];
            vc[c][k] = v[k * 3 + c];
        }
    }
    double p[9];
    uvt(uc[0], uc[1], uc[2], vc[0], vc[1], vc[2], p);
    double d = det_rows(p) >= 0.0 ? 1.0 : -1.0;
    double u2d[3] = { d * uc[2][0], d * uc[2][1], d * uc[2][2] };
    uvt(uc[0], uc[1], u2d, vc[0], vc[1], vc[2], r);
}

void rs_star_kabsch(const double tgt[21], const double src[21], double r[9])
{
    double h[9];
    rs_cov(tgt, src, 7, h);
    rs_kabsch(h, r);
}

static void swing_twist_with(const double r[9], int ax, int any, double swing[9],
                             double twist[9], double *angle)
{
    double v0[3] = { ax == 0 ? 1.0 : 0.0, ax == 1 ? 1.0 : 0.0, ax == 2 ? 1.0 : 0.0 };
    double v1[3], v2[3], ru[3], u0[3], c1[3], c2[3];
    rs_frame_of(v0, v1, v2);
    for (int i = 0; i < 3; i++) {
        ru[i] = v3_dot(&r[i * 3], v0);
    }
    unit_k(ru, u0);
    if (any) {
        rs_any_completion(u0, c1, c2);
    } else {
        rs_canon_completion(v0, u0, v1, v2, c1, c2);
    }
    uvt(u0, c1, c2, v0, v1, v2, swing);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            twist[i * 3 + j] = swing[0 * 3 + i] * r[0 * 3 + j] + swing[1 * 3 + i] * r[1 * 3 + j] +
                               swing[2 * 3 + i] * r[2 * 3 + j];
        }
    }
    const double *t = twist;
    if (ax == 0) {
        *angle = atan2(t[7] - t[5], t[4] + t[8]);
    } else if (ax == 1) {
        *angle = atan2(t[2] - t[6], t[8] + t[0]);
    } else {
        *angle = atan2(t[3] - t[1], t[0] + t[4]);
    }
}

void rs_swing_twist(const double r[9], int ax, double swing[9], double twist[9], double *angle)
{
    swing_twist_with(r, ax, 0, swing, twist, angle);
}

void rs_swing_twist_any(const double r[9], int ax, double swing[9], double twist[9], double *angle)
{
    swing_twist_with(r, ax, 1, swing, twist, angle);
}
