struct LbCauchyParams {
  uint n;
  uint mcap;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LbCauchyParams> params;
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
RWStructuredBuffer<float> xcp;
[[vk::binding(10, 0)]]
RWStructuredBuffer<float> vecc;
[[vk::binding(11, 0)]]
RWStructuredBuffer<float> wk;
[[vk::binding(12, 0)]]
RWStructuredBuffer<uint> iset;

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

[shader("compute")] [numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint n = params.n;
  uint mc = params.mcap;
  uint nc = st[0u];
  float theta = bf[0u];
  uint fvb = (8u + n);
  uint ordb = (8u + (2u * n));
  for (uint i = 0u; i < n; ++i) {
    xcp[i] = x[i];
  }
  float vc[32];
  float vp[32];
  float ch[32];
  float wa[32];
  for (uint j = 0u; j < 32u; ++j) {
    vc[j] = 0.000000;
    vp[j] = 0.000000;
    ch[j] = 0.000000;
    wa[j] = 0.000000;
  }
  uint nfree = 0u;
  uint nord = 0u;
  for (uint i = 0u; i < n; ++i) {
    float li = lb[i];
    float ui = ub[i];
    float gi = g[i];
    float xi = x[i];
    float b = asfloat(2139095039u);
    if ((li == ui)) {
      b = 0.000000;
    } else {
      if ((gi < 0.000000)) {
        if ((ui < asfloat(1900671690u))) {
          b = ((xi - ui) / gi);
        }
      } else {
        if ((gi > 0.000000)) {
          if ((li > (-asfloat(1900671690u)))) {
            b = ((xi - li) / gi);
          }
        }
      }
    }
    wk[i] = b;
    wk[(n + i)] = ((b == 0.000000) ? 0.000000 : (-gi));
    if ((b >= asfloat(2139095039u))) {
      iset[(fvb + nfree)] = i;
      nfree = (nfree + 1u);
    } else {
      if ((b != 0.000000)) {
        iset[(ordb + nord)] = i;
        nord = (nord + 1u);
      }
    }
  }
  for (uint q = 0u; q < (nord / 2u); ++q) {
    uint hr = (((nord / 2u) - 1u) - q);
    uint hgo = 1u;
    while ((hgo == 1u)) {
      uint hc = ((2u * hr) + 1u);
      if ((hc >= nord)) {
        hgo = 0u;
      } else {
        if ((((hc + 1u) < nord) && (wk[iset[(ordb + hc)]] < wk[iset[(ordb + (hc + 1u))]]))) {
          hc = (hc + 1u);
        }
        if ((wk[iset[(ordb + hr)]] < wk[iset[(ordb + hc)]])) {
          uint htmp = iset[(ordb + hr)];
          iset[(ordb + hr)] = iset[(ordb + hc)];
          iset[(ordb + hc)] = htmp;
          hr = hc;
        } else {
          hgo = 0u;
        }
      }
    }
  }
  if ((nord > 1u)) {
    for (uint q = 0u; q < (nord - 1u); ++q) {
      uint e = ((nord - 1u) - q);
      uint sw = iset[(ordb + 0u)];
      iset[(ordb + 0u)] = iset[(ordb + e)];
      iset[(ordb + e)] = sw;
      uint sr = 0u;
      uint sgo = 1u;
      while ((sgo == 1u)) {
        uint sc = ((2u * sr) + 1u);
        if ((sc >= e)) {
          sgo = 0u;
        } else {
          if ((((sc + 1u) < e) && (wk[iset[(ordb + sc)]] < wk[iset[(ordb + (sc + 1u))]]))) {
            sc = (sc + 1u);
          }
          if ((wk[iset[(ordb + sr)]] < wk[iset[(ordb + sc)]])) {
            uint stmp = iset[(ordb + sr)];
            iset[(ordb + sr)] = iset[(ordb + sc)];
            iset[(ordb + sc)] = stmp;
            sr = sc;
          } else {
            sgo = 0u;
          }
        }
      }
    }
  }
  iset[3u] = nord;
  if (((nfree == 0u) && (nord == 0u))) {
    for (uint j = 0u; j < (2u * nc); ++j) {
      vecc[j] = 0.000000;
    }
    iset[0u] = 0u;
    iset[1u] = 0u;
    iset[2u] = 2u;
    return;
  }
  float a_hi;
  float a_lo;
  float sum;
  for (uint j = 0u; j < nc; ++j) {
    a_hi = 0.000000;
    a_lo = 0.000000;
    for (uint i = 0u; i < n; ++i) {
      df_acc(a_hi, a_lo, Y[((j * n) + i)], wk[(n + i)]);
    }
    sum = (a_hi + a_lo);
    vp[j] = sum;
    a_hi = 0.000000;
    a_lo = 0.000000;
    for (uint i = 0u; i < n; ++i) {
      df_acc(a_hi, a_lo, S[((j * n) + i)], wk[(n + i)]);
    }
    sum = (a_hi + a_lo);
    vp[(nc + j)] = (theta * sum);
  }
  a_hi = 0.000000;
  a_lo = 0.000000;
  for (uint i = 0u; i < n; ++i) {
    df_acc(a_hi, a_lo, wk[(n + i)], wk[(n + i)]);
  }
  sum = (a_hi + a_lo);
  float fp = (-sum);
  for (uint j = 0u; j < (2u * nc); ++j) {
    ch[j] = vp[j];
  }
  lb_mv(nc, mc, ch);
  float pm = 0.000000;
  for (uint j = 0u; j < (2u * nc); ++j) {
    pm = (pm + (vp[j] * ch[j]));
  }
  float fpp = (((-theta) * fp) - pm);
  float dtm = ((-fp) / fpp);
  float il = 0.000000;
  uint bb = 0u;
  float iu = ((nord < 1u) ? asfloat(2139095039u) : wk[iset[(ordb + 0u)]]);
  float dt = (iu - il);
  uint crossed = 0u;
  uint go = 1u;
  uint nact = 0u;
  while (((go == 1u) && (dtm >= dt))) {
    for (uint j = 0u; j < (2u * nc); ++j) {
      vc[j] = (vc[j] + (dt * vp[j]));
    }
    uint ae = bb;
    uint sg = 1u;
    while (((sg == 1u) && (ae < nord))) {
      if ((wk[iset[(ordb + ae)]] > iu)) {
        sg = 0u;
      } else {
        ae = (ae + 1u);
      }
    }
    if (((nfree == 0u) && (ae == nord))) {
      for (uint k = bb; k < ae; ++k) {
        uint a = iset[(ordb + k)];
        xcp[a] = ((wk[(n + a)] > 0.000000) ? ub[a] : lb[a]);
        iset[(8u + nact)] = a;
        nact = (nact + 1u);
      }
      crossed = 1u;
      go = 0u;
    } else {
      fp = (fp + (dt * fpp));
      for (uint k = bb; k < ae; ++k) {
        uint a = iset[(ordb + k)];
        float xa = ((wk[(n + a)] > 0.000000) ? ub[a] : lb[a]);
        xcp[a] = xa;
        float z = (xa - x[a]);
        float ga = g[a];
        float gg = (ga * ga);
        for (uint j = 0u; j < nc; ++j) {
          wa[j] = Y[((j * n) + a)];
          wa[(nc + j)] = (theta * S[((j * n) + a)]);
        }
        for (uint j = 0u; j < (2u * nc); ++j) {
          ch[j] = wa[j];
        }
        lb_mv(nc, mc, ch);
        float cv = 0.000000;
        float cp = 0.000000;
        float cw = 0.000000;
        for (uint j = 0u; j < (2u * nc); ++j) {
          cv = (cv + (ch[j] * vc[j]));
          cp = (cp + (ch[j] * vp[j]));
          cw = (cw + (ch[j] * wa[j]));
        }
        fp = (fp + ((gg + ((theta * ga) * z)) - (ga * cv)));
        fpp = (fpp - (((theta * gg) + ((2.000000 * ga) * cp)) + (gg * cw)));
        for (uint j = 0u; j < (2u * nc); ++j) {
          vp[j] = (vp[j] + (ga * wa[j]));
        }
        wk[(n + a)] = 0.000000;
        iset[(8u + nact)] = a;
        nact = (nact + 1u);
      }
      dtm = ((-fp) / fpp);
      il = iu;
      bb = ae;
      if ((bb >= nord)) {
        go = 0u;
      } else {
        iu = wk[iset[(ordb + bb)]];
        dt = (iu - il);
      }
    }
  }
  if ((fpp < asfloat(629145600u))) {
    dtm = ((-fp) / asfloat(629145600u));
  }
  if ((crossed == 0u)) {
    dtm = max(dtm, 0.000000);
    uint live = ((bb < nord) ? 1u : 0u);
    for (uint k = 0u; k < nfree; ++k) {
      if ((wk[(n + iset[(fvb + k)])] != 0.000000)) {
        live = 1u;
      }
    }
    if ((live == 0u)) {
      dtm = 0.000000;
    }
    for (uint j = 0u; j < (2u * nc); ++j) {
      vc[j] = (vc[j] + (dtm * vp[j]));
    }
    float tf = (il + dtm);
    for (uint k = 0u; k < nfree; ++k) {
      uint c = iset[(fvb + k)];
      xcp[c] = (x[c] + (tf * wk[(n + c)]));
    }
    for (uint k = bb; k < nord; ++k) {
      uint c = iset[(ordb + k)];
      xcp[c] = (x[c] + (tf * wk[(n + c)]));
      iset[(fvb + nfree)] = c;
      nfree = (nfree + 1u);
    }
  }
  for (uint j = 0u; j < (2u * nc); ++j) {
    vecc[j] = vc[j];
  }
  iset[0u] = nact;
  iset[1u] = nfree;
  iset[2u] = crossed;
}
