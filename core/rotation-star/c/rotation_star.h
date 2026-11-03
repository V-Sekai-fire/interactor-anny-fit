/*
 * rotation_star: the rotational-star rotation solver in C, a transcription of the Lean library
 * RotationStar (the modules under lean/RotationStar), function for function and in the same operation
 * order, so the C and the Lean oracle agree to a few ulps.
 *
 *   Star solver (RotationStar.Kabsch, after entities-godot ik_kabsch_6d.cpp):
 *     rs_star, rs_cov, rs_svd3 (one-sided Jacobi + rank completions), rs_kabsch,
 *     rs_star_kabsch
 *   Completion (RotationStar.Completion, after swing-twist-kusudama KusKabsch.lean):
 *     rs_frame_of, rs_canon_completion, rs_any_completion (the planted wrong one)
 *   Split (RotationStar.SwingTwist): rs_swing_twist
 *   sinew's align recipe (RotationStar.Align; contract-sinew-align's sinew_align.c, renamed
 *     sinew_* -> rs_*, kabsch -> rs_kabsch_eig): rs_det9, rs_mul9, rs_transpose9,
 *     rs_rodrigues, rs_ns30, rs_regularize, rs_valid9, rs_jacobi9, rs_kabsch_eig,
 *     rs_finish_align, rs_align
 *
 * Matrices are row-major double[9]; vectors double[3]. Rotations stay 3x3 matrices throughout:
 * no quaternion or Euler angle is formed.
 */
#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Jacobi sweeps, orthogonality and rank tolerances: RotationStar.Kabsch (sweeps, orthTol,
 * rankTol), with the reasons stated there. */
#define RS_SWEEPS 8
#define RS_ORTH_TOL 2.220446049250313e-16
#define RS_RANK_TOL 1e-6

/* The 7 points of the frame r (columns = axes) at origin o: o, o+x, o-x, o+y, o-y, o+z, o-z. */
void rs_star(const double r[9], const double o[3], double pts[21]);
/* H = sum (a_i - mean a)(b_i - mean b)^T over n points (a target, b source). */
void rs_cov(const double *a, const double *b, size_t n, double h[9]);
/* H = U diag(sig) V^T, sig descending, U completed where H is rank deficient (rank 1: the
 * canonical minimal-twist completion; rank 2: u2 = u0 x u1). n_sweeps: RS_SWEEPS. */
void rs_svd3(const double h[9], int n_sweeps, double u[9], double v[9], double sig[3]);
/* R = U diag(1, 1, sign det(U V^T)) V^T. */
void rs_kabsch(const double h[9], double r[9]);
/* Kabsch on two stars (21 doubles each): tgt - mean ~= R (src - mean). */
void rs_star_kabsch(const double tgt[21], const double src[21], double r[9]);

void rs_frame_of(const double v0[3], double v1[3], double v2[3]);
void rs_canon_completion(const double v0[3], const double u0[3], const double v1[3],
                         const double v2[3], double c1[3], double c2[3]);
/* The planted wrong completion (a fixed perpendicular frame of u0); for negative controls. */
void rs_any_completion(const double u0[3], double c1[3], double c2[3]);
/* Split r about the coordinate axis ax (0 = x): swing R0, twist T = R0^T r, its angle. */
void rs_swing_twist(const double r[9], int ax, double swing[9], double twist[9], double *angle);
/* The same with the planted wrong completion in the swing; for negative controls. */
void rs_swing_twist_any(const double r[9], int ax, double swing[9], double twist[9], double *angle);

double rs_det9(const double m[9]);
void rs_mul9(const double a[9], const double b[9], double out[9]);
void rs_transpose9(const double m[9], double out[9]);
void rs_rodrigues(const double a[3], const double b[3], double out_r[9]);
void rs_ns30(const double h[9], double out_r[9]);
void rs_regularize(const double h[9], double out_h[9]);
int rs_valid9(const double r[9]);
void rs_jacobi9(const double a_in[9], double out_eigenvalues[3], double out_v[9]);
void rs_kabsch_eig(const double h[9], double out_r[9]);
void rs_finish_align(const double h[9], int n, const double a0[3], const double a1[3],
                     const double b0[3], const double b1[3], double out_r[9]);
/* targets[i] ~= R sources[i] over count pairs. */
void rs_align(const double *targets_xyz, const double *sources_xyz, size_t count, double out_r[9]);

#ifdef __cplusplus
}
#endif
