#include "slang_rt/slang-cpp-prelude.h"

#ifdef SLANG_PRELUDE_NAMESPACE
using namespace SLANG_PRELUDE_NAMESPACE;
#endif


#line 119 "E:/sinew-moved/viz_native/../spec/slang/pheno_skeleton_fit.slang"
struct GlobalParams_0
{
    StructuredBuffer<float> v0_0;
    StructuredBuffer<float> bsh_0;
    StructuredBuffer<float> bw_0;
    StructuredBuffer<float> jpos_0;
    StructuredBuffer<float> sw_0;
    StructuredBuffer<int32_t> parents_0;
    RWStructuredBuffer<float> outbw_0;
};


#line 119
struct KernelContext_0
{
    GlobalParams_0* globalParams_0;
};


#line 8919 "hlsl.meta.slang"
static float clamp_0(float x_0, float minBound_0, float maxBound_0)
{

#line 8931
    return (F32_min(((F32_max((x_0), (minBound_0)))), (maxBound_0)));
}


#line 9865
static float dot_0(Vector<float, 3>  x_1, Vector<float, 3>  y_0)
{

#line 9865
    int32_t i_0 = int(0);

#line 9865
    float result_0 = 0.0f;

#line 9884
    for(;;)
    {

#line 9884
        if(i_0 < int(3))
        {
        }
        else
        {

#line 9884
            break;
        }

#line 9885
        float result_1 = result_0 + _slang_vector_get_element(x_1, i_0) * _slang_vector_get_element(y_0, i_0);

#line 9884
        i_0 = i_0 + int(1);

#line 9884
        result_0 = result_1;

#line 9884
    }

    return result_0;
}


#line 12120
static float length_0(Vector<float, 3>  x_2)
{

#line 12132
    return (F32_sqrt((dot_0(x_2, x_2))));
}


#line 14 "E:/sinew-moved/viz_native/../spec/slang/pheno_skeleton_fit.slang"
static Vector<float, 3>  cr_0(Vector<float, 3>  a_0, Vector<float, 3>  b_0)
{

#line 14
    float _S1 = a_0.y;

#line 14
    float _S2 = b_0.z;

#line 14
    float _S3 = a_0.z;

#line 14
    float _S4 = b_0.y;

#line 14
    float _S5 = b_0.x;

#line 14
    float _S6 = a_0.x;

#line 14
    return Vector<float, 3> (_S1 * _S2 - _S3 * _S4, _S3 * _S5 - _S6 * _S2, _S6 * _S4 - _S1 * _S5);
}


#line 16
static void mul9_0(FixedArray<float, 9>  * A_0, FixedArray<float, 9>  * B_0, FixedArray<float, 9>  * C_0)
{

#line 16
    int32_t r_0 = int(0);
    for(;;)
    {

#line 17
        if(r_0 < int(3))
        {
        }
        else
        {

#line 17
            break;
        }

#line 17
        int32_t c_0 = int(0);

#line 17
        for(;;)
        {

#line 17
            if(c_0 < int(3))
            {
            }
            else
            {

#line 17
                break;
            }

#line 18
            int32_t _S7 = r_0 * int(3);

#line 18
            (*C_0)[_S7 + c_0] = (*A_0)[_S7] * (*B_0)[c_0] + (*A_0)[_S7 + int(1)] * (*B_0)[int(3) + c_0] + (*A_0)[_S7 + int(2)] * (*B_0)[int(6) + c_0];

#line 17
            c_0 = c_0 + int(1);

#line 17
        }

#line 17
        r_0 = r_0 + int(1);

#line 17
    }

    return;
}


#line 20
static void rodrigues_0(Vector<float, 3>  a_1, Vector<float, 3>  b_1, FixedArray<float, 9>  * R_0)
{
    Vector<float, 3>  au_0 = a_1 / (Vector<float, 3> )(F32_max((length_0(a_1)), (9.99999993922529029e-09f)));

#line 22
    Vector<float, 3>  bu_0 = b_1 / (Vector<float, 3> )(F32_max((length_0(b_1)), (9.99999993922529029e-09f)));
    float d_0 = clamp_0(dot_0(au_0, bu_0), -1.0f, 1.0f);

#line 23
    int32_t r_1;

#line 23
    float _S8;
    if(d_0 < -0.99999898672103882f)
    {

#line 24
        Vector<float, 3>  w_0;
        if((F32_abs((bu_0.x))) > 0.60000002384185791f)
        {

#line 25
            w_0 = Vector<float, 3> (0.0f, 1.0f, 0.0f);

#line 25
        }
        else
        {

#line 25
            w_0 = Vector<float, 3> (1.0f, 0.0f, 0.0f);

#line 25
        }
        Vector<float, 3>  x_3 = cr_0(bu_0, w_0);

#line 26
        Vector<float, 3>  x_4 = x_3 / (Vector<float, 3> )length_0(x_3);

#line 26
        FixedArray<float, 3>  _S9 = { x_4.x, x_4.y, x_4.z };

#line 26
        r_1 = int(0);
        for(;;)
        {

#line 27
            if(r_1 < int(3))
            {
            }
            else
            {

#line 27
                break;
            }

#line 27
            int32_t c_1 = int(0);

#line 27
            for(;;)
            {

#line 27
                if(c_1 < int(3))
                {
                }
                else
                {

#line 27
                    break;
                }

#line 27
                float _S10 = 2.0f * _S9[r_1] * _S9[c_1];

#line 27
                if(r_1 == c_1)
                {

#line 27
                    _S8 = 1.0f;

#line 27
                }
                else
                {

#line 27
                    _S8 = 0.0f;

#line 27
                }

#line 27
                (*R_0)[r_1 * int(3) + c_1] = _S10 - _S8;

#line 27
                c_1 = c_1 + int(1);

#line 27
            }

#line 27
            r_1 = r_1 + int(1);

#line 27
        }
        return;
    }
    Vector<float, 3>  v_0 = cr_0(bu_0, au_0);
    FixedArray<float, 9>  K_0;

#line 31
    K_0[int(0)] = 0.0f;

#line 31
    float _S11 = v_0.z;

#line 31
    K_0[int(1)] = - _S11;

#line 31
    float _S12 = v_0.y;

#line 31
    K_0[int(2)] = _S12;

#line 31
    K_0[int(3)] = _S11;

#line 31
    K_0[int(4)] = 0.0f;

#line 31
    float _S13 = v_0.x;

#line 31
    K_0[int(5)] = - _S13;

#line 31
    K_0[int(6)] = - _S12;

#line 31
    K_0[int(7)] = _S13;

#line 31
    K_0[int(8)] = 0.0f;

#line 31
    FixedArray<float, 9>  _S14 = K_0;

#line 31
    FixedArray<float, 9>  _S15 = K_0;
    FixedArray<float, 9>  KK_0;

#line 32
    mul9_0(&_S14, &_S15, &KK_0);

#line 32
    float _S16 = 1.0f / (1.0f + d_0);

#line 32
    r_1 = int(0);
    for(;;)
    {

#line 33
        if(r_1 < int(9))
        {
        }
        else
        {

#line 33
            break;
        }

#line 33
        if((r_1 % int(4)) == int(0))
        {

#line 33
            _S8 = 1.0f;

#line 33
        }
        else
        {

#line 33
            _S8 = 0.0f;

#line 33
        }

#line 33
        (*R_0)[r_1] = _S8 + K_0[r_1] + _S16 * KK_0[r_1];

#line 33
        r_1 = r_1 + int(1);

#line 33
    }
    return;
}


#line 15
static float det9_0(FixedArray<float, 9>  * m_0)
{

#line 15
    return (*m_0)[int(0)] * ((*m_0)[int(4)] * (*m_0)[int(8)] - (*m_0)[int(5)] * (*m_0)[int(7)]) - (*m_0)[int(1)] * ((*m_0)[int(3)] * (*m_0)[int(8)] - (*m_0)[int(5)] * (*m_0)[int(6)]) + (*m_0)[int(2)] * ((*m_0)[int(3)] * (*m_0)[int(7)] - (*m_0)[int(4)] * (*m_0)[int(6)]);
}


#line 47
static void regularize_0(FixedArray<float, 9>  * H_0)
{

#line 47
    float scale_0 = 0.0f;

#line 47
    int32_t r_2 = int(0);

    for(;;)
    {

#line 49
        if(r_2 < int(3))
        {
        }
        else
        {

#line 49
            break;
        }

#line 49
        int32_t _S17 = r_2 * int(3);

#line 49
        float _S18 = (F32_max((scale_0), ((F32_abs(((*H_0)[_S17]))) + (F32_abs(((*H_0)[_S17 + int(1)]))) + (F32_abs(((*H_0)[_S17 + int(2)]))))));

#line 49
        int32_t _S19 = r_2 + int(1);

#line 49
        scale_0 = _S18;

#line 49
        r_2 = _S19;

#line 49
    }
    if(scale_0 < 9.99999993922529029e-09f)
    {

#line 50
        scale_0 = 9.99999993922529029e-09f;

#line 50
    }

#line 50
    FixedArray<float, 9>  _S20 = *H_0;

#line 50
    float _S21 = det9_0(&_S20);

    float add_0 = 0.05000000074505806f * clamp_0((9.99999997475242708e-07f - (F32_abs((_S21))) / (scale_0 * scale_0 * scale_0)) / 9.99999997475242708e-07f, 0.0f, 1.0f) * scale_0;
    (*H_0)[int(0)] = (*H_0)[int(0)] + add_0;

#line 53
    (*H_0)[int(4)] = (*H_0)[int(4)] + add_0;

#line 53
    (*H_0)[int(8)] = (*H_0)[int(8)] + add_0;
    return;
}


#line 35
static void ns30_0(FixedArray<float, 9>  * H_1, FixedArray<float, 9>  * R_1)
{

#line 35
    int32_t b_2;

#line 35
    float mrs_0 = 0.0f;

#line 35
    int32_t r_3 = int(0);

    for(;;)
    {

#line 37
        if(r_3 < int(3))
        {
        }
        else
        {

#line 37
            break;
        }

#line 37
        int32_t _S22 = r_3 * int(3);

#line 37
        float _S23 = (F32_max((mrs_0), ((F32_abs(((*H_1)[_S22]))) + (F32_abs(((*H_1)[_S22 + int(1)]))) + (F32_abs(((*H_1)[_S22 + int(2)]))))));

#line 37
        int32_t _S24 = r_3 + int(1);

#line 37
        mrs_0 = _S23;

#line 37
        r_3 = _S24;

#line 37
    }

#line 37
    int32_t i_1 = int(0);
    for(;;)
    {

#line 38
        if(i_1 < int(9))
        {
        }
        else
        {

#line 38
            break;
        }

#line 38
        (*R_1)[i_1] = (*H_1)[i_1] / (mrs_0 + 9.99999993922529029e-09f);

#line 38
        i_1 = i_1 + int(1);

#line 38
    }

#line 38
    int32_t it_0 = int(0);
    for(;;)
    {

#line 39
        if(it_0 < int(30))
        {
        }
        else
        {

#line 39
            break;
        }

#line 40
        FixedArray<float, 9>  RtR_0;

#line 40
        int32_t a_2 = int(0);
        for(;;)
        {

#line 41
            if(a_2 < int(3))
            {
            }
            else
            {

#line 41
                break;
            }

#line 41
            b_2 = int(0);

#line 41
            for(;;)
            {

#line 41
                if(b_2 < int(3))
                {
                }
                else
                {

#line 41
                    break;
                }

#line 41
                RtR_0[a_2 * int(3) + b_2] = (*R_1)[a_2] * (*R_1)[b_2] + (*R_1)[int(3) + a_2] * (*R_1)[int(3) + b_2] + (*R_1)[int(6) + a_2] * (*R_1)[int(6) + b_2];

#line 41
                b_2 = b_2 + int(1);

#line 41
            }

#line 41
            a_2 = a_2 + int(1);

#line 41
        }
        FixedArray<float, 9>  term_0;

#line 42
        b_2 = int(0);

#line 42
        for(;;)
        {

#line 42
            if(b_2 < int(9))
            {
            }
            else
            {

#line 42
                break;
            }

#line 42
            if((b_2 % int(4)) == int(0))
            {

#line 42
                mrs_0 = 3.0f;

#line 42
            }
            else
            {

#line 42
                mrs_0 = 0.0f;

#line 42
            }

#line 42
            term_0[b_2] = mrs_0 - RtR_0[b_2];

#line 42
            b_2 = b_2 + int(1);

#line 42
        }

#line 42
        FixedArray<float, 9>  _S25 = *R_1;

#line 42
        FixedArray<float, 9>  _S26 = term_0;
        FixedArray<float, 9>  Rn_0;

#line 43
        mul9_0(&_S25, &_S26, &Rn_0);

#line 43
        int32_t k_0 = int(0);

#line 43
        for(;;)
        {

#line 43
            if(k_0 < int(9))
            {
            }
            else
            {

#line 43
                break;
            }

#line 43
            (*R_1)[k_0] = Rn_0[k_0] * 0.5f;

#line 43
            k_0 = k_0 + int(1);

#line 43
        }

#line 39
        it_0 = it_0 + int(1);

#line 39
    }

#line 39
    FixedArray<float, 9>  _S27 = *R_1;

#line 39
    float _S28 = det9_0(&_S27);

#line 45
    if(_S28 < 0.0f)
    {

#line 45
        (*R_1)[int(2)] = - (*R_1)[int(2)];

#line 45
        (*R_1)[int(5)] = - (*R_1)[int(5)];

#line 45
        (*R_1)[int(8)] = - (*R_1)[int(8)];

#line 45
    }
    return;
}


#line 55
static bool valid9_0(FixedArray<float, 9>  * R_2)
{

#line 55
    float _S29 = det9_0(R_2);

#line 55
    bool _S30;

    if(_S29 > 0.0f)
    {

#line 57
        _S30 = (F32_abs((_S29 - 1.0f))) <= 0.00999999977648258f;

#line 57
    }
    else
    {

#line 57
        _S30 = false;

#line 57
    }

#line 57
    if(!_S30)
    {

#line 57
        return false;
    }

#line 57
    float e_0 = 0.0f;

#line 57
    int32_t i_2 = int(0);

    for(;;)
    {

#line 59
        if(i_2 < int(3))
        {
        }
        else
        {

#line 59
            break;
        }

#line 59
        int32_t j_0 = int(0);

#line 59
        for(;;)
        {

#line 59
            if(j_0 < int(3))
            {
            }
            else
            {

#line 59
                break;
            }

#line 59
            float _S31 = (*R_2)[i_2] * (*R_2)[j_0] + (*R_2)[int(3) + i_2] * (*R_2)[int(3) + j_0] + (*R_2)[int(6) + i_2] * (*R_2)[int(6) + j_0];

#line 59
            float _S32;

#line 59
            if(i_2 == j_0)
            {

#line 59
                _S32 = 1.0f;

#line 59
            }
            else
            {

#line 59
                _S32 = 0.0f;

#line 59
            }

#line 59
            float _S33 = (F32_max((e_0), ((F32_abs((_S31 - _S32))))));

#line 59
            int32_t _S34 = j_0 + int(1);

#line 59
            e_0 = _S33;

#line 59
            j_0 = _S34;

#line 59
        }

#line 59
        i_2 = i_2 + int(1);

#line 59
    }
    return e_0 <= 0.00999999977648258f;
}


#line 62
static void jacobi9_0(FixedArray<float, 9>  * Ain_0, FixedArray<float, 3>  * w_1, FixedArray<float, 9>  * V_0)
{

#line 63
    FixedArray<float, 9>  A_1;

#line 63
    int32_t i_3 = int(0);

#line 63
    for(;;)
    {

#line 63
        if(i_3 < int(9))
        {
        }
        else
        {

#line 63
            break;
        }

#line 63
        A_1[i_3] = (*Ain_0)[i_3];

#line 63
        i_3 = i_3 + int(1);

#line 63
    }

#line 63
    i_3 = int(0);
    for(;;)
    {

#line 64
        if(i_3 < int(9))
        {
        }
        else
        {

#line 64
            break;
        }

#line 64
        float _S35;

#line 64
        if((i_3 % int(4)) == int(0))
        {

#line 64
            _S35 = 1.0f;

#line 64
        }
        else
        {

#line 64
            _S35 = 0.0f;

#line 64
        }

#line 64
        (*V_0)[i_3] = _S35;

#line 64
        i_3 = i_3 + int(1);

#line 64
    }
    FixedArray<int32_t, 3>  pp_0;

#line 65
    pp_0[int(0)] = int(0);

#line 65
    pp_0[int(1)] = int(0);

#line 65
    pp_0[int(2)] = int(1);

#line 65
    FixedArray<int32_t, 3>  qq_0;

#line 65
    qq_0[int(0)] = int(1);

#line 65
    qq_0[int(1)] = int(2);

#line 65
    qq_0[int(2)] = int(2);

#line 65
    int32_t sweep_0 = int(0);
    for(;;)
    {

#line 66
        if(sweep_0 < int(50))
        {
        }
        else
        {

#line 66
            break;
        }

#line 67
        if(((F32_abs((A_1[int(1)]))) + (F32_abs((A_1[int(2)]))) + (F32_abs((A_1[int(5)])))) < 9.99999968265522539e-21f)
        {

#line 67
            break;
        }

#line 67
        int32_t k_1 = int(0);
        for(;;)
        {

#line 68
            if(k_1 < int(3))
            {
            }
            else
            {

#line 68
                break;
            }

#line 69
            int32_t p_0 = pp_0[k_1];

#line 69
            int32_t q_0 = qq_0[k_1];

#line 69
            int32_t _S36 = pp_0[k_1] * int(3);

#line 69
            float apq_0 = A_1[_S36 + qq_0[k_1]];
            if((F32_abs((A_1[_S36 + qq_0[k_1]]))) < 9.99999968265522539e-21f)
            {

#line 70
                k_1 = k_1 + int(1);

#line 68
                continue;
            }

            int32_t _S37 = q_0 * int(3);

#line 71
            float phi_0 = 0.5f * (F32_atan2((2.0f * apq_0), (A_1[_S37 + q_0] - A_1[_S36 + p_0])));

#line 71
            float _S38 = (F32_cos((phi_0)));

#line 71
            float _S39 = (F32_sin((phi_0)));

#line 71
            i_3 = int(0);
            for(;;)
            {

#line 72
                if(i_3 < int(3))
                {
                }
                else
                {

#line 72
                    break;
                }

#line 72
                int32_t _S40 = i_3 * int(3);

#line 72
                float aip_0 = A_1[_S40 + p_0];

#line 72
                float aiq_0 = A_1[_S40 + q_0];

#line 72
                A_1[_S40 + p_0] = _S38 * A_1[_S40 + p_0] - _S39 * A_1[_S40 + q_0];

#line 72
                A_1[_S40 + q_0] = _S39 * aip_0 + _S38 * aiq_0;

#line 72
                i_3 = i_3 + int(1);

#line 72
            }

#line 72
            int32_t i_4 = int(0);
            for(;;)
            {

#line 73
                if(i_4 < int(3))
                {
                }
                else
                {

#line 73
                    break;
                }

#line 73
                float api_0 = A_1[_S36 + i_4];

#line 73
                float aqi_0 = A_1[_S37 + i_4];

#line 73
                A_1[_S36 + i_4] = _S38 * A_1[_S36 + i_4] - _S39 * A_1[_S37 + i_4];

#line 73
                A_1[_S37 + i_4] = _S39 * api_0 + _S38 * aqi_0;

#line 73
                i_4 = i_4 + int(1);

#line 73
            }

#line 73
            int32_t i_5 = int(0);
            for(;;)
            {

#line 74
                if(i_5 < int(3))
                {
                }
                else
                {

#line 74
                    break;
                }

#line 74
                int32_t _S41 = i_5 * int(3);

#line 74
                float vip_0 = (*V_0)[_S41 + p_0];

#line 74
                float viq_0 = (*V_0)[_S41 + q_0];

#line 74
                (*V_0)[_S41 + p_0] = _S38 * (*V_0)[_S41 + p_0] - _S39 * (*V_0)[_S41 + q_0];

#line 74
                (*V_0)[_S41 + q_0] = _S39 * vip_0 + _S38 * viq_0;

#line 74
                i_5 = i_5 + int(1);

#line 74
            }

#line 68
            k_1 = k_1 + int(1);

#line 68
        }

#line 66
        sweep_0 = sweep_0 + int(1);

#line 66
    }

#line 77
    (*w_1)[int(0)] = A_1[int(0)];

#line 77
    (*w_1)[int(1)] = A_1[int(4)];

#line 77
    (*w_1)[int(2)] = A_1[int(8)];
    return;
}


#line 79
static void kabsch_0(FixedArray<float, 9>  * H_2, FixedArray<float, 9>  * R_3)
{

#line 79
    int32_t j_1;

#line 79
    int32_t b_3;

#line 79
    float sc_0;
    FixedArray<float, 9>  S_0;

#line 80
    int32_t i_6 = int(0);

#line 80
    for(;;)
    {

#line 80
        if(i_6 < int(3))
        {
        }
        else
        {

#line 80
            break;
        }

#line 80
        j_1 = int(0);

#line 80
        for(;;)
        {

#line 80
            if(j_1 < int(3))
            {
            }
            else
            {

#line 80
                break;
            }

#line 80
            S_0[i_6 * int(3) + j_1] = (*H_2)[i_6] * (*H_2)[j_1] + (*H_2)[int(3) + i_6] * (*H_2)[int(3) + j_1] + (*H_2)[int(6) + i_6] * (*H_2)[int(6) + j_1];

#line 80
            j_1 = j_1 + int(1);

#line 80
        }

#line 80
        i_6 = i_6 + int(1);

#line 80
    }

#line 80
    FixedArray<float, 9>  _S42 = S_0;
    FixedArray<float, 3>  w_2;

#line 81
    FixedArray<float, 9>  Vc_0;

#line 81
    jacobi9_0(&_S42, &w_2, &Vc_0);
    FixedArray<int32_t, 3>  ord_0;

#line 82
    ord_0[int(0)] = int(0);

#line 82
    ord_0[int(1)] = int(1);

#line 82
    ord_0[int(2)] = int(2);

#line 82
    int32_t a_3 = int(0);
    for(;;)
    {

#line 83
        if(a_3 < int(3))
        {
        }
        else
        {

#line 83
            break;
        }

#line 83
        int32_t _S43 = a_3 + int(1);

#line 83
        b_3 = _S43;

#line 83
        for(;;)
        {

#line 83
            if(b_3 < int(3))
            {
            }
            else
            {

#line 83
                break;
            }

#line 83
            if((w_2[ord_0[b_3]]) > w_2[ord_0[a_3]])
            {

#line 83
                int32_t t_0 = ord_0[a_3];

#line 83
                ord_0[a_3] = ord_0[b_3];

#line 83
                ord_0[b_3] = t_0;

#line 83
            }

#line 83
            b_3 = b_3 + int(1);

#line 83
        }

#line 83
        a_3 = _S43;

#line 83
    }
    FixedArray<float, 9>  V_1;

#line 84
    FixedArray<float, 3>  sig_0;

#line 84
    b_3 = int(0);
    for(;;)
    {

#line 85
        if(b_3 < int(3))
        {
        }
        else
        {

#line 85
            break;
        }

#line 85
        sig_0[b_3] = (F32_sqrt(((F32_max((w_2[ord_0[b_3]]), (0.0f))))));

#line 85
        j_1 = int(0);

#line 85
        for(;;)
        {

#line 85
            if(j_1 < int(3))
            {
            }
            else
            {

#line 85
                break;
            }

#line 85
            int32_t _S44 = j_1 * int(3);

#line 85
            V_1[_S44 + b_3] = Vc_0[_S44 + ord_0[b_3]];

#line 85
            j_1 = j_1 + int(1);

#line 85
        }

#line 85
        b_3 = b_3 + int(1);

#line 85
    }
    FixedArray<float, 9>  U_0;

#line 86
    b_3 = int(0);
    for(;;)
    {

#line 87
        if(b_3 < int(3))
        {
        }
        else
        {

#line 87
            break;
        }

#line 88
        int32_t _S45 = int(3) + b_3;

#line 88
        int32_t _S46 = int(6) + b_3;

#line 88
        float _S47 = (*H_2)[int(0)] * V_1[b_3] + (*H_2)[int(1)] * V_1[_S45] + (*H_2)[int(2)] * V_1[_S46];

#line 88
        float _S48 = (*H_2)[int(3)] * V_1[b_3] + (*H_2)[int(4)] * V_1[_S45] + (*H_2)[int(5)] * V_1[_S46];

#line 88
        float _S49 = (*H_2)[int(6)] * V_1[b_3] + (*H_2)[int(7)] * V_1[_S45] + (*H_2)[int(8)] * V_1[_S46];
        if((sig_0[b_3]) > 9.999999960041972e-13f)
        {

#line 89
            sc_0 = 1.0f / sig_0[b_3];

#line 89
        }
        else
        {

#line 89
            sc_0 = 0.0f;

#line 89
        }

#line 89
        U_0[b_3] = _S47 * sc_0;

#line 89
        U_0[_S45] = _S48 * sc_0;

#line 89
        U_0[_S46] = _S49 * sc_0;

#line 87
        b_3 = b_3 + int(1);

#line 87
    }

#line 87
    b_3 = int(0);



    for(;;)
    {

#line 91
        if(b_3 < int(3))
        {
        }
        else
        {

#line 91
            break;
        }

#line 91
        if((sig_0[b_3]) <= 9.999999960041972e-13f)
        {

#line 92
            int32_t a_4 = (b_3 + int(1)) % int(3);

#line 92
            int32_t b_4 = (b_3 + int(2)) % int(3);
            Vector<float, 3>  uc_0 = cr_0(Vector<float, 3> (U_0[a_4], U_0[int(3) + a_4], U_0[int(6) + a_4]), Vector<float, 3> (U_0[b_4], U_0[int(3) + b_4], U_0[int(6) + b_4]));

#line 93
            U_0[b_3] = uc_0.x;

#line 93
            U_0[int(3) + b_3] = uc_0.y;

#line 93
            U_0[int(6) + b_3] = uc_0.z;

#line 91
        }

#line 91
        b_3 = b_3 + int(1);

#line 91
    }



    FixedArray<float, 9>  Vt_0;

#line 95
    i_6 = int(0);

#line 95
    for(;;)
    {

#line 95
        if(i_6 < int(3))
        {
        }
        else
        {

#line 95
            break;
        }

#line 95
        j_1 = int(0);

#line 95
        for(;;)
        {

#line 95
            if(j_1 < int(3))
            {
            }
            else
            {

#line 95
                break;
            }

#line 95
            Vt_0[i_6 * int(3) + j_1] = V_1[j_1 * int(3) + i_6];

#line 95
            j_1 = j_1 + int(1);

#line 95
        }

#line 95
        i_6 = i_6 + int(1);

#line 95
    }

#line 95
    FixedArray<float, 9>  _S50 = U_0;

#line 95
    FixedArray<float, 9>  _S51 = Vt_0;
    FixedArray<float, 9>  UVt_0;

#line 96
    mul9_0(&_S50, &_S51, &UVt_0);

#line 96
    FixedArray<float, 9>  _S52 = UVt_0;

#line 96
    float _S53 = det9_0(&_S52);

#line 96
    if(_S53 < 0.0f)
    {

#line 96
        sc_0 = -1.0f;

#line 96
    }
    else
    {

#line 96
        sc_0 = 1.0f;

#line 96
    }
    FixedArray<float, 9>  Ud_0;

#line 97
    i_6 = int(0);

#line 97
    for(;;)
    {

#line 97
        if(i_6 < int(9))
        {
        }
        else
        {

#line 97
            break;
        }

#line 97
        Ud_0[i_6] = U_0[i_6];

#line 97
        i_6 = i_6 + int(1);

#line 97
    }

#line 97
    Ud_0[int(2)] = Ud_0[int(2)] * sc_0;

#line 97
    Ud_0[int(5)] = Ud_0[int(5)] * sc_0;

#line 97
    Ud_0[int(8)] = Ud_0[int(8)] * sc_0;

#line 97
    FixedArray<float, 9>  _S54 = Ud_0;

#line 97
    FixedArray<float, 9>  _S55 = Vt_0;

#line 97
    mul9_0(&_S54, &_S55, R_3);

    return;
}


#line 100
static void finishAlign_0(FixedArray<float, 9>  * Hin_0, int32_t N_0, Vector<float, 3>  a0_0, Vector<float, 3>  a1_0, Vector<float, 3>  b0_0, Vector<float, 3>  b1_0, FixedArray<float, 9>  * R_4)
{

#line 101
    if(N_0 == int(1))
    {

#line 101
        rodrigues_0(a0_0, b0_0, R_4);

#line 101
        return;
    }

#line 102
    FixedArray<float, 9>  H_3;

#line 102
    int32_t i_7 = int(0);

#line 102
    for(;;)
    {

#line 102
        if(i_7 < int(9))
        {
        }
        else
        {

#line 102
            break;
        }

#line 102
        H_3[i_7] = (*Hin_0)[i_7];

#line 102
        i_7 = i_7 + int(1);

#line 102
    }
    Vector<float, 3>  ns_0 = cr_0(a0_0, a1_0);

#line 103
    Vector<float, 3>  nd_0 = cr_0(b0_0, b1_0);

#line 103
    float ln_0 = length_0(ns_0);

#line 103
    float ld_0 = length_0(nd_0);

#line 103
    bool _S56;
    if(ln_0 > 9.99999971718068537e-10f)
    {

#line 104
        _S56 = ld_0 > 9.99999971718068537e-10f;

#line 104
    }
    else
    {

#line 104
        _S56 = false;

#line 104
    }

#line 104
    if(_S56)
    {

#line 105
        Vector<float, 3>  vs_0 = ns_0 * (Vector<float, 3> )(length_0(a0_0) / (ln_0 + 9.99999993922529029e-09f));

#line 105
        FixedArray<float, 3>  vsa_0;

#line 105
        vsa_0[int(0)] = vs_0.x;

#line 105
        vsa_0[int(1)] = vs_0.y;

#line 105
        vsa_0[int(2)] = vs_0.z;
        Vector<float, 3>  vd_0 = nd_0 * (Vector<float, 3> )(length_0(b0_0) / (ld_0 + 9.99999993922529029e-09f));

#line 106
        FixedArray<float, 3>  vda_0;

#line 106
        vda_0[int(0)] = vd_0.x;

#line 106
        vda_0[int(1)] = vd_0.y;

#line 106
        vda_0[int(2)] = vd_0.z;

#line 106
        i_7 = int(0);
        for(;;)
        {

#line 107
            if(i_7 < int(3))
            {
            }
            else
            {

#line 107
                break;
            }

#line 107
            int32_t j_2 = int(0);

#line 107
            for(;;)
            {

#line 107
                if(j_2 < int(3))
                {
                }
                else
                {

#line 107
                    break;
                }

#line 107
                H_3[i_7 * int(3) + j_2] = H_3[i_7 * int(3) + j_2] + vsa_0[i_7] * vda_0[j_2];

#line 107
                j_2 = j_2 + int(1);

#line 107
            }

#line 107
            i_7 = i_7 + int(1);

#line 107
        }

#line 104
    }

#line 109
    regularize_0(&H_3);

#line 109
    FixedArray<float, 9>  _S57 = H_3;

#line 109
    ns30_0(&_S57, R_4);

#line 109
    FixedArray<float, 9>  _S58 = *R_4;

#line 109
    bool _S59 = valid9_0(&_S58);

#line 109
    if(!_S59)
    {

#line 109
        FixedArray<float, 9>  _S60 = H_3;

#line 109
        kabsch_0(&_S60, R_4);

#line 109
    }
    return;
}


void _skeleton_fit(void* _S61, void* entryPointParams_0, void* globalParams_1)
{

#line 114
    int32_t k_2;

#line 114
    int32_t a_5;

#line 114
    int32_t b_5;

#line 114
    Vector<float, 3>  sa0_0;

#line 114
    Vector<float, 3>  sa1_0;

#line 114
    Vector<float, 3>  sb0_0;

#line 114
    Vector<float, 3>  sb1_0;

#line 114
    ComputeThreadVaryingInput * _S62 = (slang_bit_cast<ComputeThreadVaryingInput *>(_S61));

#line 114
    KernelContext_0 kernelContext_0;

#line 114
    (&kernelContext_0)->globalParams_0 = (slang_bit_cast<GlobalParams_0*>(globalParams_1));

    if(((_S62->groupID + _S62->groupThreadID).x) != 0U)
    {

#line 116
        return;
    }

#line 117
    uint _elementCount_0;
    uint _stride_0;
    (&kernelContext_0)->globalParams_0->parents_0.GetDimensions(&_elementCount_0, &_stride_0);
    Vector<uint32_t, 2>  _S63 = uint2(_elementCount_0, _stride_0);

#line 117
    uint32_t _S64 = _S63.x;
    uint _elementCount_1;
    uint _stride_1;
    (&kernelContext_0)->globalParams_0->v0_0.GetDimensions(&_elementCount_1, &_stride_1);
    Vector<uint32_t, 2>  _S65 = uint2(_elementCount_1, _stride_1);

#line 118
    uint32_t _S66 = _S65.x / 3U;

#line 118
    uint32_t j_3 = 0U;
    for(;;)
    {

#line 119
        if(j_3 < _S64)
        {
        }
        else
        {

#line 119
            break;
        }

#line 119
        k_2 = int(0);

#line 119
        for(;;)
        {

#line 119
            if(k_2 < int(9))
            {
            }
            else
            {

#line 119
                break;
            }

#line 119
            uint32_t _S67 = j_3 * 16U + uint32_t(k_2 / int(3) * int(4)) + uint32_t(k_2 % int(3));

#line 119
            *(&((&kernelContext_0)->globalParams_0->outbw_0)[_S67]) = (&kernelContext_0)->globalParams_0->bw_0.Load(_S67);

#line 119
            k_2 = k_2 + int(1);

#line 119
        }

#line 119
        j_3 = j_3 + 1U;

#line 119
    }

#line 119
    uint32_t i_8 = 1U;
    for(;;)
    {

#line 120
        if(i_8 < _S64)
        {
        }
        else
        {

#line 120
            break;
        }

#line 121
        int32_t _S68 = (&kernelContext_0)->globalParams_0->parents_0.Load(i_8);
        FixedArray<int32_t, 32>  ch_0;

#line 122
        j_3 = 0U;

#line 122
        int32_t nc_0 = int(0);
        for(;;)
        {

#line 123
            if(j_3 < _S64)
            {
            }
            else
            {

#line 123
                break;
            }

#line 123
            int32_t _S69 = int32_t(i_8);

#line 123
            bool _S70;

#line 123
            if(((&kernelContext_0)->globalParams_0->parents_0).Load(j_3) == _S69)
            {

#line 123
                _S70 = int32_t(j_3) != _S69;

#line 123
            }
            else
            {

#line 123
                _S70 = false;

#line 123
            }

#line 123
            if(_S70)
            {

#line 123
                if(nc_0 < int(32))
                {

#line 123
                    ch_0[nc_0] = int32_t(j_3);

#line 123
                    k_2 = nc_0 + int(1);

#line 123
                }
                else
                {

#line 123
                    k_2 = nc_0;

#line 123
                }

#line 123
                nc_0 = k_2;

#line 123
            }

#line 123
            j_3 = j_3 + 1U;

#line 123
        }
        if(nc_0 == int(0))
        {

#line 124
            k_2 = int(0);

#line 124
            for(;;)
            {

#line 124
                if(k_2 < int(9))
                {
                }
                else
                {

#line 124
                    break;
                }

#line 124
                int32_t c_2 = k_2 % int(3);

#line 124
                int32_t _S71 = k_2 / int(3) * int(4);

#line 124
                *(&((&kernelContext_0)->globalParams_0->outbw_0)[i_8 * 16U + uint32_t(_S71) + uint32_t(c_2)]) = *(&((&kernelContext_0)->globalParams_0->outbw_0)[_S68 * int(16) + _S71 + c_2]);

#line 124
                k_2 = k_2 + int(1);

#line 124
            }

#line 124
            i_8 = i_8 + 1U;

#line 120
            continue;
        }



        uint32_t _S72 = i_8 * 16U;

#line 125
        float _S73 = (&kernelContext_0)->globalParams_0->bw_0.Load(_S72 + 3U);

#line 125
        float _S74 = (&kernelContext_0)->globalParams_0->bw_0.Load(_S72 + 7U);

#line 125
        float _S75 = (&kernelContext_0)->globalParams_0->bw_0.Load(_S72 + 11U);
        uint32_t _S76 = i_8 * 3U;

#line 126
        float _S77 = (&kernelContext_0)->globalParams_0->jpos_0.Load(_S76);

#line 126
        float _S78 = (&kernelContext_0)->globalParams_0->jpos_0.Load(_S76 + 1U);

#line 126
        float _S79 = (&kernelContext_0)->globalParams_0->jpos_0.Load(_S76 + 2U);
        FixedArray<float, 9>  Hs_0;

#line 127
        k_2 = int(0);

#line 127
        for(;;)
        {

#line 127
            if(k_2 < int(9))
            {
            }
            else
            {

#line 127
                break;
            }

#line 127
            Hs_0[k_2] = 0.0f;

#line 127
            k_2 = k_2 + int(1);

#line 127
        }
        Vector<float, 3>  _S80 = Vector<float, 3> (0.0f, 0.0f, 0.0f);

#line 128
        Vector<float, 3>  sa0_1 = _S80;

#line 128
        Vector<float, 3>  sa1_1 = _S80;

#line 128
        Vector<float, 3>  sb0_1 = _S80;

#line 128
        Vector<float, 3>  sb1_1 = _S80;

#line 128
        uint32_t v_1 = 0U;

#line 128
        int32_t cnt_0 = int(0);
        for(;;)
        {

#line 129
            if(v_1 < _S66)
            {
            }
            else
            {

#line 129
                break;
            }

#line 130
            if(((&kernelContext_0)->globalParams_0->sw_0).Load(v_1 * _S64 + i_8) <= 0.00999999977648258f)
            {

#line 130
                v_1 = v_1 + 1U;

#line 129
                continue;
            }
            uint32_t _S81 = v_1 * 3U;

#line 131
            float _S82 = (&kernelContext_0)->globalParams_0->v0_0.Load(_S81) - _S77;

#line 131
            uint32_t _S83 = _S81 + 1U;

#line 131
            float _S84 = (&kernelContext_0)->globalParams_0->v0_0.Load(_S83) - _S78;

#line 131
            uint32_t _S85 = _S81 + 2U;

#line 131
            float _S86 = (&kernelContext_0)->globalParams_0->v0_0.Load(_S85) - _S79;

#line 131
            Vector<float, 3>  tg_0 = Vector<float, 3> (_S82, _S84, _S86);
            float _S87 = (&kernelContext_0)->globalParams_0->bsh_0.Load(_S81) - _S73;

#line 132
            float _S88 = (&kernelContext_0)->globalParams_0->bsh_0.Load(_S83) - _S74;

#line 132
            float _S89 = (&kernelContext_0)->globalParams_0->bsh_0.Load(_S85) - _S75;

#line 132
            Vector<float, 3>  og_0 = Vector<float, 3> (_S87, _S88, _S89);
            FixedArray<float, 3>  _S90 = { _S82, _S84, _S86 };

#line 133
            FixedArray<float, 3>  _S91 = { _S87, _S88, _S89 };

#line 133
            a_5 = int(0);
            for(;;)
            {

#line 134
                if(a_5 < int(3))
                {
                }
                else
                {

#line 134
                    break;
                }

#line 134
                b_5 = int(0);

#line 134
                for(;;)
                {

#line 134
                    if(b_5 < int(3))
                    {
                    }
                    else
                    {

#line 134
                        break;
                    }

#line 134
                    Hs_0[a_5 * int(3) + b_5] = Hs_0[a_5 * int(3) + b_5] + _S90[a_5] * _S91[b_5];

#line 134
                    b_5 = b_5 + int(1);

#line 134
                }

#line 134
                a_5 = a_5 + int(1);

#line 134
            }
            if(cnt_0 == int(0))
            {

#line 135
                sa0_0 = tg_0;

#line 135
                sa1_0 = sa1_1;

#line 135
                sb0_0 = og_0;

#line 135
                sb1_0 = sb1_1;

#line 135
            }
            else
            {

#line 135
                if(cnt_0 == int(1))
                {

#line 135
                    sa0_0 = tg_0;

#line 135
                    sa1_0 = og_0;

#line 135
                }
                else
                {

#line 135
                    sa0_0 = sa1_1;

#line 135
                    sa1_0 = sb1_1;

#line 135
                }

#line 128
                Vector<float, 3>  _S92 = sa0_0;

#line 128
                Vector<float, 3>  _S93 = sa1_0;

#line 128
                sa0_0 = sa0_1;

#line 128
                sa1_0 = _S92;

#line 128
                sb0_0 = sb0_1;

#line 128
                sb1_0 = _S93;

#line 135
            }

#line 135
            int32_t _S94 = cnt_0 + int(1);

#line 135
            sa0_1 = sa0_0;

#line 135
            sa1_1 = sa1_0;

#line 135
            sb0_1 = sb0_0;

#line 135
            sb1_1 = sb1_0;

#line 135
            cnt_0 = _S94;

#line 129
            v_1 = v_1 + 1U;

#line 129
        }

#line 129
        FixedArray<float, 9>  _S95 = Hs_0;

#line 137
        FixedArray<float, 9>  Rinit_0;

#line 137
        finishAlign_0(&_S95, cnt_0, sa0_1, sa1_1, sb0_1, sb1_1, &Rinit_0);
        FixedArray<float, 9>  Hc_0;

#line 138
        int32_t k_3 = int(0);

#line 138
        for(;;)
        {

#line 138
            if(k_3 < int(9))
            {
            }
            else
            {

#line 138
                break;
            }

#line 138
            Hc_0[k_3] = 0.0f;

#line 138
            k_3 = k_3 + int(1);

#line 138
        }

#line 138
        sa0_0 = _S80;

#line 138
        sa1_0 = _S80;

#line 138
        sb0_0 = _S80;

#line 138
        sb1_0 = _S80;

#line 138
        int32_t c_3 = int(0);

        for(;;)
        {

#line 140
            if(c_3 < nc_0)
            {
            }
            else
            {

#line 140
                break;
            }
            int32_t _S96 = ch_0[c_3] * int(16);

#line 142
            float _S97 = (&kernelContext_0)->globalParams_0->bw_0.Load(_S96 + int(3)) - _S73;

#line 142
            float _S98 = (&kernelContext_0)->globalParams_0->bw_0.Load(_S96 + int(7)) - _S74;

#line 142
            float _S99 = (&kernelContext_0)->globalParams_0->bw_0.Load(_S96 + int(11)) - _S75;
            float _S100 = Rinit_0[int(0)] * _S97 + Rinit_0[int(1)] * _S98 + Rinit_0[int(2)] * _S99;

#line 143
            float _S101 = Rinit_0[int(3)] * _S97 + Rinit_0[int(4)] * _S98 + Rinit_0[int(5)] * _S99;

#line 143
            float _S102 = Rinit_0[int(6)] * _S97 + Rinit_0[int(7)] * _S98 + Rinit_0[int(8)] * _S99;

#line 143
            Vector<float, 3>  pco_0 = Vector<float, 3> (_S100, _S101, _S102);
            int32_t _S103 = ch_0[c_3] * int(3);

#line 144
            float _S104 = (&kernelContext_0)->globalParams_0->jpos_0.Load(_S103) - _S77;

#line 144
            float _S105 = (&kernelContext_0)->globalParams_0->jpos_0.Load(_S103 + int(1)) - _S78;

#line 144
            float _S106 = (&kernelContext_0)->globalParams_0->jpos_0.Load(_S103 + int(2)) - _S79;

#line 144
            Vector<float, 3>  pcn_0 = Vector<float, 3> (_S104, _S105, _S106);
            FixedArray<float, 3>  _S107 = { _S104, _S105, _S106 };

#line 145
            FixedArray<float, 3>  _S108 = { _S100, _S101, _S102 };

#line 145
            a_5 = int(0);
            for(;;)
            {

#line 146
                if(a_5 < int(3))
                {
                }
                else
                {

#line 146
                    break;
                }

#line 146
                b_5 = int(0);

#line 146
                for(;;)
                {

#line 146
                    if(b_5 < int(3))
                    {
                    }
                    else
                    {

#line 146
                        break;
                    }

#line 146
                    Hc_0[a_5 * int(3) + b_5] = Hc_0[a_5 * int(3) + b_5] + _S107[a_5] * _S108[b_5];

#line 146
                    b_5 = b_5 + int(1);

#line 146
                }

#line 146
                a_5 = a_5 + int(1);

#line 146
            }
            if(c_3 == int(0))
            {

#line 147
                sa0_0 = pcn_0;

#line 147
                sb0_0 = pco_0;

#line 147
            }
            else
            {

#line 147
                Vector<float, 3>  ca1_0;

#line 147
                Vector<float, 3>  cb1_0;

#line 147
                if(c_3 == int(1))
                {

#line 147
                    ca1_0 = pcn_0;

#line 147
                    cb1_0 = pco_0;

#line 147
                }
                else
                {

#line 147
                    ca1_0 = sa1_0;

#line 147
                    cb1_0 = sb1_0;

#line 147
                }

#line 147
                sa1_0 = ca1_0;

#line 147
                sb1_0 = cb1_0;

#line 147
            }

#line 140
            c_3 = c_3 + int(1);

#line 140
        }

#line 140
        FixedArray<float, 9>  _S109 = Hc_0;

#line 149
        FixedArray<float, 9>  Arot_0;

#line 149
        finishAlign_0(&_S109, nc_0, sa0_0, sa1_0, sb0_0, sb1_0, &Arot_0);
        FixedArray<float, 9>  R0_0;

#line 150
        a_5 = int(0);

#line 150
        for(;;)
        {

#line 150
            if(a_5 < int(9))
            {
            }
            else
            {

#line 150
                break;
            }

#line 150
            R0_0[a_5] = *(&((&kernelContext_0)->globalParams_0->outbw_0)[_S72 + uint32_t(a_5 / int(3) * int(4)) + uint32_t(a_5 % int(3))]);

#line 150
            a_5 = a_5 + int(1);

#line 150
        }

#line 150
        FixedArray<float, 9>  _S110 = Arot_0;

#line 150
        FixedArray<float, 9>  _S111 = Rinit_0;
        FixedArray<float, 9>  tmp_0;

#line 151
        mul9_0(&_S110, &_S111, &tmp_0);

#line 151
        FixedArray<float, 9>  _S112 = tmp_0;

#line 151
        FixedArray<float, 9>  _S113 = R0_0;

#line 151
        FixedArray<float, 9>  Ri_0;

#line 151
        mul9_0(&_S112, &_S113, &Ri_0);

#line 151
        b_5 = int(0);
        for(;;)
        {

#line 152
            if(b_5 < int(9))
            {
            }
            else
            {

#line 152
                break;
            }

#line 152
            *(&((&kernelContext_0)->globalParams_0->outbw_0)[_S72 + uint32_t(b_5 / int(3) * int(4)) + uint32_t(b_5 % int(3))]) = Ri_0[b_5];

#line 152
            b_5 = b_5 + int(1);

#line 152
        }

#line 120
        i_8 = i_8 + 1U;

#line 120
    }

#line 120
    j_3 = 0U;

#line 154
    for(;;)
    {

#line 154
        if(j_3 < _S64)
        {
        }
        else
        {

#line 154
            break;
        }

#line 155
        uint32_t _S114 = j_3 * 16U;

#line 155
        uint32_t _S115 = j_3 * 3U;

#line 155
        *(&((&kernelContext_0)->globalParams_0->outbw_0)[_S114 + 3U]) = (&kernelContext_0)->globalParams_0->jpos_0.Load(_S115);

#line 155
        *(&((&kernelContext_0)->globalParams_0->outbw_0)[_S114 + 7U]) = (&kernelContext_0)->globalParams_0->jpos_0.Load(_S115 + 1U);

#line 155
        *(&((&kernelContext_0)->globalParams_0->outbw_0)[_S114 + 11U]) = (&kernelContext_0)->globalParams_0->jpos_0.Load(_S115 + 2U);
        *(&((&kernelContext_0)->globalParams_0->outbw_0)[_S114 + 12U]) = 0.0f;

#line 156
        *(&((&kernelContext_0)->globalParams_0->outbw_0)[_S114 + 13U]) = 0.0f;

#line 156
        *(&((&kernelContext_0)->globalParams_0->outbw_0)[_S114 + 14U]) = 0.0f;

#line 156
        *(&((&kernelContext_0)->globalParams_0->outbw_0)[_S114 + 15U]) = 1.0f;

#line 154
        j_3 = j_3 + 1U;

#line 154
    }



    return;
}

// [numthreads(1, 1, 1)]
SLANG_PRELUDE_EXPORT
void skeleton_fit_Thread(ComputeThreadVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    _skeleton_fit(varyingInput, entryPointParams, globalParams);
}
// [numthreads(1, 1, 1)]
SLANG_PRELUDE_EXPORT
void skeleton_fit_Group(ComputeVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    ComputeThreadVaryingInput threadInput = {};
    threadInput.groupID = varyingInput->startGroupID;
    _skeleton_fit(&threadInput, entryPointParams, globalParams);
}
// [numthreads(1, 1, 1)]
SLANG_PRELUDE_EXPORT
void skeleton_fit(ComputeVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
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
                skeleton_fit_Group(&groupVaryingInput, entryPointParams, globalParams);
            }
        }
    }
}
