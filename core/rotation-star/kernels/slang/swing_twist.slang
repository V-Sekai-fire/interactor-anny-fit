struct SwingTwistParams {
  uint N;
};

[[vk::binding(0, 0)]]
ConstantBuffer<SwingTwistParams> params;
[[vk::binding(1, 0)]]
StructuredBuffer<float> rot;
[[vk::binding(2, 0)]]
StructuredBuffer<uint> axis;
[[vk::binding(3, 0)]]
RWStructuredBuffer<float> swing;
[[vk::binding(4, 0)]]
RWStructuredBuffer<float> twist;
[[vk::binding(5, 0)]]
RWStructuredBuffer<float> angle;

[shader("compute")] [numthreads(64, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint i = tid.x;
  if ((i >= params.N)) {
    return;
  }
  float R0 = rot[((i * 9u) + 0u)];
  float R1 = rot[((i * 9u) + 1u)];
  float R2 = rot[((i * 9u) + 2u)];
  float R3 = rot[((i * 9u) + 3u)];
  float R4 = rot[((i * 9u) + 4u)];
  float R5 = rot[((i * 9u) + 5u)];
  float R6 = rot[((i * 9u) + 6u)];
  float R7 = rot[((i * 9u) + 7u)];
  float R8 = rot[((i * 9u) + 8u)];
  uint ax = axis[i];
  float st_v00 = ((ax == 0u) ? 1.000000 : 0.000000);
  float st_v01 = ((ax == 1u) ? 1.000000 : 0.000000);
  float st_v02 = ((ax == 2u) ? 1.000000 : 0.000000);
  float st_ru0 = (((R0 * st_v00) + (R1 * st_v01)) + (R2 * st_v02));
  float st_ru1 = (((R3 * st_v00) + (R4 * st_v01)) + (R5 * st_v02));
  float st_ru2 = (((R6 * st_v00) + (R7 * st_v01)) + (R8 * st_v02));
  float st_u0_l = sqrt((((st_ru0 * st_ru0) + (st_ru1 * st_ru1)) + (st_ru2 * st_ru2)));
  float st_u00 = ((st_u0_l > 1.0e-12f) ? (st_ru0 / st_u0_l) : st_ru0);
  float st_u01 = ((st_u0_l > 1.0e-12f) ? (st_ru1 / st_u0_l) : st_ru1);
  float st_u02 = ((st_u0_l > 1.0e-12f) ? (st_ru2 / st_u0_l) : st_ru2);
  bool st_fy = (abs(st_v01) < 0.900000);
  float st_ref0 = (st_fy ? 0.000000 : 1.000000);
  float st_ref1 = (st_fy ? 1.000000 : 0.000000);
  float st_ref2 = 0.000000;
  float st_v1r0 = ((st_v01 * st_ref2) - (st_v02 * st_ref1));
  float st_v1r1 = ((st_v02 * st_ref0) - (st_v00 * st_ref2));
  float st_v1r2 = ((st_v00 * st_ref1) - (st_v01 * st_ref0));
  float st_v1_l = sqrt((((st_v1r0 * st_v1r0) + (st_v1r1 * st_v1r1)) + (st_v1r2 * st_v1r2)));
  float st_v10 = ((st_v1_l > 1.0e-12f) ? (st_v1r0 / st_v1_l) : st_v1r0);
  float st_v11 = ((st_v1_l > 1.0e-12f) ? (st_v1r1 / st_v1_l) : st_v1r1);
  float st_v12 = ((st_v1_l > 1.0e-12f) ? (st_v1r2 / st_v1_l) : st_v1r2);
  float st_v2r0 = ((st_v01 * st_v12) - (st_v02 * st_v11));
  float st_v2r1 = ((st_v02 * st_v10) - (st_v00 * st_v12));
  float st_v2r2 = ((st_v00 * st_v11) - (st_v01 * st_v10));
  float st_v2_l = sqrt((((st_v2r0 * st_v2r0) + (st_v2r1 * st_v2r1)) + (st_v2r2 * st_v2r2)));
  float st_v20 = ((st_v2_l > 1.0e-12f) ? (st_v2r0 / st_v2_l) : st_v2r0);
  float st_v21 = ((st_v2_l > 1.0e-12f) ? (st_v2r1 / st_v2_l) : st_v2r1);
  float st_v22 = ((st_v2_l > 1.0e-12f) ? (st_v2r2 / st_v2_l) : st_v2r2);
  float st_ax0 = ((st_v01 * st_u02) - (st_v02 * st_u01));
  float st_ax1 = ((st_v02 * st_u00) - (st_v00 * st_u02));
  float st_ax2 = ((st_v00 * st_u01) - (st_v01 * st_u00));
  float st_sn = sqrt((((st_ax0 * st_ax0) + (st_ax1 * st_ax1)) + (st_ax2 * st_ax2)));
  float st_cs = (((st_v00 * st_u00) + (st_v01 * st_u01)) + (st_v02 * st_u02));
  float st_k0 = (st_ax0 / max(st_sn, 1.0e-9f));
  float st_k1 = (st_ax1 / max(st_sn, 1.0e-9f));
  float st_k2 = (st_ax2 / max(st_sn, 1.0e-9f));
  float st_c1_kx0 = ((st_k1 * st_v12) - (st_k2 * st_v11));
  float st_c1_kx1 = ((st_k2 * st_v10) - (st_k0 * st_v12));
  float st_c1_kx2 = ((st_k0 * st_v11) - (st_k1 * st_v10));
  float st_c1_kw = (((st_k0 * st_v10) + (st_k1 * st_v11)) + (st_k2 * st_v12));
  float st_c10 = (((st_cs * st_v10) + (st_sn * st_c1_kx0)) + ((st_c1_kw * (1.000000 - st_cs)) * st_k0));
  float st_c11 = (((st_cs * st_v11) + (st_sn * st_c1_kx1)) + ((st_c1_kw * (1.000000 - st_cs)) * st_k1));
  float st_c12 = (((st_cs * st_v12) + (st_sn * st_c1_kx2)) + ((st_c1_kw * (1.000000 - st_cs)) * st_k2));
  float st_c2_kx0 = ((st_k1 * st_v22) - (st_k2 * st_v21));
  float st_c2_kx1 = ((st_k2 * st_v20) - (st_k0 * st_v22));
  float st_c2_kx2 = ((st_k0 * st_v21) - (st_k1 * st_v20));
  float st_c2_kw = (((st_k0 * st_v20) + (st_k1 * st_v21)) + (st_k2 * st_v22));
  float st_c20 = (((st_cs * st_v20) + (st_sn * st_c2_kx0)) + ((st_c2_kw * (1.000000 - st_cs)) * st_k0));
  float st_c21 = (((st_cs * st_v21) + (st_sn * st_c2_kx1)) + ((st_c2_kw * (1.000000 - st_cs)) * st_k1));
  float st_c22 = (((st_cs * st_v22) + (st_sn * st_c2_kx2)) + ((st_c2_kw * (1.000000 - st_cs)) * st_k2));
  if ((st_sn < 1.0e-9f)) {
    if ((st_cs > 0.000000)) {
      st_c10 = st_v10;
      st_c11 = st_v11;
      st_c12 = st_v12;
      st_c20 = st_v20;
      st_c21 = st_v21;
      st_c22 = st_v22;
    } else {
      bool st_px = ((abs(st_u00) <= abs(st_u01)) && (abs(st_u00) <= abs(st_u02)));
      float st_pr0 = (st_px ? 1.000000 : 0.000000);
      float st_pr1 = (st_px ? 0.000000 : 1.000000);
      float st_pr2 = 0.000000;
      float st_p10 = ((st_u01 * st_pr2) - (st_u02 * st_pr1));
      float st_p11 = ((st_u02 * st_pr0) - (st_u00 * st_pr2));
      float st_p12 = ((st_u00 * st_pr1) - (st_u01 * st_pr0));
      float st_p1n_l = sqrt((((st_p10 * st_p10) + (st_p11 * st_p11)) + (st_p12 * st_p12)));
      float st_p1n0 = ((st_p1n_l > 1.0e-12f) ? (st_p10 / st_p1n_l) : st_p10);
      float st_p1n1 = ((st_p1n_l > 1.0e-12f) ? (st_p11 / st_p1n_l) : st_p11);
      float st_p1n2 = ((st_p1n_l > 1.0e-12f) ? (st_p12 / st_p1n_l) : st_p12);
      float st_p20 = ((st_u01 * st_p1n2) - (st_u02 * st_p1n1));
      float st_p21 = ((st_u02 * st_p1n0) - (st_u00 * st_p1n2));
      float st_p22 = ((st_u00 * st_p1n1) - (st_u01 * st_p1n0));
      float st_p2n_l = sqrt((((st_p20 * st_p20) + (st_p21 * st_p21)) + (st_p22 * st_p22)));
      float st_p2n0 = ((st_p2n_l > 1.0e-12f) ? (st_p20 / st_p2n_l) : st_p20);
      float st_p2n1 = ((st_p2n_l > 1.0e-12f) ? (st_p21 / st_p2n_l) : st_p21);
      float st_p2n2 = ((st_p2n_l > 1.0e-12f) ? (st_p22 / st_p2n_l) : st_p22);
      st_c10 = st_p1n0;
      st_c11 = st_p1n1;
      st_c12 = st_p1n2;
      st_c20 = st_p2n0;
      st_c21 = st_p2n1;
      st_c22 = st_p2n2;
    }
  }
  float st_R00 = (((st_u00 * st_v00) + (st_c10 * st_v10)) + (st_c20 * st_v20));
  float st_R01 = (((st_u00 * st_v01) + (st_c10 * st_v11)) + (st_c20 * st_v21));
  float st_R02 = (((st_u00 * st_v02) + (st_c10 * st_v12)) + (st_c20 * st_v22));
  float st_R03 = (((st_u01 * st_v00) + (st_c11 * st_v10)) + (st_c21 * st_v20));
  float st_R04 = (((st_u01 * st_v01) + (st_c11 * st_v11)) + (st_c21 * st_v21));
  float st_R05 = (((st_u01 * st_v02) + (st_c11 * st_v12)) + (st_c21 * st_v22));
  float st_R06 = (((st_u02 * st_v00) + (st_c12 * st_v10)) + (st_c22 * st_v20));
  float st_R07 = (((st_u02 * st_v01) + (st_c12 * st_v11)) + (st_c22 * st_v21));
  float st_R08 = (((st_u02 * st_v02) + (st_c12 * st_v12)) + (st_c22 * st_v22));
  float st_T0 = (((st_R00 * R0) + (st_R03 * R3)) + (st_R06 * R6));
  float st_T1 = (((st_R00 * R1) + (st_R03 * R4)) + (st_R06 * R7));
  float st_T2 = (((st_R00 * R2) + (st_R03 * R5)) + (st_R06 * R8));
  float st_T3 = (((st_R01 * R0) + (st_R04 * R3)) + (st_R07 * R6));
  float st_T4 = (((st_R01 * R1) + (st_R04 * R4)) + (st_R07 * R7));
  float st_T5 = (((st_R01 * R2) + (st_R04 * R5)) + (st_R07 * R8));
  float st_T6 = (((st_R02 * R0) + (st_R05 * R3)) + (st_R08 * R6));
  float st_T7 = (((st_R02 * R1) + (st_R05 * R4)) + (st_R08 * R7));
  float st_T8 = (((st_R02 * R2) + (st_R05 * R5)) + (st_R08 * R8));
  float st = atan2(((ax == 0u) ? (st_T7 - st_T5) : ((ax == 1u) ? (st_T2 - st_T6) : (st_T3 - st_T1))), ((ax == 0u) ? (st_T4 + st_T8) : ((ax == 1u) ? (st_T8 + st_T0) : (st_T0 + st_T4))));
  swing[((i * 9u) + 0u)] = st_R00;
  swing[((i * 9u) + 1u)] = st_R01;
  swing[((i * 9u) + 2u)] = st_R02;
  swing[((i * 9u) + 3u)] = st_R03;
  swing[((i * 9u) + 4u)] = st_R04;
  swing[((i * 9u) + 5u)] = st_R05;
  swing[((i * 9u) + 6u)] = st_R06;
  swing[((i * 9u) + 7u)] = st_R07;
  swing[((i * 9u) + 8u)] = st_R08;
  twist[((i * 9u) + 0u)] = st_T0;
  twist[((i * 9u) + 1u)] = st_T1;
  twist[((i * 9u) + 2u)] = st_T2;
  twist[((i * 9u) + 3u)] = st_T3;
  twist[((i * 9u) + 4u)] = st_T4;
  twist[((i * 9u) + 5u)] = st_T5;
  twist[((i * 9u) + 6u)] = st_T6;
  twist[((i * 9u) + 7u)] = st_T7;
  twist[((i * 9u) + 8u)] = st_T8;
  angle[i] = st;
}
