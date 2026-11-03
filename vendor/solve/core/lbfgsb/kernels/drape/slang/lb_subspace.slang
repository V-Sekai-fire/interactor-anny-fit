struct LbSubspaceParams {
  uint n;
  uint mcap;
  uint maxit;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LbSubspaceParams> params;
[[vk::binding(1, 0)]]
StructuredBuffer<float> x;
[[vk::binding(2, 0)]]
StructuredBuffer<float> g;
[[vk::binding(3, 0)]]
StructuredBuffer<float> lb;
[[vk::binding(4, 0)]]
StructuredBuffer<float> ub;
[[vk::binding(5, 0)]]
StructuredBuffer<float> S;
[[vk::binding(6, 0)]]
StructuredBuffer<float> Y;
[[vk::binding(7, 0)]]
StructuredBuffer<uint> st;
[[vk::binding(8, 0)]]
StructuredBuffer<float> bf;
[[vk::binding(9, 0)]]
StructuredBuffer<float> xcp;
[[vk::binding(10, 0)]]
StructuredBuffer<float> vecc;
[[vk::binding(11, 0)]]
RWStructuredBuffer<uint> iset;
[[vk::binding(12, 0)]]
RWStructuredBuffer<float> drt;
[[vk::binding(13, 0)]]
RWStructuredBuffer<float> wk;

void two_sum(float a, float b, out float hi, out float lo) {
  float h = (a + b);
  float bb = (h - a);
  float ah = (h - bb);
  float lo_a = (a - ah);
  float lo_b = (b - bb);
  hi = h;
  lo = (lo_a + lo_b);
  return;
}

void quick_two_sum(float a, float b, out float hi, out float lo) {
  float h = (a + b);
  float t = (h - a);
  hi = h;
  lo = (b - t);
  return;
}

void two_prod(float a, float b, out float hi, out float lo) {
  float h = (a * b);
  hi = h;
  lo = fma(a, b, (-h));
  return;
}

void df_add(float x_hi, float x_lo, float y_hi, float y_lo, out float z_hi, out float z_lo) {
  float sh;
  float sl;
  two_sum(x_hi, y_hi, sh, sl);
  float xy_lo = (x_lo + y_lo);
  float sl2 = (sl + xy_lo);
  quick_two_sum(sh, sl2, z_hi, z_lo);
  return;
}

void df_acc(inout float hi, inout float lo, float a, float b) {
  float p_hi;
  float p_lo;
  two_prod(a, b, p_hi, p_lo);
  float n_hi;
  float n_lo;
  df_add(hi, lo, p_hi, p_lo, n_hi, n_lo);
  hi = n_hi;
  lo = n_lo;
  return;
}

void lb_mv(uint nc, uint mc, inout float w[32]) {
  float r[16];
  for (uint i = 0u; i < nc; ++i) {
    float acc = w[(nc + i)];
    for (uint j = 0u; j < nc; ++j) {
      acc = (acc + ((bf[(((4u + ((2u * mc) * mc)) + (i * mc)) + j)] * w[j]) / bf[((4u + ((4u * mc) * mc)) + j)]));
    }
    r[i] = acc;
  }
  for (uint fi = 0u; fi < nc; ++fi) {
    float fr = r[fi];
    for (uint ft = 0u; ft < fi; ++ft) {
      fr = (fr - (bf[(((4u + ((3u * mc) * mc)) + (fi * mc)) + ft)] * r[ft]));
    }
    r[fi] = (fr / bf[(((4u + ((3u * mc) * mc)) + (fi * mc)) + fi)]);
  }
  for (uint bq = 0u; bq < nc; ++bq) {
    uint bi = ((nc - 1u) - bq);
    float br = r[bi];
    for (uint bt = (bi + 1u); bt < nc; ++bt) {
      br = (br - (bf[(((4u + ((3u * mc) * mc)) + (bt * mc)) + bi)] * r[bt]));
    }
    r[bi] = (br / bf[(((4u + ((3u * mc) * mc)) + (bi * mc)) + bi)]);
  }
  for (uint j = 0u; j < nc; ++j) {
    float acc = (-w[j]);
    for (uint i = 0u; i < nc; ++i) {
      acc = (acc + (bf[(((4u + ((2u * mc) * mc)) + (i * mc)) + j)] * r[i]));
    }
    w[j] = (acc / bf[((4u + ((4u * mc) * mc)) + j)]);
  }
  for (uint i = 0u; i < nc; ++i) {
    w[(nc + i)] = r[i];
  }
  return;
}

uint lb_solve_p(uint n, uint mc, uint nc, uint nfree, float theta) {
  uint ok = 1u;
  uint nP = 0u;
  for (uint k = 0u; k < nfree; ++k) {
    if ((wk[((7u * n) + k)] == 2.000000)) {
      nP = (nP + 1u);
    }
  }
  if (((nc < 1u) || (nP < 1u))) {
    for (uint k = 0u; k < nfree; ++k) {
      if ((wk[((7u * n) + k)] == 2.000000)) {
        wk[((3u * n) + k)] = (wk[((8u * n) + k)] / theta);
      }
    }
    return ok;
  }
  uint oldest = (((st[1u] + mc) - nc) % mc);
  uint rk[16];
  for (uint i = 0u; i < nc; ++i) {
    rk[i] = (((i + mc) - oldest) % mc);
  }
  for (uint c = 0u; c < n; ++c) {
    wk[((9u * n) + c)] = 0.000000;
  }
  for (uint k = 0u; k < nfree; ++k) {
    if ((wk[((7u * n) + k)] == 2.000000)) {
      wk[((9u * n) + iset[((8u + n) + k)])] = 1.000000;
    }
  }
  float A11[256];
  float A21[256];
  float A22[256];
  float Z[256];
  float r1[16];
  float qv[16];
  float r2[16];
  float tv[16];
  for (uint i = 0u; i < nc; ++i) {
    r1[i] = 0.000000;
    r2[i] = 0.000000;
    for (uint j = 0u; j < nc; ++j) {
      A11[((i * 16u) + j)] = 0.000000;
      A21[((i * 16u) + j)] = 0.000000;
      A22[((i * 16u) + j)] = 0.000000;
    }
  }
  for (uint c = 0u; c < n; ++c) {
    if ((wk[((9u * n) + c)] == 1.000000)) {
      for (uint i = 0u; i < nc; ++i) {
        float yi = Y[((i * n) + c)];
        float si = S[((i * n) + c)];
        for (uint j = 0u; j < nc; ++j) {
          float yj = Y[((j * n) + c)];
          A11[((i * 16u) + j)] = (A11[((i * 16u) + j)] + (yi * yj));
          if ((rk[i] <= rk[j])) {
            A21[((i * 16u) + j)] = (A21[((i * 16u) + j)] - (si * yj));
          }
        }
      }
    } else {
      for (uint i = 0u; i < nc; ++i) {
        float si = S[((i * n) + c)];
        for (uint j = 0u; j < nc; ++j) {
          A22[((i * 16u) + j)] = (A22[((i * 16u) + j)] + (si * S[((j * n) + c)]));
          if ((rk[i] > rk[j])) {
            A21[((i * 16u) + j)] = (A21[((i * 16u) + j)] + (si * Y[((j * n) + c)]));
          }
        }
      }
    }
  }
  for (uint i = 0u; i < nc; ++i) {
    for (uint j = 0u; j < nc; ++j) {
      A11[((i * 16u) + j)] = (A11[((i * 16u) + j)] / theta);
      A22[((i * 16u) + j)] = (theta * A22[((i * 16u) + j)]);
    }
    A11[((i * 16u) + i)] = (A11[((i * 16u) + i)] + bf[((4u + ((4u * mc) * mc)) + i)]);
  }
  for (uint k = 0u; k < nfree; ++k) {
    if ((wk[((7u * n) + k)] == 2.000000)) {
      uint c = iset[((8u + n) + k)];
      float rv = wk[((8u * n) + k)];
      for (uint j = 0u; j < nc; ++j) {
        r1[j] = (r1[j] + (Y[((j * n) + c)] * rv));
        r2[j] = (r2[j] + (S[((j * n) + c)] * rv));
      }
    }
  }
  for (uint j = 0u; j < nc; ++j) {
    r2[j] = (theta * r2[j]);
  }
  for (uint pj = 0u; pj < nc; ++pj) {
    float ps = A11[((pj * 16u) + pj)];
    for (uint pt = 0u; pt < pj; ++pt) {
      ps = (ps - (A11[((pj * 16u) + pt)] * A11[((pj * 16u) + pt)]));
    }
    if ((ps <= 0.000000)) {
      ok = 0u;
      ps = (asfloat(872415232u) * asfloat(872415232u));
    }
    float pd = sqrt(ps);
    A11[((pj * 16u) + pj)] = pd;
    for (uint pi = (pj + 1u); pi < nc; ++pi) {
      float pr = A11[((pi * 16u) + pj)];
      for (uint pt = 0u; pt < pj; ++pt) {
        pr = (pr - (A11[((pi * 16u) + pt)] * A11[((pj * 16u) + pt)]));
      }
      A11[((pi * 16u) + pj)] = (pr / pd);
    }
  }
  for (uint i = 0u; i < nc; ++i) {
    for (uint j = 0u; j < nc; ++j) {
      Z[((i * 16u) + j)] = A21[((i * 16u) + j)];
    }
    for (uint zi = 0u; zi < nc; ++zi) {
      float zr = Z[((i * 16u) + zi)];
      for (uint zt = 0u; zt < zi; ++zt) {
        zr = (zr - (A11[((zi * 16u) + zt)] * Z[((i * 16u) + zt)]));
      }
      Z[((i * 16u) + zi)] = (zr / A11[((zi * 16u) + zi)]);
    }
  }
  for (uint i = 0u; i < nc; ++i) {
    for (uint k = 0u; k < (i + 1u); ++k) {
      float acc = A22[((i * 16u) + k)];
      for (uint j = 0u; j < nc; ++j) {
        acc = (acc + (Z[((i * 16u) + j)] * Z[((k * 16u) + j)]));
      }
      A22[((i * 16u) + k)] = acc;
    }
  }
  for (uint j = 0u; j < nc; ++j) {
    qv[j] = r1[j];
  }
  for (uint qi = 0u; qi < nc; ++qi) {
    float qr = qv[qi];
    for (uint qt = 0u; qt < qi; ++qt) {
      qr = (qr - (A11[((qi * 16u) + qt)] * qv[qt]));
    }
    qv[qi] = (qr / A11[((qi * 16u) + qi)]);
  }
  for (uint i = 0u; i < nc; ++i) {
    float acc = r2[i];
    for (uint j = 0u; j < nc; ++j) {
      acc = (acc + (Z[((i * 16u) + j)] * qv[j]));
    }
    r2[i] = acc;
  }
  for (uint sj = 0u; sj < nc; ++sj) {
    float ss = A22[((sj * 16u) + sj)];
    for (uint st = 0u; st < sj; ++st) {
      ss = (ss - (A22[((sj * 16u) + st)] * A22[((sj * 16u) + st)]));
    }
    if ((ss <= 0.000000)) {
      ok = 0u;
      ss = (asfloat(872415232u) * asfloat(872415232u));
    }
    float sd = sqrt(ss);
    A22[((sj * 16u) + sj)] = sd;
    for (uint si = (sj + 1u); si < nc; ++si) {
      float sr = A22[((si * 16u) + sj)];
      for (uint st = 0u; st < sj; ++st) {
        sr = (sr - (A22[((si * 16u) + st)] * A22[((sj * 16u) + st)]));
      }
      A22[((si * 16u) + sj)] = (sr / sd);
    }
  }
  for (uint sfi = 0u; sfi < nc; ++sfi) {
    float sfr = r2[sfi];
    for (uint sft = 0u; sft < sfi; ++sft) {
      sfr = (sfr - (A22[((sfi * 16u) + sft)] * r2[sft]));
    }
    r2[sfi] = (sfr / A22[((sfi * 16u) + sfi)]);
  }
  for (uint sbq = 0u; sbq < nc; ++sbq) {
    uint sbi = ((nc - 1u) - sbq);
    float sbr = r2[sbi];
    for (uint sbt = (sbi + 1u); sbt < nc; ++sbt) {
      sbr = (sbr - (A22[((sbt * 16u) + sbi)] * r2[sbt]));
    }
    r2[sbi] = (sbr / A22[((sbi * 16u) + sbi)]);
  }
  for (uint j = 0u; j < nc; ++j) {
    float acc = (-r1[j]);
    for (uint i = 0u; i < nc; ++i) {
      acc = (acc + (A21[((i * 16u) + j)] * r2[i]));
    }
    tv[j] = acc;
  }
  for (uint tfi = 0u; tfi < nc; ++tfi) {
    float tfr = tv[tfi];
    for (uint tft = 0u; tft < tfi; ++tft) {
      tfr = (tfr - (A11[((tfi * 16u) + tft)] * tv[tft]));
    }
    tv[tfi] = (tfr / A11[((tfi * 16u) + tfi)]);
  }
  for (uint tbq = 0u; tbq < nc; ++tbq) {
    uint tbi = ((nc - 1u) - tbq);
    float tbr = tv[tbi];
    for (uint tbt = (tbi + 1u); tbt < nc; ++tbt) {
      tbr = (tbr - (A11[((tbt * 16u) + tbi)] * tv[tbt]));
    }
    tv[tbi] = (tbr / A11[((tbi * 16u) + tbi)]);
  }
  for (uint k = 0u; k < nfree; ++k) {
    if ((wk[((7u * n) + k)] == 2.000000)) {
      uint c = iset[((8u + n) + k)];
      float acc = 0.000000;
      for (uint j = 0u; j < nc; ++j) {
        acc = ((acc + (Y[((j * n) + c)] * tv[j])) + (S[((j * n) + c)] * (theta * r2[j])));
      }
      wk[((3u * n) + k)] = ((wk[((8u * n) + k)] / theta) + (acc / (theta * theta)));
    }
  }
  return ok;
}

[shader("compute")] [numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint n = params.n;
  uint mc = params.mcap;
  uint nc = st[0u];
  float theta = bf[0u];
  uint nact = iset[0u];
  uint nfree = iset[1u];
  iset[4u] = 0u;
  iset[5u] = 0u;
  iset[6u] = 0u;
  for (uint i = 0u; i < n; ++i) {
    drt[i] = (xcp[i] - x[i]);
  }
  if ((nfree < 1u)) {
    return;
  }
  for (uint k = 0u; k < nfree; ++k) {
    wk[((2u * n) + k)] = 0.000000;
  }
  float rh[32];
  for (uint j = 0u; j < 32u; ++j) {
    rh[j] = 0.000000;
  }
  if (((nc > 0u) && (nact > 0u))) {
    if ((nact <= nfree)) {
      for (uint j = 0u; j < nc; ++j) {
        float dy = 0.000000;
        float ds = 0.000000;
        for (uint q = 0u; q < nact; ++q) {
          uint c = iset[(8u + q)];
          dy = (dy + (Y[((j * n) + c)] * drt[c]));
          ds = (ds + (S[((j * n) + c)] * drt[c]));
        }
        rh[j] = dy;
        rh[(nc + j)] = (theta * ds);
      }
    } else {
      for (uint j = 0u; j < nc; ++j) {
        float dy = 0.000000;
        float ds = 0.000000;
        for (uint q = 0u; q < nfree; ++q) {
          uint c = iset[((8u + n) + q)];
          dy = (dy + (Y[((j * n) + c)] * drt[c]));
          ds = (ds + (S[((j * n) + c)] * drt[c]));
        }
        rh[j] = (vecc[j] - dy);
        rh[(nc + j)] = (vecc[(nc + j)] - (theta * ds));
      }
    }
    lb_mv(nc, mc, rh);
    for (uint j = 0u; j < nc; ++j) {
      rh[(nc + j)] = (theta * rh[(nc + j)]);
    }
    for (uint k = 0u; k < nfree; ++k) {
      float acc = 0.000000;
      for (uint j = 0u; j < nc; ++j) {
        acc = ((acc + (Y[((j * n) + iset[((8u + n) + k)])] * rh[j])) + (S[((j * n) + iset[((8u + n) + k)])] * rh[(nc + j)]));
      }
      wk[((2u * n) + k)] = (-acc);
    }
  }
  for (uint k = 0u; k < nfree; ++k) {
    uint c = iset[((8u + n) + k)];
    wk[((0u * n) + k)] = (lb[c] - x[c]);
    wk[((1u * n) + k)] = (ub[c] - x[c]);
    wk[((2u * n) + k)] = (wk[((2u * n) + k)] + g[c]);
    wk[((7u * n) + k)] = 2.000000;
    wk[((8u * n) + k)] = (-wk[((2u * n) + k)]);
  }
  if ((lb_solve_p(n, mc, nc, nfree, theta) == 0u)) {
    iset[6u] = 1u;
  }
  uint inb = 1u;
  for (uint k = 0u; k < nfree; ++k) {
    if (((wk[((3u * n) + k)] < wk[((0u * n) + k)]) || (wk[((3u * n) + k)] > wk[((1u * n) + k)]))) {
      inb = 0u;
    }
  }
  if ((inb == 1u)) {
    for (uint k = 0u; k < nfree; ++k) {
      drt[iset[((8u + n) + k)]] = wk[((3u * n) + k)];
    }
    iset[5u] = 1u;
    return;
  }
  for (uint k = 0u; k < nfree; ++k) {
    wk[((4u * n) + k)] = wk[((3u * n) + k)];
    wk[((5u * n) + k)] = 0.000000;
    wk[((6u * n) + k)] = 0.000000;
  }
  uint nL = 0u;
  uint nU = 0u;
  uint nP = 0u;
  uint kk = 0u;
  uint conv = 0u;
  while (((conv == 0u) && (kk < params.maxit))) {
    nL = 0u;
    nU = 0u;
    nP = 0u;
    for (uint k = 0u; k < nfree; ++k) {
      float yk = wk[((3u * n) + k)];
      float lk = wk[((0u * n) + k)];
      float uk = wk[((1u * n) + k)];
      if (((yk < lk) || ((yk == lk) && (wk[((5u * n) + k)] >= 0.000000)))) {
        wk[((7u * n) + k)] = 0.000000;
        wk[((3u * n) + k)] = lk;
        wk[((6u * n) + k)] = 0.000000;
        nL = (nL + 1u);
      } else {
        if (((yk > uk) || ((yk == uk) && (wk[((6u * n) + k)] >= 0.000000)))) {
          wk[((7u * n) + k)] = 1.000000;
          wk[((3u * n) + k)] = uk;
          wk[((5u * n) + k)] = 0.000000;
          nU = (nU + 1u);
        } else {
          wk[((7u * n) + k)] = 2.000000;
          wk[((5u * n) + k)] = 0.000000;
          wk[((6u * n) + k)] = 0.000000;
          nP = (nP + 1u);
        }
      }
    }
    if ((nP > 0u)) {
      float wq[32];
      for (uint j = 0u; j < nc; ++j) {
        float dy = 0.000000;
        float ds = 0.000000;
        for (uint k = 0u; k < nfree; ++k) {
          if ((wk[((7u * n) + k)] < 2.000000)) {
            uint c = iset[((8u + n) + k)];
            dy = (dy + (Y[((j * n) + c)] * ((wk[((7u * n) + k)] == 0.000000) ? wk[((0u * n) + k)] : wk[((1u * n) + k)])));
            ds = (ds + (S[((j * n) + c)] * ((wk[((7u * n) + k)] == 0.000000) ? wk[((0u * n) + k)] : wk[((1u * n) + k)])));
          }
        }
        wq[j] = dy;
        wq[(nc + j)] = (theta * ds);
      }
      lb_mv(nc, mc, wq);
      for (uint j = 0u; j < nc; ++j) {
        wq[(nc + j)] = (theta * wq[(nc + j)]);
      }
      for (uint k = 0u; k < nfree; ++k) {
        if ((wk[((7u * n) + k)] == 2.000000)) {
          float acc = 0.000000;
          for (uint j = 0u; j < nc; ++j) {
            acc = ((acc + (Y[((j * n) + iset[((8u + n) + k)])] * wq[j])) + (S[((j * n) + iset[((8u + n) + k)])] * wq[(nc + j)]));
          }
          wk[((8u * n) + k)] = (-(wk[((2u * n) + k)] - acc));
        }
      }
      if ((lb_solve_p(n, mc, nc, nfree, theta) == 0u)) {
        iset[6u] = 1u;
      }
    }
    if (((nL > 0u) || (nU > 0u))) {
      float fy[32];
      for (uint j = 0u; j < nc; ++j) {
        float dy = 0.000000;
        float ds = 0.000000;
        for (uint k = 0u; k < nfree; ++k) {
          if (true) {
            uint c = iset[((8u + n) + k)];
            dy = (dy + (Y[((j * n) + c)] * wk[((3u * n) + k)]));
            ds = (ds + (S[((j * n) + c)] * wk[((3u * n) + k)]));
          }
        }
        fy[j] = dy;
        fy[(nc + j)] = (theta * ds);
      }
      lb_mv(nc, mc, fy);
      for (uint j = 0u; j < nc; ++j) {
        fy[(nc + j)] = (theta * fy[(nc + j)]);
      }
      for (uint k = 0u; k < nfree; ++k) {
        if ((wk[((7u * n) + k)] < 2.000000)) {
          float acc = 0.000000;
          for (uint j = 0u; j < nc; ++j) {
            acc = ((acc + (Y[((j * n) + iset[((8u + n) + k)])] * fy[j])) + (S[((j * n) + iset[((8u + n) + k)])] * fy[(nc + j)]));
          }
          float res = (((-acc) + wk[((2u * n) + k)]) + (theta * wk[((3u * n) + k)]));
          if ((wk[((7u * n) + k)] == 0.000000)) {
            wk[((5u * n) + k)] = res;
          } else {
            wk[((6u * n) + k)] = (-res);
          }
        }
      }
    }
    uint cv = 1u;
    for (uint k = 0u; k < nfree; ++k) {
      if (((wk[((7u * n) + k)] == 0.000000) && (wk[((5u * n) + k)] < 0.000000))) {
        cv = 0u;
      }
      if (((wk[((7u * n) + k)] == 1.000000) && (wk[((6u * n) + k)] < 0.000000))) {
        cv = 0u;
      }
      if (((wk[((7u * n) + k)] == 2.000000) && ((wk[((3u * n) + k)] < wk[((0u * n) + k)]) || (wk[((3u * n) + k)] > wk[((1u * n) + k)])))) {
        cv = 0u;
      }
    }
    if ((cv == 1u)) {
      conv = 1u;
    } else {
      kk = (kk + 1u);
    }
  }
  iset[4u] = kk;
  if ((conv == 1u)) {
    for (uint k = 0u; k < nfree; ++k) {
      drt[iset[((8u + n) + k)]] = wk[((3u * n) + k)];
    }
    iset[5u] = 2u;
    return;
  }
  float a_hi;
  float a_lo;
  float dg = 0.000000;
  for (uint k = 0u; k < nfree; ++k) {
    wk[((3u * n) + k)] = min(max(wk[((3u * n) + k)], wk[((0u * n) + k)]), wk[((1u * n) + k)]);
  }
  for (uint k = 0u; k < nfree; ++k) {
    drt[iset[((8u + n) + k)]] = wk[((3u * n) + k)];
  }
  a_hi = 0.000000;
  a_lo = 0.000000;
  for (uint i = 0u; i < n; ++i) {
    df_acc(a_hi, a_lo, drt[i], g[i]);
  }
  dg = (a_hi + a_lo);
  if ((dg <= (-asfloat(629145600u)))) {
    iset[5u] = 3u;
  } else {
    for (uint k = 0u; k < nfree; ++k) {
      drt[iset[((8u + n) + k)]] = min(max(wk[((4u * n) + k)], wk[((0u * n) + k)]), wk[((1u * n) + k)]);
    }
    a_hi = 0.000000;
    a_lo = 0.000000;
    for (uint i = 0u; i < n; ++i) {
      df_acc(a_hi, a_lo, drt[i], g[i]);
    }
    dg = (a_hi + a_lo);
    if ((dg <= (-asfloat(629145600u)))) {
      iset[5u] = 4u;
    } else {
      for (uint k = 0u; k < nfree; ++k) {
        drt[iset[((8u + n) + k)]] = wk[((4u * n) + k)];
      }
      iset[5u] = 5u;
    }
  }
}
