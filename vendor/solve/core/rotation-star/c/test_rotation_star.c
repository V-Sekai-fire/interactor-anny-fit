/*
 * The C port against the Lean oracle (tests/oracle/oracle.txt, written by
 * `lake exe rotation_star_oracle`), with planted controls.
 *
 *   test_rotation_star <oracle.txt>
 *
 * The gate: every case's C result is within TOL of the oracle's, entry by entry. TOL = 1e-12 is
 * ~4500 ulps of 1.0: the C repeats Lean's double operations in the same order, so the two agree
 * to a few ulps, and anything at 1e-12 is a different computation.
 *
 * Controls, each the same gate run on a planted error, each of which must FAIL the gate:
 *   1. star:  the target star's +x end moved by (0, 0.05, 0.03);
 *   2. split: the swing built with the any-perpendicular completion (rs_swing_twist_any);
 *   3. align: the sources rotated a further 1e-3 rad about z;
 *   4. oracle: one oracle entry changed by 1e-9 (shows TOL can see 1e-9).
 * Plus AlignTest.lean's own criterion: the 120-degree turn recovered to 1e-4 at N = 5, 2, 1.
 */
#include "rotation_star.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOL 1e-12
#define MAX_LINE 65536

static double hexf(const char *tok)
{
    uint64_t bits = strtoull(tok, NULL, 16);
    double d;
    memcpy(&d, &bits, sizeof(d));
    return d;
}

static double maxdiff(const double *a, const double *b, int n)
{
    double e = 0.0;
    for (int i = 0; i < n; i++) {
        double d = fabs(a[i] - b[i]);
        if (!(d <= e)) e = d; /* NaN propagates as a failure */
    }
    return e;
}

typedef struct {
    const char *name;
    double worst;
    int cases;
    int over;
} gate;

static void see(gate *g, double e)
{
    g->cases++;
    if (!(e <= TOL)) g->over++;
    if (!(e <= g->worst)) g->worst = e;
}

static int report(const gate *g, int want_pass)
{
    int passed = g->cases > 0 && g->over == 0;
    int ok = passed == want_pass;
    printf("%s %s: %d cases, %d over %.0e, worst %.3e (%s)\n", ok ? "PASS" : "FAIL", g->name,
           g->cases, g->over, TOL, g->worst, want_pass ? "gate holds" : "control fails the gate");
    return ok;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <oracle.txt>\n", argv[0]);
        return 2;
    }
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 2;
    }
    gate star = { "star kabsch vs oracle", 0, 0, 0 };
    gate split = { "swing-twist vs oracle", 0, 0, 0 };
    gate align = { "sinew align vs oracle", 0, 0, 0 };
    gate c_star = { "control: perturbed star", 0, 0, 0 };
    gate c_split = { "control: any-perpendicular swing", 0, 0, 0 };
    gate c_align = { "control: sources turned 1e-3 rad", 0, 0, 0 };
    gate c_oracle = { "control: oracle entry moved 1e-9", 0, 0, 0 };
    double align_err[3] = { -1, -1, -1 };
    int align_seen = 0;

    static char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        char *toks[1024];
        int nt = 0;
        for (char *t = strtok(line, " \n"); t && nt < 1024; t = strtok(NULL, " \n")) {
            toks[nt++] = t;
        }
        if (nt == 0) continue;
        if (strcmp(toks[0], "K") == 0 && nt == 1 + 21 + 21 + 9) {
            double tgt[21], src[21], want[9], got[9];
            for (int i = 0; i < 21; i++) tgt[i] = hexf(toks[1 + i]);
            for (int i = 0; i < 21; i++) src[i] = hexf(toks[22 + i]);
            for (int i = 0; i < 9; i++) want[i] = hexf(toks[43 + i]);
            rs_star_kabsch(tgt, src, got);
            see(&star, maxdiff(got, want, 9));
            tgt[4] += 0.05;
            tgt[5] += 0.03;
            rs_star_kabsch(tgt, src, got);
            see(&c_star, maxdiff(got, want, 9));
            tgt[4] -= 0.05;
            tgt[5] -= 0.03;
            if (c_oracle.cases == 0) {
                rs_star_kabsch(tgt, src, got);
                want[0] += 1e-9;
                see(&c_oracle, maxdiff(got, want, 9));
            }
        } else if (strcmp(toks[0], "S") == 0 && nt == 1 + 9 + 1 + 9 + 9 + 1) {
            double r[9], want[19], got[19];
            for (int i = 0; i < 9; i++) r[i] = hexf(toks[1 + i]);
            int ax = atoi(toks[10]);
            for (int i = 0; i < 19; i++) want[i] = hexf(toks[11 + i]);
            rs_swing_twist(r, ax, got, got + 9, got + 18);
            see(&split, maxdiff(got, want, 19));
            rs_swing_twist_any(r, ax, got, got + 9, got + 18);
            see(&c_split, maxdiff(got, want, 19));
        } else if (strcmp(toks[0], "A") == 0) {
            int n = atoi(toks[1]);
            if (n < 1 || n > 64 || nt != 2 + 6 * n + 9) {
                fprintf(stderr, "bad A line\n");
                return 2;
            }
            double tgt[192], src[192], want[9], got[9];
            for (int i = 0; i < 3 * n; i++) tgt[i] = hexf(toks[2 + i]);
            for (int i = 0; i < 3 * n; i++) src[i] = hexf(toks[2 + 3 * n + i]);
            for (int i = 0; i < 9; i++) want[i] = hexf(toks[2 + 6 * n + i]);
            rs_align(tgt, src, (size_t)n, got);
            see(&align, maxdiff(got, want, 9));
            /* AlignTest.lean's oracle: the first three A lines are its 120-degree turn at N =
             * 5, 2, 1; the recovered R must map each source onto its target to 1e-4. */
            if (align_seen < 3) {
                double e = 0.0;
                for (int k = 0; k < n; k++) {
                    for (int i = 0; i < 3; i++) {
                        double y = got[i * 3] * src[3 * k] + got[i * 3 + 1] * src[3 * k + 1] +
                                   got[i * 3 + 2] * src[3 * k + 2];
                        double d = fabs(y - tgt[3 * k + i]);
                        if (d > e) e = d;
                    }
                }
                align_err[align_seen] = e;
            }
            align_seen++;
            const double c = cos(1e-3), s = sin(1e-3);
            for (int k = 0; k < n; k++) {
                double x = src[3 * k], y = src[3 * k + 1];
                src[3 * k] = c * x - s * y;
                src[3 * k + 1] = s * x + c * y;
            }
            rs_align(tgt, src, (size_t)n, got);
            see(&c_align, maxdiff(got, want, 9));
        } else {
            fprintf(stderr, "bad oracle line: %s\n", toks[0]);
            return 2;
        }
    }
    fclose(f);

    int ok = 1;
    ok &= report(&star, 1);
    ok &= report(&split, 1);
    ok &= report(&align, 1);
    ok &= report(&c_star, 0);
    ok &= report(&c_split, 0);
    ok &= report(&c_align, 0);
    ok &= report(&c_oracle, 0);
    int at = align_err[0] >= 0 && align_err[0] < 1e-4 && align_err[1] >= 0 && align_err[1] < 1e-4 &&
             align_err[2] >= 0 && align_err[2] < 1e-4;
    printf("%s AlignTest 120-degree turn: N=5 %.2e, N=2 %.2e, N=1 %.2e (< 1e-4)\n",
           at ? "PASS" : "FAIL", align_err[0], align_err[1], align_err[2]);
    ok &= at;
    printf(ok ? "OK\n" : "FAILED\n");
    return ok ? 0 : 1;
}
