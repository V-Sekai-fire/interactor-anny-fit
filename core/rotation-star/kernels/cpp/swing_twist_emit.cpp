#include "slang-cpp-prelude.h"

#ifdef SLANG_PRELUDE_NAMESPACE
using namespace SLANG_PRELUDE_NAMESPACE;
#endif


#line 1 "slang/swing_twist.slang"
struct SwingTwistParams_0
{
    uint32_t N_0;
};


#line 138
struct GlobalParams_0
{
    SwingTwistParams_0* params_0;
    StructuredBuffer<float> rot_0;
    StructuredBuffer<uint32_t> axis_0;
    RWStructuredBuffer<float> swing_0;
    RWStructuredBuffer<float> twist_0;
    RWStructuredBuffer<float> angle_0;
};


#line 138
struct KernelContext_0
{
    GlobalParams_0* globalParams_0;
};


#line 19
void _main_0(void* _S1, void* entryPointParams_0, void* globalParams_1)
{

#line 19
    ComputeThreadVaryingInput * _S2 = (slang_bit_cast<ComputeThreadVaryingInput *>(_S1));

#line 19
    KernelContext_0 kernelContext_0;

#line 19
    (&kernelContext_0)->globalParams_0 = (slang_bit_cast<GlobalParams_0*>(globalParams_1));
    uint32_t i_0 = (_S2->groupID * Vector<uint32_t, 3> (64U, 1U, 1U) + _S2->groupThreadID).x;
    if(i_0 >= ((slang_bit_cast<GlobalParams_0*>(globalParams_1))->params_0->N_0))
    {

#line 22
        return;
    }
    uint32_t _S3 = i_0 * 9U;

#line 24
    float R0_0 = (&kernelContext_0)->globalParams_0->rot_0.Load(_S3);
    uint32_t _S4 = _S3 + 1U;

#line 25
    float R1_0 = (&kernelContext_0)->globalParams_0->rot_0.Load(_S4);
    uint32_t _S5 = _S3 + 2U;

#line 26
    float R2_0 = (&kernelContext_0)->globalParams_0->rot_0.Load(_S5);
    uint32_t _S6 = _S3 + 3U;

#line 27
    float R3_0 = (&kernelContext_0)->globalParams_0->rot_0.Load(_S6);
    uint32_t _S7 = _S3 + 4U;

#line 28
    float R4_0 = (&kernelContext_0)->globalParams_0->rot_0.Load(_S7);
    uint32_t _S8 = _S3 + 5U;

#line 29
    float R5_0 = (&kernelContext_0)->globalParams_0->rot_0.Load(_S8);
    uint32_t _S9 = _S3 + 6U;

#line 30
    float R6_0 = (&kernelContext_0)->globalParams_0->rot_0.Load(_S9);
    uint32_t _S10 = _S3 + 7U;

#line 31
    float R7_0 = (&kernelContext_0)->globalParams_0->rot_0.Load(_S10);
    uint32_t _S11 = _S3 + 8U;

#line 32
    float R8_0 = (&kernelContext_0)->globalParams_0->rot_0.Load(_S11);
    uint32_t ax_0 = (&kernelContext_0)->globalParams_0->axis_0.Load(i_0);
    bool _S12 = ax_0 == 0U;

#line 34
    float st_v00_0;

#line 34
    if(_S12)
    {

#line 34
        st_v00_0 = 1.0f;

#line 34
    }
    else
    {

#line 34
        st_v00_0 = 0.0f;

#line 34
    }
    bool _S13 = ax_0 == 1U;

#line 35
    float st_v01_0;

#line 35
    if(_S13)
    {

#line 35
        st_v01_0 = 1.0f;

#line 35
    }
    else
    {

#line 35
        st_v01_0 = 0.0f;

#line 35
    }

#line 35
    float st_v02_0;
    if(ax_0 == 2U)
    {

#line 36
        st_v02_0 = 1.0f;

#line 36
    }
    else
    {

#line 36
        st_v02_0 = 0.0f;

#line 36
    }
    float st_ru0_0 = R0_0 * st_v00_0 + R1_0 * st_v01_0 + R2_0 * st_v02_0;
    float st_ru1_0 = R3_0 * st_v00_0 + R4_0 * st_v01_0 + R5_0 * st_v02_0;
    float st_ru2_0 = R6_0 * st_v00_0 + R7_0 * st_v01_0 + R8_0 * st_v02_0;
    float st_u0_l_0 = (F32_sqrt((st_ru0_0 * st_ru0_0 + st_ru1_0 * st_ru1_0 + st_ru2_0 * st_ru2_0)));
    bool _S14 = st_u0_l_0 > 9.999999960041972e-13f;

#line 41
    float st_u00_0;

#line 41
    if(_S14)
    {

#line 41
        st_u00_0 = st_ru0_0 / st_u0_l_0;

#line 41
    }
    else
    {

#line 41
        st_u00_0 = st_ru0_0;

#line 41
    }

#line 41
    float st_u01_0;
    if(_S14)
    {

#line 42
        st_u01_0 = st_ru1_0 / st_u0_l_0;

#line 42
    }
    else
    {

#line 42
        st_u01_0 = st_ru1_0;

#line 42
    }

#line 42
    float st_u02_0;
    if(_S14)
    {

#line 43
        st_u02_0 = st_ru2_0 / st_u0_l_0;

#line 43
    }
    else
    {

#line 43
        st_u02_0 = st_ru2_0;

#line 43
    }
    bool st_fy_0 = (F32_abs((st_v01_0))) < 0.89999997615814209f;

#line 44
    float st_ref0_0;
    if(st_fy_0)
    {

#line 45
        st_ref0_0 = 0.0f;

#line 45
    }
    else
    {

#line 45
        st_ref0_0 = 1.0f;

#line 45
    }

#line 45
    float st_ref1_0;
    if(st_fy_0)
    {

#line 46
        st_ref1_0 = 1.0f;

#line 46
    }
    else
    {

#line 46
        st_ref1_0 = 0.0f;

#line 46
    }

    float st_v1r0_0 = st_v01_0 * 0.0f - st_v02_0 * st_ref1_0;
    float st_v1r1_0 = st_v02_0 * st_ref0_0 - st_v00_0 * 0.0f;
    float st_v1r2_0 = st_v00_0 * st_ref1_0 - st_v01_0 * st_ref0_0;
    float st_v1_l_0 = (F32_sqrt((st_v1r0_0 * st_v1r0_0 + st_v1r1_0 * st_v1r1_0 + st_v1r2_0 * st_v1r2_0)));
    bool _S15 = st_v1_l_0 > 9.999999960041972e-13f;

#line 52
    float st_v10_0;

#line 52
    if(_S15)
    {

#line 52
        st_v10_0 = st_v1r0_0 / st_v1_l_0;

#line 52
    }
    else
    {

#line 52
        st_v10_0 = st_v1r0_0;

#line 52
    }

#line 52
    float st_v11_0;
    if(_S15)
    {

#line 53
        st_v11_0 = st_v1r1_0 / st_v1_l_0;

#line 53
    }
    else
    {

#line 53
        st_v11_0 = st_v1r1_0;

#line 53
    }

#line 53
    float st_v12_0;
    if(_S15)
    {

#line 54
        st_v12_0 = st_v1r2_0 / st_v1_l_0;

#line 54
    }
    else
    {

#line 54
        st_v12_0 = st_v1r2_0;

#line 54
    }
    float st_v2r0_0 = st_v01_0 * st_v12_0 - st_v02_0 * st_v11_0;
    float st_v2r1_0 = st_v02_0 * st_v10_0 - st_v00_0 * st_v12_0;
    float st_v2r2_0 = st_v00_0 * st_v11_0 - st_v01_0 * st_v10_0;
    float st_v2_l_0 = (F32_sqrt((st_v2r0_0 * st_v2r0_0 + st_v2r1_0 * st_v2r1_0 + st_v2r2_0 * st_v2r2_0)));
    bool _S16 = st_v2_l_0 > 9.999999960041972e-13f;

#line 59
    float st_v20_0;

#line 59
    if(_S16)
    {

#line 59
        st_v20_0 = st_v2r0_0 / st_v2_l_0;

#line 59
    }
    else
    {

#line 59
        st_v20_0 = st_v2r0_0;

#line 59
    }

#line 59
    float st_v21_0;
    if(_S16)
    {

#line 60
        st_v21_0 = st_v2r1_0 / st_v2_l_0;

#line 60
    }
    else
    {

#line 60
        st_v21_0 = st_v2r1_0;

#line 60
    }

#line 60
    float st_v22_0;
    if(_S16)
    {

#line 61
        st_v22_0 = st_v2r2_0 / st_v2_l_0;

#line 61
    }
    else
    {

#line 61
        st_v22_0 = st_v2r2_0;

#line 61
    }
    float st_ax0_0 = st_v01_0 * st_u02_0 - st_v02_0 * st_u01_0;
    float st_ax1_0 = st_v02_0 * st_u00_0 - st_v00_0 * st_u02_0;
    float st_ax2_0 = st_v00_0 * st_u01_0 - st_v01_0 * st_u00_0;
    float st_sn_0 = (F32_sqrt((st_ax0_0 * st_ax0_0 + st_ax1_0 * st_ax1_0 + st_ax2_0 * st_ax2_0)));
    float st_cs_0 = st_v00_0 * st_u00_0 + st_v01_0 * st_u01_0 + st_v02_0 * st_u02_0;
    float _S17 = (F32_max((st_sn_0), (9.99999971718068537e-10f)));

#line 67
    float st_k0_0 = st_ax0_0 / _S17;
    float st_k1_0 = st_ax1_0 / _S17;
    float st_k2_0 = st_ax2_0 / _S17;

#line 74
    float _S18 = 1.0f - st_cs_0;

#line 74
    float _S19 = (st_k0_0 * st_v10_0 + st_k1_0 * st_v11_0 + st_k2_0 * st_v12_0) * _S18;

#line 74
    float _S20 = st_cs_0 * st_v10_0 + st_sn_0 * (st_k1_0 * st_v12_0 - st_k2_0 * st_v11_0) + _S19 * st_k0_0;
    float _S21 = st_cs_0 * st_v11_0 + st_sn_0 * (st_k2_0 * st_v10_0 - st_k0_0 * st_v12_0) + _S19 * st_k1_0;
    float _S22 = st_cs_0 * st_v12_0 + st_sn_0 * (st_k0_0 * st_v11_0 - st_k1_0 * st_v10_0) + _S19 * st_k2_0;

#line 81
    float _S23 = (st_k0_0 * st_v20_0 + st_k1_0 * st_v21_0 + st_k2_0 * st_v22_0) * _S18;

#line 81
    float _S24 = st_cs_0 * st_v20_0 + st_sn_0 * (st_k1_0 * st_v22_0 - st_k2_0 * st_v21_0) + _S23 * st_k0_0;
    float _S25 = st_cs_0 * st_v21_0 + st_sn_0 * (st_k2_0 * st_v20_0 - st_k0_0 * st_v22_0) + _S23 * st_k1_0;
    float _S26 = st_cs_0 * st_v22_0 + st_sn_0 * (st_k0_0 * st_v21_0 - st_k1_0 * st_v20_0) + _S23 * st_k2_0;

#line 83
    float st_c10_0;

#line 83
    float st_c20_0;

#line 83
    float st_c11_0;

#line 83
    float st_c21_0;

#line 83
    float st_c12_0;

#line 83
    float st_c22_0;
    if(st_sn_0 < 9.99999971718068537e-10f)
    {

#line 85
        if(st_cs_0 > 0.0f)
        {

#line 85
            st_c10_0 = st_v10_0;

#line 85
            st_c20_0 = st_v20_0;

#line 85
            st_c11_0 = st_v11_0;

#line 85
            st_c21_0 = st_v21_0;

#line 85
            st_c12_0 = st_v12_0;

#line 85
            st_c22_0 = st_v22_0;

#line 85
        }
        else
        {

#line 93
            float _S27 = (F32_abs((st_u00_0)));

#line 93
            bool st_px_0;

#line 93
            if(_S27 <= (F32_abs((st_u01_0))))
            {

#line 93
                st_px_0 = _S27 <= (F32_abs((st_u02_0)));

#line 93
            }
            else
            {

#line 93
                st_px_0 = false;

#line 93
            }
            if(st_px_0)
            {

#line 94
                st_c10_0 = 1.0f;

#line 94
            }
            else
            {

#line 94
                st_c10_0 = 0.0f;

#line 94
            }
            if(st_px_0)
            {

#line 95
                st_c20_0 = 0.0f;

#line 95
            }
            else
            {

#line 95
                st_c20_0 = 1.0f;

#line 95
            }

            float st_p10_0 = st_u01_0 * 0.0f - st_u02_0 * st_c20_0;
            float st_p11_0 = st_u02_0 * st_c10_0 - st_u00_0 * 0.0f;
            float st_p12_0 = st_u00_0 * st_c20_0 - st_u01_0 * st_c10_0;
            float st_p1n_l_0 = (F32_sqrt((st_p10_0 * st_p10_0 + st_p11_0 * st_p11_0 + st_p12_0 * st_p12_0)));
            bool _S28 = st_p1n_l_0 > 9.999999960041972e-13f;

#line 101
            if(_S28)
            {

#line 101
                st_c11_0 = st_p10_0 / st_p1n_l_0;

#line 101
            }
            else
            {

#line 101
                st_c11_0 = st_p10_0;

#line 101
            }
            if(_S28)
            {

#line 102
                st_c21_0 = st_p11_0 / st_p1n_l_0;

#line 102
            }
            else
            {

#line 102
                st_c21_0 = st_p11_0;

#line 102
            }
            if(_S28)
            {

#line 103
                st_c12_0 = st_p12_0 / st_p1n_l_0;

#line 103
            }
            else
            {

#line 103
                st_c12_0 = st_p12_0;

#line 103
            }
            float st_p20_0 = st_u01_0 * st_c12_0 - st_u02_0 * st_c21_0;
            float st_p21_0 = st_u02_0 * st_c11_0 - st_u00_0 * st_c12_0;
            float st_p22_0 = st_u00_0 * st_c21_0 - st_u01_0 * st_c11_0;
            float st_p2n_l_0 = (F32_sqrt((st_p20_0 * st_p20_0 + st_p21_0 * st_p21_0 + st_p22_0 * st_p22_0)));
            bool _S29 = st_p2n_l_0 > 9.999999960041972e-13f;

#line 108
            if(_S29)
            {

#line 108
                st_c22_0 = st_p20_0 / st_p2n_l_0;

#line 108
            }
            else
            {

#line 108
                st_c22_0 = st_p20_0;

#line 108
            }

#line 108
            float st_p2n1_0;
            if(_S29)
            {

#line 109
                st_p2n1_0 = st_p21_0 / st_p2n_l_0;

#line 109
            }
            else
            {

#line 109
                st_p2n1_0 = st_p21_0;

#line 109
            }

#line 109
            float st_p2n2_0;
            if(_S29)
            {

#line 110
                st_p2n2_0 = st_p22_0 / st_p2n_l_0;

#line 110
            }
            else
            {

#line 110
                st_p2n2_0 = st_p22_0;

#line 110
            }

#line 110
            st_c10_0 = st_c11_0;

#line 110
            st_c20_0 = st_c22_0;

#line 110
            st_c11_0 = st_c21_0;

#line 110
            st_c21_0 = st_p2n1_0;

#line 110
            st_c22_0 = st_p2n2_0;

#line 85
        }

#line 84
    }
    else
    {

#line 84
        st_c10_0 = _S20;

#line 84
        st_c20_0 = _S24;

#line 84
        st_c11_0 = _S21;

#line 84
        st_c21_0 = _S25;

#line 84
        st_c12_0 = _S22;

#line 84
        st_c22_0 = _S26;

#line 84
    }

#line 119
    float st_R00_0 = st_u00_0 * st_v00_0 + st_c10_0 * st_v10_0 + st_c20_0 * st_v20_0;
    float st_R01_0 = st_u00_0 * st_v01_0 + st_c10_0 * st_v11_0 + st_c20_0 * st_v21_0;
    float st_R02_0 = st_u00_0 * st_v02_0 + st_c10_0 * st_v12_0 + st_c20_0 * st_v22_0;
    float st_R03_0 = st_u01_0 * st_v00_0 + st_c11_0 * st_v10_0 + st_c21_0 * st_v20_0;
    float st_R04_0 = st_u01_0 * st_v01_0 + st_c11_0 * st_v11_0 + st_c21_0 * st_v21_0;
    float st_R05_0 = st_u01_0 * st_v02_0 + st_c11_0 * st_v12_0 + st_c21_0 * st_v22_0;
    float st_R06_0 = st_u02_0 * st_v00_0 + st_c12_0 * st_v10_0 + st_c22_0 * st_v20_0;
    float st_R07_0 = st_u02_0 * st_v01_0 + st_c12_0 * st_v11_0 + st_c22_0 * st_v21_0;
    float st_R08_0 = st_u02_0 * st_v02_0 + st_c12_0 * st_v12_0 + st_c22_0 * st_v22_0;
    float st_T0_0 = st_R00_0 * R0_0 + st_R03_0 * R3_0 + st_R06_0 * R6_0;
    float st_T1_0 = st_R00_0 * R1_0 + st_R03_0 * R4_0 + st_R06_0 * R7_0;
    float st_T2_0 = st_R00_0 * R2_0 + st_R03_0 * R5_0 + st_R06_0 * R8_0;
    float st_T3_0 = st_R01_0 * R0_0 + st_R04_0 * R3_0 + st_R07_0 * R6_0;
    float st_T4_0 = st_R01_0 * R1_0 + st_R04_0 * R4_0 + st_R07_0 * R7_0;
    float st_T5_0 = st_R01_0 * R2_0 + st_R04_0 * R5_0 + st_R07_0 * R8_0;
    float st_T6_0 = st_R02_0 * R0_0 + st_R05_0 * R3_0 + st_R08_0 * R6_0;
    float st_T7_0 = st_R02_0 * R1_0 + st_R05_0 * R4_0 + st_R08_0 * R7_0;
    float st_T8_0 = st_R02_0 * R2_0 + st_R05_0 * R5_0 + st_R08_0 * R8_0;
    if(_S12)
    {

#line 137
        st_v00_0 = st_T7_0 - st_T5_0;

#line 137
    }
    else
    {

#line 137
        if(_S13)
        {

#line 137
            st_v00_0 = st_T2_0 - st_T6_0;

#line 137
        }
        else
        {

#line 137
            st_v00_0 = st_T3_0 - st_T1_0;

#line 137
        }

#line 137
    }

#line 137
    if(_S12)
    {

#line 137
        st_v01_0 = st_T4_0 + st_T8_0;

#line 137
    }
    else
    {

#line 137
        if(_S13)
        {

#line 137
            st_v01_0 = st_T8_0 + st_T0_0;

#line 137
        }
        else
        {

#line 137
            st_v01_0 = st_T0_0 + st_T4_0;

#line 137
        }

#line 137
    }

#line 137
    float st_0 = (F32_atan2((st_v00_0), (st_v01_0)));
    *(&((&kernelContext_0)->globalParams_0->swing_0)[_S3]) = st_R00_0;
    *(&((&kernelContext_0)->globalParams_0->swing_0)[_S4]) = st_R01_0;
    *(&((&kernelContext_0)->globalParams_0->swing_0)[_S5]) = st_R02_0;
    *(&((&kernelContext_0)->globalParams_0->swing_0)[_S6]) = st_R03_0;
    *(&((&kernelContext_0)->globalParams_0->swing_0)[_S7]) = st_R04_0;
    *(&((&kernelContext_0)->globalParams_0->swing_0)[_S8]) = st_R05_0;
    *(&((&kernelContext_0)->globalParams_0->swing_0)[_S9]) = st_R06_0;
    *(&((&kernelContext_0)->globalParams_0->swing_0)[_S10]) = st_R07_0;
    *(&((&kernelContext_0)->globalParams_0->swing_0)[_S11]) = st_R08_0;
    *(&((&kernelContext_0)->globalParams_0->twist_0)[_S3]) = st_T0_0;
    *(&((&kernelContext_0)->globalParams_0->twist_0)[_S4]) = st_T1_0;
    *(&((&kernelContext_0)->globalParams_0->twist_0)[_S5]) = st_T2_0;
    *(&((&kernelContext_0)->globalParams_0->twist_0)[_S6]) = st_T3_0;
    *(&((&kernelContext_0)->globalParams_0->twist_0)[_S7]) = st_T4_0;
    *(&((&kernelContext_0)->globalParams_0->twist_0)[_S8]) = st_T5_0;
    *(&((&kernelContext_0)->globalParams_0->twist_0)[_S9]) = st_T6_0;
    *(&((&kernelContext_0)->globalParams_0->twist_0)[_S10]) = st_T7_0;
    *(&((&kernelContext_0)->globalParams_0->twist_0)[_S11]) = st_T8_0;
    *(&((&kernelContext_0)->globalParams_0->angle_0)[i_0]) = st_0;
    return;
}

// [numthreads(64, 1, 1)]
SLANG_PRELUDE_EXPORT
void main_0_Thread(ComputeThreadVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    _main_0(varyingInput, entryPointParams, globalParams);
}
// [numthreads(64, 1, 1)]
SLANG_PRELUDE_EXPORT
void main_0_Group(ComputeVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    ComputeThreadVaryingInput threadInput = {};
    threadInput.groupID = varyingInput->startGroupID;
    for (uint32_t x = 0; x < 64; ++x)
    {
        threadInput.groupThreadID.x = x;
        _main_0(&threadInput, entryPointParams, globalParams);
    }
}
// [numthreads(64, 1, 1)]
SLANG_PRELUDE_EXPORT
void main_0(ComputeVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    ComputeVaryingInput vi = *varyingInput;
    ComputeVaryingInput groupVaryingInput = {};
    for (uint32_t z = vi.startGroupID.z; z < vi.endGroupID.z; ++z)
    {
        groupVaryingInput.startGroupID.z = z;
        for (uint32_t y = vi.startGroupID.y; y < vi.endGroupID.y; ++y)
        {
            groupVaryingInput.startGroupID.y = y;
            for (uint32_t x = vi.startGroupID.x; x < vi.endGroupID.x; ++x)
            {
                groupVaryingInput.startGroupID.x = x;
                main_0_Group(&groupVaryingInput, entryPointParams, globalParams);
            }
        }
    }
}
