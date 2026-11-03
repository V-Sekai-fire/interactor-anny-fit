#include "slang-cpp-prelude.h"

#ifdef SLANG_PRELUDE_NAMESPACE
using namespace SLANG_PRELUDE_NAMESPACE;
#endif


#line 1 "slang/star_kabsch.slang"
struct StarKabschParams_0
{
    uint32_t N_0;
};


#line 457
struct GlobalParams_0
{
    StarKabschParams_0* params_0;
    StructuredBuffer<float> tgt_0;
    StructuredBuffer<float> src_0;
    RWStructuredBuffer<float> rot_0;
};


#line 457
struct KernelContext_0
{
    GlobalParams_0* globalParams_0;
};


#line 15
void _main_0(void* _S1, void* entryPointParams_0, void* globalParams_1)
{

#line 15
    float w1_0;

#line 15
    float w4_0;

#line 15
    float a0_0;

#line 15
    float a3_0;

#line 15
    float a6_0;

#line 15
    float w0_0;

#line 15
    float w3_0;

#line 15
    float w6_0;

#line 15
    float w7_0;

#line 15
    float a2_0;

#line 15
    float a5_0;

#line 15
    float a1_0;

#line 15
    float a4_0;

#line 15
    float a7_0;

#line 15
    float a8_0;

#line 15
    float w2_0;

#line 15
    float w5_0;

#line 15
    float w8_0;

#line 15
    float u1_1_0;

#line 15
    ComputeThreadVaryingInput * _S2 = (slang_bit_cast<ComputeThreadVaryingInput *>(_S1));

#line 15
    KernelContext_0 kernelContext_0;

#line 15
    (&kernelContext_0)->globalParams_0 = (slang_bit_cast<GlobalParams_0*>(globalParams_1));
    uint32_t i_0 = (_S2->groupID * Vector<uint32_t, 3> (64U, 1U, 1U) + _S2->groupThreadID).x;
    if(i_0 >= ((slang_bit_cast<GlobalParams_0*>(globalParams_1))->params_0->N_0))
    {

#line 18
        return;
    }
    uint32_t _S3 = i_0 * 21U;

#line 20
    float t0_0_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S3);
    uint32_t _S4 = _S3 + 1U;

#line 21
    float t0_1_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S4);
    uint32_t _S5 = _S3 + 2U;

#line 22
    float t0_2_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S5);
    uint32_t _S6 = _S3 + 3U;

#line 23
    float t1_0_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S6);
    uint32_t _S7 = _S3 + 4U;

#line 24
    float t1_1_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S7);
    uint32_t _S8 = _S3 + 5U;

#line 25
    float t1_2_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S8);
    uint32_t _S9 = _S3 + 6U;

#line 26
    float t2_0_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S9);
    uint32_t _S10 = _S3 + 7U;

#line 27
    float t2_1_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S10);
    uint32_t _S11 = _S3 + 8U;

#line 28
    float t2_2_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S11);
    uint32_t _S12 = _S3 + 9U;

#line 29
    float t3_0_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S12);
    uint32_t _S13 = _S3 + 10U;

#line 30
    float t3_1_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S13);
    uint32_t _S14 = _S3 + 11U;

#line 31
    float t3_2_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S14);
    uint32_t _S15 = _S3 + 12U;

#line 32
    float t4_0_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S15);
    uint32_t _S16 = _S3 + 13U;

#line 33
    float t4_1_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S16);
    uint32_t _S17 = _S3 + 14U;

#line 34
    float t4_2_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S17);
    uint32_t _S18 = _S3 + 15U;

#line 35
    float t5_0_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S18);
    uint32_t _S19 = _S3 + 16U;

#line 36
    float t5_1_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S19);
    uint32_t _S20 = _S3 + 17U;

#line 37
    float t5_2_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S20);
    uint32_t _S21 = _S3 + 18U;

#line 38
    float t6_0_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S21);
    uint32_t _S22 = _S3 + 19U;

#line 39
    float t6_1_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S22);
    uint32_t _S23 = _S3 + 20U;

#line 40
    float t6_2_0 = (&kernelContext_0)->globalParams_0->tgt_0.Load(_S23);
    float q0_0_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S3);
    float q0_1_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S4);
    float q0_2_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S5);
    float q1_0_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S6);
    float q1_1_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S7);
    float q1_2_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S8);
    float q2_0_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S9);
    float q2_1_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S10);
    float q2_2_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S11);
    float q3_0_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S12);
    float q3_1_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S13);
    float q3_2_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S14);
    float q4_0_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S15);
    float q4_1_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S16);
    float q4_2_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S17);
    float q5_0_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S18);
    float q5_1_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S19);
    float q5_2_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S20);
    float q6_0_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S21);
    float q6_1_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S22);
    float q6_2_0 = (&kernelContext_0)->globalParams_0->src_0.Load(_S23);
    float ct0_0 = (t0_0_0 + t1_0_0 + t2_0_0 + t3_0_0 + t4_0_0 + t5_0_0 + t6_0_0) / 7.0f;
    float ct1_0 = (t0_1_0 + t1_1_0 + t2_1_0 + t3_1_0 + t4_1_0 + t5_1_0 + t6_1_0) / 7.0f;
    float ct2_0 = (t0_2_0 + t1_2_0 + t2_2_0 + t3_2_0 + t4_2_0 + t5_2_0 + t6_2_0) / 7.0f;
    float cq0_0 = (q0_0_0 + q1_0_0 + q2_0_0 + q3_0_0 + q4_0_0 + q5_0_0 + q6_0_0) / 7.0f;
    float cq1_0 = (q0_1_0 + q1_1_0 + q2_1_0 + q3_1_0 + q4_1_0 + q5_1_0 + q6_1_0) / 7.0f;
    float cq2_0 = (q0_2_0 + q1_2_0 + q2_2_0 + q3_2_0 + q4_2_0 + q5_2_0 + q6_2_0) / 7.0f;
    float dt0_0_0 = t0_0_0 - ct0_0;
    float dt0_1_0 = t0_1_0 - ct1_0;
    float dt0_2_0 = t0_2_0 - ct2_0;
    float dq0_0_0 = q0_0_0 - cq0_0;
    float dq0_1_0 = q0_1_0 - cq1_0;
    float dq0_2_0 = q0_2_0 - cq2_0;
    float dt1_0_0 = t1_0_0 - ct0_0;
    float dt1_1_0 = t1_1_0 - ct1_0;
    float dt1_2_0 = t1_2_0 - ct2_0;
    float dq1_0_0 = q1_0_0 - cq0_0;
    float dq1_1_0 = q1_1_0 - cq1_0;
    float dq1_2_0 = q1_2_0 - cq2_0;
    float dt2_0_0 = t2_0_0 - ct0_0;
    float dt2_1_0 = t2_1_0 - ct1_0;
    float dt2_2_0 = t2_2_0 - ct2_0;
    float dq2_0_0 = q2_0_0 - cq0_0;
    float dq2_1_0 = q2_1_0 - cq1_0;
    float dq2_2_0 = q2_2_0 - cq2_0;
    float dt3_0_0 = t3_0_0 - ct0_0;
    float dt3_1_0 = t3_1_0 - ct1_0;
    float dt3_2_0 = t3_2_0 - ct2_0;
    float dq3_0_0 = q3_0_0 - cq0_0;
    float dq3_1_0 = q3_1_0 - cq1_0;
    float dq3_2_0 = q3_2_0 - cq2_0;
    float dt4_0_0 = t4_0_0 - ct0_0;
    float dt4_1_0 = t4_1_0 - ct1_0;
    float dt4_2_0 = t4_2_0 - ct2_0;
    float dq4_0_0 = q4_0_0 - cq0_0;
    float dq4_1_0 = q4_1_0 - cq1_0;
    float dq4_2_0 = q4_2_0 - cq2_0;
    float dt5_0_0 = t5_0_0 - ct0_0;
    float dt5_1_0 = t5_1_0 - ct1_0;
    float dt5_2_0 = t5_2_0 - ct2_0;
    float dq5_0_0 = q5_0_0 - cq0_0;
    float dq5_1_0 = q5_1_0 - cq1_0;
    float dq5_2_0 = q5_2_0 - cq2_0;
    float dt6_0_0 = t6_0_0 - ct0_0;
    float dt6_1_0 = t6_1_0 - ct1_0;
    float dt6_2_0 = t6_2_0 - ct2_0;
    float dq6_0_0 = q6_0_0 - cq0_0;
    float dq6_1_0 = q6_1_0 - cq1_0;
    float dq6_2_0 = q6_2_0 - cq2_0;

    float H1_0 = dt0_0_0 * dq0_1_0 + dt1_0_0 * dq1_1_0 + dt2_0_0 * dq2_1_0 + dt3_0_0 * dq3_1_0 + dt4_0_0 * dq4_1_0 + dt5_0_0 * dq5_1_0 + dt6_0_0 * dq6_1_0;
    float H2_0 = dt0_0_0 * dq0_2_0 + dt1_0_0 * dq1_2_0 + dt2_0_0 * dq2_2_0 + dt3_0_0 * dq3_2_0 + dt4_0_0 * dq4_2_0 + dt5_0_0 * dq5_2_0 + dt6_0_0 * dq6_2_0;
    float H3_0 = dt0_1_0 * dq0_0_0 + dt1_1_0 * dq1_0_0 + dt2_1_0 * dq2_0_0 + dt3_1_0 * dq3_0_0 + dt4_1_0 * dq4_0_0 + dt5_1_0 * dq5_0_0 + dt6_1_0 * dq6_0_0;
    float H4_0 = dt0_1_0 * dq0_1_0 + dt1_1_0 * dq1_1_0 + dt2_1_0 * dq2_1_0 + dt3_1_0 * dq3_1_0 + dt4_1_0 * dq4_1_0 + dt5_1_0 * dq5_1_0 + dt6_1_0 * dq6_1_0;
    float H5_0 = dt0_1_0 * dq0_2_0 + dt1_1_0 * dq1_2_0 + dt2_1_0 * dq2_2_0 + dt3_1_0 * dq3_2_0 + dt4_1_0 * dq4_2_0 + dt5_1_0 * dq5_2_0 + dt6_1_0 * dq6_2_0;
    float H6_0 = dt0_2_0 * dq0_0_0 + dt1_2_0 * dq1_0_0 + dt2_2_0 * dq2_0_0 + dt3_2_0 * dq3_0_0 + dt4_2_0 * dq4_0_0 + dt5_2_0 * dq5_0_0 + dt6_2_0 * dq6_0_0;
    float H7_0 = dt0_2_0 * dq0_1_0 + dt1_2_0 * dq1_1_0 + dt2_2_0 * dq2_1_0 + dt3_2_0 * dq3_1_0 + dt4_2_0 * dq4_1_0 + dt5_2_0 * dq5_1_0 + dt6_2_0 * dq6_1_0;
    float H8_0 = dt0_2_0 * dq0_2_0 + dt1_2_0 * dq1_2_0 + dt2_2_0 * dq2_2_0 + dt3_2_0 * dq3_2_0 + dt4_2_0 * dq4_2_0 + dt5_2_0 * dq5_2_0 + dt6_2_0 * dq6_2_0;

#line 118
    float a0_1 = dt0_0_0 * dq0_0_0 + dt1_0_0 * dq1_0_0 + dt2_0_0 * dq2_0_0 + dt3_0_0 * dq3_0_0 + dt4_0_0 * dq4_0_0 + dt5_0_0 * dq5_0_0 + dt6_0_0 * dq6_0_0;

#line 118
    float a3_1 = H3_0;

#line 118
    float a6_1 = H6_0;

#line 118
    float a1_1 = H1_0;

#line 118
    float a4_1 = H4_0;

#line 118
    float a7_1 = H7_0;

#line 118
    float w0_1 = 1.0f;

#line 118
    float w1_1 = 0.0f;

#line 118
    float w3_1 = 0.0f;

#line 118
    float w4_1 = 1.0f;

#line 118
    float w6_1 = 0.0f;

#line 118
    float w7_1 = 0.0f;

#line 118
    float a2_1 = H2_0;

#line 118
    float a5_1 = H5_0;

#line 118
    float a8_1 = H8_0;

#line 118
    float w2_1 = 0.0f;

#line 118
    float w5_1 = 0.0f;

#line 118
    float w8_1 = 1.0f;

#line 118
    uint32_t sweep_0 = 0U;

#line 137
    for(;;)
    {

#line 137
        if(sweep_0 < 8U)
        {
        }
        else
        {

#line 137
            break;
        }

#line 138
        float al01_0 = a0_1 * a0_1 + a3_1 * a3_1 + a6_1 * a6_1;
        float be01_0 = a1_1 * a1_1 + a4_1 * a4_1 + a7_1 * a7_1;
        float ga01_0 = a0_1 * a1_1 + a3_1 * a4_1 + a6_1 * a7_1;
        if((F32_abs((ga01_0))) > (1.1920928955078125e-07f * (F32_sqrt((al01_0 * be01_0)))))
        {

#line 142
            float ze01_0 = (be01_0 - al01_0) / (2.0f * ga01_0);
            if(ze01_0 >= 0.0f)
            {

#line 143
                w1_0 = 1.0f;

#line 143
            }
            else
            {

#line 143
                w1_0 = -1.0f;

#line 143
            }

#line 143
            float t01_0 = w1_0 / ((F32_abs((ze01_0))) + (F32_sqrt((1.0f + ze01_0 * ze01_0))));
            float c01_0 = 1.0f / (F32_sqrt((1.0f + t01_0 * t01_0)));
            float s01_0 = c01_0 * t01_0;



            float _S24 = s01_0 * a0_1 + c01_0 * a1_1;


            float _S25 = c01_0 * w0_1 - s01_0 * w1_1;
            float _S26 = s01_0 * w0_1 + c01_0 * w1_1;


            float _S27 = c01_0 * a3_1 - s01_0 * a4_1;
            float _S28 = s01_0 * a3_1 + c01_0 * a4_1;


            float _S29 = c01_0 * w3_1 - s01_0 * w4_1;
            float _S30 = s01_0 * w3_1 + c01_0 * w4_1;


            float _S31 = c01_0 * a6_1 - s01_0 * a7_1;
            float _S32 = s01_0 * a6_1 + c01_0 * a7_1;


            float _S33 = c01_0 * w6_1 - s01_0 * w7_1;
            float _S34 = s01_0 * w6_1 + c01_0 * w7_1;

#line 169
            a0_0 = c01_0 * a0_1 - s01_0 * a1_1;

#line 169
            a3_0 = _S27;

#line 169
            a6_0 = _S31;

#line 169
            w0_0 = _S25;

#line 169
            w3_0 = _S29;

#line 169
            w6_0 = _S33;

#line 169
            a1_0 = _S24;

#line 169
            a4_0 = _S28;

#line 169
            a7_0 = _S32;

#line 169
            w1_0 = _S26;

#line 169
            w4_0 = _S30;

#line 169
            w7_0 = _S34;

#line 141
        }
        else
        {

#line 141
            a0_0 = a0_1;

#line 141
            a3_0 = a3_1;

#line 141
            a6_0 = a6_1;

#line 141
            w0_0 = w0_1;

#line 141
            w3_0 = w3_1;

#line 141
            w6_0 = w6_1;

#line 141
            a1_0 = a1_1;

#line 141
            a4_0 = a4_1;

#line 141
            a7_0 = a7_1;

#line 141
            w1_0 = w1_1;

#line 141
            w4_0 = w4_1;

#line 141
            w7_0 = w7_1;

#line 141
        }

#line 171
        float al02_0 = a0_0 * a0_0 + a3_0 * a3_0 + a6_0 * a6_0;
        float be02_0 = a2_1 * a2_1 + a5_1 * a5_1 + a8_1 * a8_1;
        float ga02_0 = a0_0 * a2_1 + a3_0 * a5_1 + a6_0 * a8_1;
        if((F32_abs((ga02_0))) > (1.1920928955078125e-07f * (F32_sqrt((al02_0 * be02_0)))))
        {

#line 175
            float ze02_0 = (be02_0 - al02_0) / (2.0f * ga02_0);
            if(ze02_0 >= 0.0f)
            {

#line 176
                a2_0 = 1.0f;

#line 176
            }
            else
            {

#line 176
                a2_0 = -1.0f;

#line 176
            }

#line 176
            float t02_0 = a2_0 / ((F32_abs((ze02_0))) + (F32_sqrt((1.0f + ze02_0 * ze02_0))));
            float c02_0 = 1.0f / (F32_sqrt((1.0f + t02_0 * t02_0)));
            float s02_0 = c02_0 * t02_0;


            float _S35 = c02_0 * a0_0 - s02_0 * a2_1;



            float _S36 = c02_0 * w0_0 - s02_0 * w2_1;
            float _S37 = s02_0 * w0_0 + c02_0 * w2_1;


            float _S38 = c02_0 * a3_0 - s02_0 * a5_1;
            float _S39 = s02_0 * a3_0 + c02_0 * a5_1;


            float _S40 = c02_0 * w3_0 - s02_0 * w5_1;
            float _S41 = s02_0 * w3_0 + c02_0 * w5_1;


            float _S42 = c02_0 * a6_0 - s02_0 * a8_1;
            float _S43 = s02_0 * a6_0 + c02_0 * a8_1;


            float _S44 = c02_0 * w6_0 - s02_0 * w8_1;
            float _S45 = s02_0 * w6_0 + c02_0 * w8_1;

#line 202
            a2_0 = s02_0 * a0_0 + c02_0 * a2_1;

#line 202
            a5_0 = _S39;

#line 202
            a8_0 = _S43;

#line 202
            w2_0 = _S37;

#line 202
            w5_0 = _S41;

#line 202
            w8_0 = _S45;

#line 202
            a0_1 = _S35;

#line 202
            a3_1 = _S38;

#line 202
            a6_1 = _S42;

#line 202
            w0_1 = _S36;

#line 202
            w3_1 = _S40;

#line 202
            w6_1 = _S44;

#line 174
        }
        else
        {

#line 174
            a2_0 = a2_1;

#line 174
            a5_0 = a5_1;

#line 174
            a8_0 = a8_1;

#line 174
            w2_0 = w2_1;

#line 174
            w5_0 = w5_1;

#line 174
            w8_0 = w8_1;

#line 174
            a0_1 = a0_0;

#line 174
            a3_1 = a3_0;

#line 174
            a6_1 = a6_0;

#line 174
            w0_1 = w0_0;

#line 174
            w3_1 = w3_0;

#line 174
            w6_1 = w6_0;

#line 174
        }

#line 204
        float al12_0 = a1_0 * a1_0 + a4_0 * a4_0 + a7_0 * a7_0;
        float be12_0 = a2_0 * a2_0 + a5_0 * a5_0 + a8_0 * a8_0;
        float ga12_0 = a1_0 * a2_0 + a4_0 * a5_0 + a7_0 * a8_0;
        if((F32_abs((ga12_0))) > (1.1920928955078125e-07f * (F32_sqrt((al12_0 * be12_0)))))
        {

#line 208
            float ze12_0 = (be12_0 - al12_0) / (2.0f * ga12_0);
            if(ze12_0 >= 0.0f)
            {

#line 209
                u1_1_0 = 1.0f;

#line 209
            }
            else
            {

#line 209
                u1_1_0 = -1.0f;

#line 209
            }

#line 209
            float t12_0 = u1_1_0 / ((F32_abs((ze12_0))) + (F32_sqrt((1.0f + ze12_0 * ze12_0))));
            float c12_0 = 1.0f / (F32_sqrt((1.0f + t12_0 * t12_0)));
            float s12_0 = c12_0 * t12_0;



            float _S46 = s12_0 * a1_0 + c12_0 * a2_0;


            float _S47 = c12_0 * w1_0 - s12_0 * w2_0;
            float _S48 = s12_0 * w1_0 + c12_0 * w2_0;


            float _S49 = c12_0 * a4_0 - s12_0 * a5_0;
            float _S50 = s12_0 * a4_0 + c12_0 * a5_0;


            float _S51 = c12_0 * w4_0 - s12_0 * w5_0;
            float _S52 = s12_0 * w4_0 + c12_0 * w5_0;


            float _S53 = c12_0 * a7_0 - s12_0 * a8_0;
            float _S54 = s12_0 * a7_0 + c12_0 * a8_0;


            float _S55 = c12_0 * w7_0 - s12_0 * w8_0;
            float _S56 = s12_0 * w7_0 + c12_0 * w8_0;

#line 235
            a1_1 = c12_0 * a1_0 - s12_0 * a2_0;

#line 235
            a4_1 = _S49;

#line 235
            a7_1 = _S53;

#line 235
            w1_1 = _S47;

#line 235
            w4_1 = _S51;

#line 235
            w7_1 = _S55;

#line 235
            a2_1 = _S46;

#line 235
            a5_1 = _S50;

#line 235
            a8_1 = _S54;

#line 235
            w2_1 = _S48;

#line 235
            w5_1 = _S52;

#line 235
            w8_1 = _S56;

#line 207
        }
        else
        {

#line 207
            a1_1 = a1_0;

#line 207
            a4_1 = a4_0;

#line 207
            a7_1 = a7_0;

#line 207
            w1_1 = w1_0;

#line 207
            w4_1 = w4_0;

#line 207
            w7_1 = w7_0;

#line 207
            a2_1 = a2_0;

#line 207
            a5_1 = a5_0;

#line 207
            a8_1 = a8_0;

#line 207
            w2_1 = w2_0;

#line 207
            w5_1 = w5_0;

#line 207
            w8_1 = w8_0;

#line 207
        }

#line 137
        sweep_0 = sweep_0 + 1U;

#line 137
    }

#line 238
    float sg0_0 = (F32_sqrt((a0_1 * a0_1 + a3_1 * a3_1 + a6_1 * a6_1)));
    float sg1_0 = (F32_sqrt((a1_1 * a1_1 + a4_1 * a4_1 + a7_1 * a7_1)));
    float sg2_0 = (F32_sqrt((a2_1 * a2_1 + a5_1 * a5_1 + a8_1 * a8_1)));
    bool sw01_0 = sg1_0 > sg0_0;

    if(sw01_0)
    {

#line 243
        w1_0 = sg1_0;

#line 243
    }
    else
    {

#line 243
        w1_0 = sg0_0;

#line 243
    }
    if(sw01_0)
    {

#line 244
        w4_0 = sg0_0;

#line 244
    }
    else
    {

#line 244
        w4_0 = sg1_0;

#line 244
    }

    if(sw01_0)
    {

#line 246
        a0_0 = a1_1;

#line 246
    }
    else
    {

#line 246
        a0_0 = a0_1;

#line 246
    }
    if(sw01_0)
    {

#line 247
        a1_1 = a0_1;

#line 247
    }

    if(sw01_0)
    {

#line 249
        a3_0 = a4_1;

#line 249
    }
    else
    {

#line 249
        a3_0 = a3_1;

#line 249
    }
    if(sw01_0)
    {

#line 250
        a4_1 = a3_1;

#line 250
    }

    if(sw01_0)
    {

#line 252
        a6_0 = a7_1;

#line 252
    }
    else
    {

#line 252
        a6_0 = a6_1;

#line 252
    }
    if(sw01_0)
    {

#line 253
        a7_1 = a6_1;

#line 253
    }

    if(sw01_0)
    {

#line 255
        w0_0 = w1_1;

#line 255
    }
    else
    {

#line 255
        w0_0 = w0_1;

#line 255
    }
    if(sw01_0)
    {

#line 256
        w1_1 = w0_1;

#line 256
    }

    if(sw01_0)
    {

#line 258
        w3_0 = w4_1;

#line 258
    }
    else
    {

#line 258
        w3_0 = w3_1;

#line 258
    }
    if(sw01_0)
    {

#line 259
        w4_1 = w3_1;

#line 259
    }

    if(sw01_0)
    {

#line 261
        w6_0 = w7_1;

#line 261
    }
    else
    {

#line 261
        w6_0 = w6_1;

#line 261
    }
    if(sw01_0)
    {

#line 262
        w7_1 = w6_1;

#line 262
    }
    bool sw02_0 = sg2_0 > w1_0;

    if(sw02_0)
    {

#line 265
        w7_0 = sg2_0;

#line 265
    }
    else
    {

#line 265
        w7_0 = w1_0;

#line 265
    }
    if(sw02_0)
    {

#line 266
        a2_0 = w1_0;

#line 266
    }
    else
    {

#line 266
        a2_0 = sg2_0;

#line 266
    }

    if(sw02_0)
    {

#line 268
        a0_1 = a2_1;

#line 268
    }
    else
    {

#line 268
        a0_1 = a0_0;

#line 268
    }
    if(sw02_0)
    {

#line 269
        a2_1 = a0_0;

#line 269
    }

    if(sw02_0)
    {

#line 271
        a3_1 = a5_1;

#line 271
    }
    else
    {

#line 271
        a3_1 = a3_0;

#line 271
    }
    if(sw02_0)
    {

#line 272
        a5_1 = a3_0;

#line 272
    }

    if(sw02_0)
    {

#line 274
        a6_1 = a8_1;

#line 274
    }
    else
    {

#line 274
        a6_1 = a6_0;

#line 274
    }
    if(sw02_0)
    {

#line 275
        a8_1 = a6_0;

#line 275
    }

    if(sw02_0)
    {

#line 277
        w0_1 = w2_1;

#line 277
    }
    else
    {

#line 277
        w0_1 = w0_0;

#line 277
    }
    if(sw02_0)
    {

#line 278
        w2_1 = w0_0;

#line 278
    }

    if(sw02_0)
    {

#line 280
        w3_1 = w5_1;

#line 280
    }
    else
    {

#line 280
        w3_1 = w3_0;

#line 280
    }
    if(sw02_0)
    {

#line 281
        w5_1 = w3_0;

#line 281
    }

    if(sw02_0)
    {

#line 283
        w6_1 = w8_1;

#line 283
    }
    else
    {

#line 283
        w6_1 = w6_0;

#line 283
    }
    if(sw02_0)
    {

#line 284
        w8_1 = w6_0;

#line 284
    }
    bool sw12_0 = a2_0 > w4_0;

    if(sw12_0)
    {

#line 287
        a5_0 = a2_0;

#line 287
    }
    else
    {

#line 287
        a5_0 = w4_0;

#line 287
    }
    if(sw12_0)
    {

#line 288
        a2_0 = w4_0;

#line 288
    }

    if(sw12_0)
    {

#line 290
        a1_0 = a2_1;

#line 290
    }
    else
    {

#line 290
        a1_0 = a1_1;

#line 290
    }
    if(sw12_0)
    {

#line 291
        a2_1 = a1_1;

#line 291
    }

    if(sw12_0)
    {

#line 293
        a4_0 = a5_1;

#line 293
    }
    else
    {

#line 293
        a4_0 = a4_1;

#line 293
    }
    if(sw12_0)
    {

#line 294
        a5_1 = a4_1;

#line 294
    }

    if(sw12_0)
    {

#line 296
        a7_0 = a8_1;

#line 296
    }
    else
    {

#line 296
        a7_0 = a7_1;

#line 296
    }
    if(sw12_0)
    {

#line 297
        a8_1 = a7_1;

#line 297
    }

    if(sw12_0)
    {

#line 299
        a1_1 = w2_1;

#line 299
    }
    else
    {

#line 299
        a1_1 = w1_1;

#line 299
    }
    if(sw12_0)
    {

#line 300
        a4_1 = w1_1;

#line 300
    }
    else
    {

#line 300
        a4_1 = w2_1;

#line 300
    }

    if(sw12_0)
    {

#line 302
        a7_1 = w5_1;

#line 302
    }
    else
    {

#line 302
        a7_1 = w4_1;

#line 302
    }
    if(sw12_0)
    {

#line 303
        w1_1 = w4_1;

#line 303
    }
    else
    {

#line 303
        w1_1 = w5_1;

#line 303
    }

    if(sw12_0)
    {

#line 305
        w4_1 = w8_1;

#line 305
    }
    else
    {

#line 305
        w4_1 = w7_1;

#line 305
    }
    if(sw12_0)
    {
    }
    else
    {

#line 306
        w7_1 = w8_1;

#line 306
    }
    float eps_0 = 9.99999997475242708e-07f * w7_0;
    bool _S57 = w7_0 > eps_0;

#line 308
    if(_S57)
    {

#line 308
        a8_0 = a0_1 / w7_0;

#line 308
    }
    else
    {

#line 308
        a8_0 = 0.0f;

#line 308
    }
    if(_S57)
    {

#line 309
        w2_0 = a3_1 / w7_0;

#line 309
    }
    else
    {

#line 309
        w2_0 = 0.0f;

#line 309
    }
    if(_S57)
    {

#line 310
        w5_0 = a6_1 / w7_0;

#line 310
    }
    else
    {

#line 310
        w5_0 = 0.0f;

#line 310
    }
    bool _S58 = a5_0 > eps_0;

#line 311
    if(_S58)
    {

#line 311
        w8_0 = a1_0 / a5_0;

#line 311
    }
    else
    {

#line 311
        w8_0 = 0.0f;

#line 311
    }
    if(_S58)
    {

#line 312
        u1_1_0 = a4_0 / a5_0;

#line 312
    }
    else
    {

#line 312
        u1_1_0 = 0.0f;

#line 312
    }

#line 312
    float u1_2_0;
    if(_S58)
    {

#line 313
        u1_2_0 = a7_0 / a5_0;

#line 313
    }
    else
    {

#line 313
        u1_2_0 = 0.0f;

#line 313
    }
    bool _S59 = a2_0 > eps_0;

#line 314
    if(_S59)
    {

#line 314
        a0_1 = a2_1 / a2_0;

#line 314
    }
    else
    {

#line 314
        a0_1 = 0.0f;

#line 314
    }
    if(_S59)
    {

#line 315
        a3_1 = a5_1 / a2_0;

#line 315
    }
    else
    {

#line 315
        a3_1 = 0.0f;

#line 315
    }
    if(_S59)
    {

#line 316
        a6_1 = a8_1 / a2_0;

#line 316
    }
    else
    {

#line 316
        a6_1 = 0.0f;

#line 316
    }

#line 316
    float u2_0_0;

#line 316
    float u2_1_0;

#line 316
    float u2_2_0;

#line 316
    float d_0;

#line 323
    if(w7_0 <= eps_0)
    {

#line 303
        float _S60 = w1_1;

        float _S61 = w4_1;
        float _S62 = w7_1;

#line 306
        a8_0 = 1.0f;

#line 306
        w8_0 = 0.0f;

#line 306
        w1_1 = a1_1;

#line 306
        u2_0_0 = 0.0f;

#line 306
        w2_1 = a4_1;

#line 306
        w4_1 = a7_1;

#line 306
        w5_1 = _S60;

#line 306
        w7_1 = _S61;

#line 306
        w8_1 = _S62;

#line 306
        w2_0 = 0.0f;

#line 306
        u1_1_0 = 1.0f;

#line 306
        u2_1_0 = 0.0f;

#line 306
        w5_0 = 0.0f;

#line 306
        u1_2_0 = 0.0f;

#line 306
        u2_2_0 = 1.0f;

#line 323
    }
    else
    {

#line 334
        if(a5_0 <= eps_0)
        {

#line 335
            bool kc_fy_0 = (F32_abs((w3_1))) < 0.89999997615814209f;
            if(kc_fy_0)
            {

#line 336
                u2_0_0 = 0.0f;

#line 336
            }
            else
            {

#line 336
                u2_0_0 = 1.0f;

#line 336
            }
            if(kc_fy_0)
            {

#line 337
                u2_1_0 = 1.0f;

#line 337
            }
            else
            {

#line 337
                u2_1_0 = 0.0f;

#line 337
            }

            float kc_v1r0_0 = w3_1 * 0.0f - w6_1 * u2_1_0;
            float kc_v1r1_0 = w6_1 * u2_0_0 - w0_1 * 0.0f;
            float kc_v1r2_0 = w0_1 * u2_1_0 - w3_1 * u2_0_0;
            float kc_v1_l_0 = (F32_sqrt((kc_v1r0_0 * kc_v1r0_0 + kc_v1r1_0 * kc_v1r1_0 + kc_v1r2_0 * kc_v1r2_0)));
            bool _S63 = kc_v1_l_0 > 9.999999960041972e-13f;

#line 343
            if(_S63)
            {

#line 343
                u2_2_0 = kc_v1r0_0 / kc_v1_l_0;

#line 343
            }
            else
            {

#line 343
                u2_2_0 = kc_v1r0_0;

#line 343
            }
            if(_S63)
            {

#line 344
                d_0 = kc_v1r1_0 / kc_v1_l_0;

#line 344
            }
            else
            {

#line 344
                d_0 = kc_v1r1_0;

#line 344
            }

#line 344
            float kc_v12_0;
            if(_S63)
            {

#line 345
                kc_v12_0 = kc_v1r2_0 / kc_v1_l_0;

#line 345
            }
            else
            {

#line 345
                kc_v12_0 = kc_v1r2_0;

#line 345
            }
            float kc_v2r0_0 = w3_1 * kc_v12_0 - w6_1 * d_0;
            float kc_v2r1_0 = w6_1 * u2_2_0 - w0_1 * kc_v12_0;
            float kc_v2r2_0 = w0_1 * d_0 - w3_1 * u2_2_0;
            float kc_v2_l_0 = (F32_sqrt((kc_v2r0_0 * kc_v2r0_0 + kc_v2r1_0 * kc_v2r1_0 + kc_v2r2_0 * kc_v2r2_0)));
            bool _S64 = kc_v2_l_0 > 9.999999960041972e-13f;

#line 350
            float kc_v20_0;

#line 350
            if(_S64)
            {

#line 350
                kc_v20_0 = kc_v2r0_0 / kc_v2_l_0;

#line 350
            }
            else
            {

#line 350
                kc_v20_0 = kc_v2r0_0;

#line 350
            }

#line 350
            float kc_v21_0;
            if(_S64)
            {

#line 351
                kc_v21_0 = kc_v2r1_0 / kc_v2_l_0;

#line 351
            }
            else
            {

#line 351
                kc_v21_0 = kc_v2r1_0;

#line 351
            }

#line 351
            float kc_v22_0;
            if(_S64)
            {

#line 352
                kc_v22_0 = kc_v2r2_0 / kc_v2_l_0;

#line 352
            }
            else
            {

#line 352
                kc_v22_0 = kc_v2r2_0;

#line 352
            }
            float kc_ax0_0 = w3_1 * w5_0 - w6_1 * w2_0;
            float kc_ax1_0 = w6_1 * a8_0 - w0_1 * w5_0;
            float kc_ax2_0 = w0_1 * w2_0 - w3_1 * a8_0;
            float kc_sn_0 = (F32_sqrt((kc_ax0_0 * kc_ax0_0 + kc_ax1_0 * kc_ax1_0 + kc_ax2_0 * kc_ax2_0)));
            float kc_cs_0 = w0_1 * a8_0 + w3_1 * w2_0 + w6_1 * w5_0;
            float _S65 = (F32_max((kc_sn_0), (9.99999971718068537e-10f)));

#line 358
            float kc_k0_0 = kc_ax0_0 / _S65;
            float kc_k1_0 = kc_ax1_0 / _S65;
            float kc_k2_0 = kc_ax2_0 / _S65;

#line 365
            float _S66 = 1.0f - kc_cs_0;

#line 365
            float _S67 = (kc_k0_0 * u2_2_0 + kc_k1_0 * d_0 + kc_k2_0 * kc_v12_0) * _S66;

#line 365
            float _S68 = kc_cs_0 * u2_2_0 + kc_sn_0 * (kc_k1_0 * kc_v12_0 - kc_k2_0 * d_0) + _S67 * kc_k0_0;
            float _S69 = kc_cs_0 * d_0 + kc_sn_0 * (kc_k2_0 * u2_2_0 - kc_k0_0 * kc_v12_0) + _S67 * kc_k1_0;
            float _S70 = kc_cs_0 * kc_v12_0 + kc_sn_0 * (kc_k0_0 * d_0 - kc_k1_0 * u2_2_0) + _S67 * kc_k2_0;

#line 372
            float _S71 = (kc_k0_0 * kc_v20_0 + kc_k1_0 * kc_v21_0 + kc_k2_0 * kc_v22_0) * _S66;

#line 372
            float _S72 = kc_cs_0 * kc_v20_0 + kc_sn_0 * (kc_k1_0 * kc_v22_0 - kc_k2_0 * kc_v21_0) + _S71 * kc_k0_0;
            float _S73 = kc_cs_0 * kc_v21_0 + kc_sn_0 * (kc_k2_0 * kc_v20_0 - kc_k0_0 * kc_v22_0) + _S71 * kc_k1_0;
            float _S74 = kc_cs_0 * kc_v22_0 + kc_sn_0 * (kc_k0_0 * kc_v21_0 - kc_k1_0 * kc_v20_0) + _S71 * kc_k2_0;

#line 374
            float kc_c10_0;

#line 374
            float kc_c11_0;

#line 374
            float kc_c12_0;

#line 374
            float kc_c20_0;

#line 374
            float kc_c21_0;

#line 374
            float kc_c22_0;
            if(kc_sn_0 < 9.99999971718068537e-10f)
            {

#line 376
                if(kc_cs_0 > 0.0f)
                {

#line 376
                    kc_c10_0 = u2_2_0;

#line 376
                    kc_c11_0 = d_0;

#line 376
                    kc_c12_0 = kc_v12_0;

#line 376
                    kc_c20_0 = kc_v20_0;

#line 376
                    kc_c21_0 = kc_v21_0;

#line 376
                    kc_c22_0 = kc_v22_0;

#line 376
                }
                else
                {

#line 384
                    float _S75 = (F32_abs((a8_0)));

#line 384
                    bool kc_px_0;

#line 384
                    if(_S75 <= (F32_abs((w2_0))))
                    {

#line 384
                        kc_px_0 = _S75 <= (F32_abs((w5_0)));

#line 384
                    }
                    else
                    {

#line 384
                        kc_px_0 = false;

#line 384
                    }
                    if(kc_px_0)
                    {

#line 385
                        kc_c10_0 = 1.0f;

#line 385
                    }
                    else
                    {

#line 385
                        kc_c10_0 = 0.0f;

#line 385
                    }
                    if(kc_px_0)
                    {

#line 386
                        kc_c11_0 = 0.0f;

#line 386
                    }
                    else
                    {

#line 386
                        kc_c11_0 = 1.0f;

#line 386
                    }

                    float kc_p10_0 = w2_0 * 0.0f - w5_0 * kc_c11_0;
                    float kc_p11_0 = w5_0 * kc_c10_0 - a8_0 * 0.0f;
                    float kc_p12_0 = a8_0 * kc_c11_0 - w2_0 * kc_c10_0;
                    float kc_p1n_l_0 = (F32_sqrt((kc_p10_0 * kc_p10_0 + kc_p11_0 * kc_p11_0 + kc_p12_0 * kc_p12_0)));
                    bool _S76 = kc_p1n_l_0 > 9.999999960041972e-13f;

#line 392
                    if(_S76)
                    {

#line 392
                        kc_c12_0 = kc_p10_0 / kc_p1n_l_0;

#line 392
                    }
                    else
                    {

#line 392
                        kc_c12_0 = kc_p10_0;

#line 392
                    }
                    if(_S76)
                    {

#line 393
                        kc_c20_0 = kc_p11_0 / kc_p1n_l_0;

#line 393
                    }
                    else
                    {

#line 393
                        kc_c20_0 = kc_p11_0;

#line 393
                    }
                    if(_S76)
                    {

#line 394
                        kc_c21_0 = kc_p12_0 / kc_p1n_l_0;

#line 394
                    }
                    else
                    {

#line 394
                        kc_c21_0 = kc_p12_0;

#line 394
                    }
                    float kc_p20_0 = w2_0 * kc_c21_0 - w5_0 * kc_c20_0;
                    float kc_p21_0 = w5_0 * kc_c12_0 - a8_0 * kc_c21_0;
                    float kc_p22_0 = a8_0 * kc_c20_0 - w2_0 * kc_c12_0;
                    float kc_p2n_l_0 = (F32_sqrt((kc_p20_0 * kc_p20_0 + kc_p21_0 * kc_p21_0 + kc_p22_0 * kc_p22_0)));
                    bool _S77 = kc_p2n_l_0 > 9.999999960041972e-13f;

#line 399
                    if(_S77)
                    {

#line 399
                        kc_c22_0 = kc_p20_0 / kc_p2n_l_0;

#line 399
                    }
                    else
                    {

#line 399
                        kc_c22_0 = kc_p20_0;

#line 399
                    }

#line 399
                    float kc_p2n1_0;
                    if(_S77)
                    {

#line 400
                        kc_p2n1_0 = kc_p21_0 / kc_p2n_l_0;

#line 400
                    }
                    else
                    {

#line 400
                        kc_p2n1_0 = kc_p21_0;

#line 400
                    }

#line 400
                    float kc_p2n2_0;
                    if(_S77)
                    {

#line 401
                        kc_p2n2_0 = kc_p22_0 / kc_p2n_l_0;

#line 401
                    }
                    else
                    {

#line 401
                        kc_p2n2_0 = kc_p22_0;

#line 401
                    }

#line 401
                    kc_c10_0 = kc_c12_0;

#line 401
                    kc_c11_0 = kc_c20_0;

#line 401
                    kc_c12_0 = kc_c21_0;

#line 401
                    kc_c20_0 = kc_c22_0;

#line 401
                    kc_c21_0 = kc_p2n1_0;

#line 401
                    kc_c22_0 = kc_p2n2_0;

#line 376
                }

#line 375
            }
            else
            {

#line 375
                kc_c10_0 = _S68;

#line 375
                kc_c11_0 = _S69;

#line 375
                kc_c12_0 = _S70;

#line 375
                kc_c20_0 = _S72;

#line 375
                kc_c21_0 = _S73;

#line 375
                kc_c22_0 = _S74;

#line 375
            }

#line 375
            w8_0 = kc_c10_0;

#line 375
            w1_1 = u2_2_0;

#line 375
            u2_0_0 = kc_c20_0;

#line 375
            w2_1 = kc_v20_0;

#line 375
            w4_1 = d_0;

#line 375
            w5_1 = kc_v21_0;

#line 375
            w7_1 = kc_v12_0;

#line 375
            w8_1 = kc_v22_0;

#line 375
            u1_1_0 = kc_c11_0;

#line 375
            u2_1_0 = kc_c21_0;

#line 375
            u1_2_0 = kc_c12_0;

#line 375
            u2_2_0 = kc_c22_0;

#line 334
        }
        else
        {

#line 423
            if(a2_0 <= eps_0)
            {

#line 424
                float kx0_0 = w2_0 * u1_2_0 - w5_0 * u1_1_0;
                float kx1_0 = w5_0 * w8_0 - a8_0 * u1_2_0;
                float kx2_0 = a8_0 * u1_1_0 - w2_0 * w8_0;
                float kn_l_0 = (F32_sqrt((kx0_0 * kx0_0 + kx1_0 * kx1_0 + kx2_0 * kx2_0)));
                bool _S78 = kn_l_0 > 9.999999960041972e-13f;

#line 428
                if(_S78)
                {

#line 428
                    u2_0_0 = kx0_0 / kn_l_0;

#line 428
                }
                else
                {

#line 428
                    u2_0_0 = kx0_0;

#line 428
                }
                if(_S78)
                {

#line 429
                    u2_1_0 = kx1_0 / kn_l_0;

#line 429
                }
                else
                {

#line 429
                    u2_1_0 = kx1_0;

#line 429
                }
                if(_S78)
                {

#line 430
                    u2_2_0 = kx2_0 / kn_l_0;

#line 430
                }
                else
                {

#line 430
                    u2_2_0 = kx2_0;

#line 430
                }

#line 423
            }
            else
            {

#line 423
                u2_0_0 = a0_1;

#line 423
                u2_1_0 = a3_1;

#line 423
                u2_2_0 = a6_1;

#line 423
            }

#line 303
            float _S79 = w1_1;

            float _S80 = w4_1;
            float _S81 = w7_1;

#line 306
            w1_1 = a1_1;

#line 306
            w2_1 = a4_1;

#line 306
            w4_1 = a7_1;

#line 306
            w5_1 = _S79;

#line 306
            w7_1 = _S80;

#line 306
            w8_1 = _S81;

#line 334
        }

#line 323
    }

#line 437
    float _S82 = a8_0 * w0_1 + w8_0 * w1_1;
    float _S83 = a8_0 * w3_1 + w8_0 * w4_1;
    float _S84 = a8_0 * w6_1 + w8_0 * w7_1;
    float _S85 = w2_0 * w0_1 + u1_1_0 * w1_1;

#line 440
    float P3_0 = _S85 + u2_1_0 * w2_1;
    float _S86 = w2_0 * w3_1 + u1_1_0 * w4_1;

#line 441
    float P4_0 = _S86 + u2_1_0 * w5_1;
    float _S87 = w2_0 * w6_1 + u1_1_0 * w7_1;

#line 442
    float P5_0 = _S87 + u2_1_0 * w8_1;
    float _S88 = w5_0 * w0_1 + u1_2_0 * w1_1;

#line 443
    float P6_0 = _S88 + u2_2_0 * w2_1;
    float _S89 = w5_0 * w3_1 + u1_2_0 * w4_1;

#line 444
    float P7_0 = _S89 + u2_2_0 * w5_1;
    float _S90 = w5_0 * w6_1 + u1_2_0 * w7_1;

#line 445
    float P8_0 = _S90 + u2_2_0 * w8_1;

    if(((_S82 + u2_0_0 * w2_1) * (P4_0 * P8_0 - P5_0 * P7_0) + (_S83 + u2_0_0 * w5_1) * (P5_0 * P6_0 - P3_0 * P8_0) + (_S84 + u2_0_0 * w8_1) * (P3_0 * P7_0 - P4_0 * P6_0)) >= 0.0f)
    {

#line 447
        d_0 = 1.0f;

#line 447
    }
    else
    {

#line 447
        d_0 = -1.0f;

#line 447
    }
    float _S91 = d_0 * u2_0_0;
    float R1_0 = _S83 + _S91 * w5_1;
    float R2_0 = _S84 + _S91 * w8_1;
    float _S92 = d_0 * u2_1_0;

#line 451
    float R3_0 = _S85 + _S92 * w2_1;
    float R4_0 = _S86 + _S92 * w5_1;
    float R5_0 = _S87 + _S92 * w8_1;
    float _S93 = d_0 * u2_2_0;

#line 454
    float R6_0 = _S88 + _S93 * w2_1;
    float R7_0 = _S89 + _S93 * w5_1;
    float R8_0 = _S90 + _S93 * w8_1;
    uint32_t _S94 = i_0 * 9U;

#line 457
    *(&((&kernelContext_0)->globalParams_0->rot_0)[_S94]) = _S82 + _S91 * w2_1;
    *(&((&kernelContext_0)->globalParams_0->rot_0)[_S94 + 1U]) = R1_0;
    *(&((&kernelContext_0)->globalParams_0->rot_0)[_S94 + 2U]) = R2_0;
    *(&((&kernelContext_0)->globalParams_0->rot_0)[_S94 + 3U]) = R3_0;
    *(&((&kernelContext_0)->globalParams_0->rot_0)[_S94 + 4U]) = R4_0;
    *(&((&kernelContext_0)->globalParams_0->rot_0)[_S94 + 5U]) = R5_0;
    *(&((&kernelContext_0)->globalParams_0->rot_0)[_S94 + 6U]) = R6_0;
    *(&((&kernelContext_0)->globalParams_0->rot_0)[_S94 + 7U]) = R7_0;
    *(&((&kernelContext_0)->globalParams_0->rot_0)[_S94 + 8U]) = R8_0;
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
