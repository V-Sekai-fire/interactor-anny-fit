struct LbCompactParams {
  uint mcap;
  uint mode;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LbCompactParams> params;
[[vk::binding(1, 0)]]
StructuredBuffer<float> dots;
[[vk::binding(2, 0)]]
RWStructuredBuffer<uint> st;
[[vk::binding(3, 0)]]
RWStructuredBuffer<float> bf;

[shader("compute")] [numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint mc = params.mcap;
  if ((params.mode == 1u)) {
    st[0u] = 0u;
    st[1u] = mc;
    st[2u] = 0u;
    st[3u] = 0u;
    st[4u] = 1u;
    bf[0u] = 1.000000;
    return;
  }
  float sy = (dots[(4u * mc)] + dots[((4u * mc) + 1u)]);
  float yy = (dots[((4u * mc) + 2u)] + dots[(((4u * mc) + 2u) + 1u)]);
  float ss = (dots[((4u * mc) + 4u)] + dots[(((4u * mc) + 4u) + 1u)]);
  uint ncorr = st[0u];
  uint ptr = st[1u];
  uint ok = ((sy > (asfloat(872415232u) * yy)) ? 1u : 0u);
  st[3u] = ok;
  if ((ok == 1u)) {
    uint loc = (ptr % mc);
    ncorr = min((ncorr + 1u), mc);
    for (uint j = 0u; j < ncorr; ++j) {
      float sj = ((j == loc) ? ss : (dots[(2u * j)] + dots[((2u * j) + 1u)]));
      bf[((4u + (loc * mc)) + j)] = sj;
      bf[((4u + (j * mc)) + loc)] = sj;
    }
    for (uint i = 0u; i < mc; ++i) {
      bf[(((4u + (mc * mc)) + (i * mc)) + loc)] = 0.000000;
    }
    for (uint j = 0u; j < ncorr; ++j) {
      bf[(((4u + (mc * mc)) + (loc * mc)) + j)] = ((j == loc) ? sy : (dots[((2u * mc) + (2u * j))] + dots[(((2u * mc) + (2u * j)) + 1u)]));
    }
    bf[((4u + ((4u * mc) * mc)) + loc)] = sy;
    bf[0u] = (yy / sy);
    ptr = (loc + 1u);
    st[0u] = ncorr;
    st[1u] = ptr;
    st[2u] = loc;
  }
  float theta = bf[0u];
  uint oldest = (((ptr + mc) - ncorr) % mc);
  for (uint i = 0u; i < ncorr; ++i) {
    uint ri = (((i + mc) - oldest) % mc);
    for (uint j = 0u; j < ncorr; ++j) {
      uint rj = (((j + mc) - oldest) % mc);
      bf[(((4u + ((2u * mc) * mc)) + (i * mc)) + j)] = ((ri > rj) ? bf[(((4u + (mc * mc)) + (i * mc)) + j)] : 0.000000);
    }
  }
  for (uint i = 0u; i < ncorr; ++i) {
    for (uint j = 0u; j < (i + 1u); ++j) {
      float acc = (theta * bf[((4u + (i * mc)) + j)]);
      for (uint k = 0u; k < ncorr; ++k) {
        acc = (acc + ((bf[(((4u + ((2u * mc) * mc)) + (i * mc)) + k)] * bf[(((4u + ((2u * mc) * mc)) + (j * mc)) + k)]) / bf[((4u + ((4u * mc) * mc)) + k)]));
      }
      bf[(((4u + ((3u * mc) * mc)) + (i * mc)) + j)] = acc;
    }
  }
  uint cok = 1u;
  for (uint cj = 0u; cj < ncorr; ++cj) {
    float cs = bf[(((4u + ((3u * mc) * mc)) + (cj * mc)) + cj)];
    for (uint ct = 0u; ct < cj; ++ct) {
      cs = (cs - (bf[(((4u + ((3u * mc) * mc)) + (cj * mc)) + ct)] * bf[(((4u + ((3u * mc) * mc)) + (cj * mc)) + ct)]));
    }
    if ((cs <= 0.000000)) {
      cok = 0u;
      cs = (asfloat(872415232u) * asfloat(872415232u));
    }
    float cd = sqrt(cs);
    bf[(((4u + ((3u * mc) * mc)) + (cj * mc)) + cj)] = cd;
    for (uint ci = (cj + 1u); ci < ncorr; ++ci) {
      float cr = bf[(((4u + ((3u * mc) * mc)) + (ci * mc)) + cj)];
      for (uint ct = 0u; ct < cj; ++ct) {
        cr = (cr - (bf[(((4u + ((3u * mc) * mc)) + (ci * mc)) + ct)] * bf[(((4u + ((3u * mc) * mc)) + (cj * mc)) + ct)]));
      }
      bf[(((4u + ((3u * mc) * mc)) + (ci * mc)) + cj)] = (cr / cd);
    }
  }
  st[4u] = cok;
}
