struct SdfSplineParams {
  uint count;
};

[[vk::binding(0, 0)]]
StructuredBuffer<double> stencil;
[[vk::binding(1, 0)]]
StructuredBuffer<double> uvw;
[[vk::binding(2, 0)]]
RWStructuredBuffer<double> result;
[[vk::binding(3, 0)]]
ConstantBuffer<SdfSplineParams> params;

double bspline(double x) {
  double absx = abs(x);
  if ((absx >= double(2))) {
    return double(0);
  }
  if ((absx >= double(1))) {
    double tmp = (double(2) - absx);
    return (((tmp * tmp) * tmp) / double(6));
  }
  return ((double(2) / double(3)) + ((((double(0.500000) * absx) - double(1)) * x) * x));
}

double bspline_d1(double x) {
  double absx = abs(x);
  if ((absx >= double(2))) {
    return double(0);
  }
  if ((absx >= double(1))) {
    double tmp = (double(2) - absx);
    return (((double(-0.500000) * tmp) * tmp) * ((x > double(0)) ? double(1) : double((-1))));
  }
  return (x * ((double(1.500000) * absx) - double(2)));
}

double bspline_d2(double x) {
  double absx = abs(x);
  if ((absx >= double(2))) {
    return double(0);
  }
  if ((absx >= double(1))) {
    return (double(2) - absx);
  }
  return ((double(3) * absx) - double(2));
}

[shader("compute")] [numthreads(64, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint lane = tid.x;
  if ((lane >= params.count)) {
    return;
  }
  uint sb = (64u * lane);
  uint ub = (3u * lane);
  uint rb = (10u * lane);
  double basis[12];
  double deriv1[12];
  double deriv2[12];
  for (uint d = 0u; d < 3u; ++d) {
    double w = uvw[(ub + d)];
    uint t = (4u * d);
    basis[t] = bspline((w + double(1)));
    deriv1[t] = bspline_d1((w + double(1)));
    deriv2[t] = bspline_d2((w + double(1)));
    basis[(t + 1u)] = bspline(w);
    deriv1[(t + 1u)] = bspline_d1(w);
    deriv2[(t + 1u)] = bspline_d2(w);
    basis[(t + 2u)] = bspline((double(1) - w));
    deriv1[(t + 2u)] = (-bspline_d1((double(1) - w)));
    deriv2[(t + 2u)] = bspline_d2((double(1) - w));
    basis[(t + 3u)] = bspline((double(2) - w));
    deriv1[(t + 3u)] = (-bspline_d1((double(2) - w)));
    deriv2[(t + 3u)] = bspline_d2((double(2) - w));
  }
  double x = double(0);
  double g0 = double(0);
  double g1 = double(0);
  double g2 = double(0);
  double h00 = double(0);
  double h01 = double(0);
  double h02 = double(0);
  double h11 = double(0);
  double h12 = double(0);
  double h22 = double(0);
  for (uint i = 0u; i < 4u; ++i) {
    for (uint j = 0u; j < 4u; ++j) {
      for (uint k = 0u; k < 4u; ++k) {
        double s = stencil[(((sb + (i * 16u)) + (j * 4u)) + k)];
        x = (x + (((s * basis[i]) * basis[(4u + j)]) * basis[(8u + k)]));
        g0 = (g0 + (((s * deriv1[i]) * basis[(4u + j)]) * basis[(8u + k)]));
        g1 = (g1 + (((s * basis[i]) * deriv1[(4u + j)]) * basis[(8u + k)]));
        g2 = (g2 + (((s * basis[i]) * basis[(4u + j)]) * deriv1[(8u + k)]));
        h00 = (h00 + (((s * deriv2[i]) * basis[(4u + j)]) * basis[(8u + k)]));
        h01 = (h01 + (((s * deriv1[i]) * deriv1[(4u + j)]) * basis[(8u + k)]));
        h02 = (h02 + (((s * deriv1[i]) * basis[(4u + j)]) * deriv1[(8u + k)]));
        h11 = (h11 + (((s * basis[i]) * deriv2[(4u + j)]) * basis[(8u + k)]));
        h12 = (h12 + (((s * basis[i]) * deriv1[(4u + j)]) * deriv1[(8u + k)]));
        h22 = (h22 + (((s * basis[i]) * basis[(4u + j)]) * deriv2[(8u + k)]));
      }
    }
  }
  result[rb] = x;
  result[(rb + 1u)] = g0;
  result[(rb + 2u)] = g1;
  result[(rb + 3u)] = g2;
  result[(rb + 4u)] = h00;
  result[(rb + 5u)] = h01;
  result[(rb + 6u)] = h02;
  result[(rb + 7u)] = h11;
  result[(rb + 8u)] = h12;
  result[(rb + 9u)] = h22;
}
