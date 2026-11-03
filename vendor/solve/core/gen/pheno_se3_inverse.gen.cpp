#include "slang_rt/slang-cpp-prelude.h"

#ifdef SLANG_PRELUDE_NAMESPACE
using namespace SLANG_PRELUDE_NAMESPACE;
#endif


#line 6993 "hlsl.meta.slang"
struct GlobalParams_0
{
    StructuredBuffer<float> bw_0;
    RWStructuredBuffer<float> inv_0;
};


#line 6993
struct KernelContext_0
{
    GlobalParams_0* globalParams_0;
};


#line 9 "E:/sinew-moved/viz_native/../spec/slang/pheno_se3_inverse.slang"
void _se3_inverse(void* _S1, void* entryPointParams_0, void* globalParams_1)
{

#line 9
    ComputeThreadVaryingInput * _S2 = (slang_bit_cast<ComputeThreadVaryingInput *>(_S1));

#line 9
    KernelContext_0 kernelContext_0;

#line 9
    (&kernelContext_0)->globalParams_0 = (slang_bit_cast<GlobalParams_0*>(globalParams_1));

    uint32_t j_0 = (_S2->groupID * Vector<uint32_t, 3> (64U, 1U, 1U) + _S2->groupThreadID).x;
    uint _elementCount_0;
    uint _stride_0;
    (slang_bit_cast<GlobalParams_0*>(globalParams_1))->inv_0.GetDimensions(&_elementCount_0, &_stride_0);
    Vector<uint32_t, 2>  _S3 = uint2(_elementCount_0, _stride_0);

#line 13
    if(j_0 >= (_S3.x / 16U))
    {

#line 13
        return;
    }

#line 14
    uint32_t o_0 = j_0 * 16U;
    float _S4 = (&kernelContext_0)->globalParams_0->bw_0.Load(o_0 + 3U);

#line 15
    float _S5 = (&kernelContext_0)->globalParams_0->bw_0.Load(o_0 + 7U);

#line 15
    float _S6 = (&kernelContext_0)->globalParams_0->bw_0.Load(o_0 + 11U);

#line 15
    int32_t r_0 = int(0);
    for(;;)
    {

#line 16
        if(r_0 < int(3))
        {
        }
        else
        {

#line 16
            break;
        }

#line 16
        int32_t c_0 = int(0);
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

#line 17
            *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->inv_0)[o_0 + uint32_t(r_0 * int(4)) + uint32_t(c_0)]) = (&kernelContext_0)->globalParams_0->bw_0.Load(o_0 + uint32_t(c_0 * int(4)) + uint32_t(r_0));

#line 17
            c_0 = c_0 + int(1);

#line 17
        }
        uint32_t _S7 = uint32_t(r_0);

#line 18
        *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->inv_0)[o_0 + uint32_t(r_0 * int(4)) + 3U]) = - ((&kernelContext_0)->globalParams_0->bw_0.Load(o_0 + _S7) * _S4 + (&kernelContext_0)->globalParams_0->bw_0.Load(o_0 + 4U + _S7) * _S5 + (&kernelContext_0)->globalParams_0->bw_0.Load(o_0 + 8U + _S7) * _S6);

#line 16
        r_0 = r_0 + int(1);

#line 16
    }



    *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->inv_0)[o_0 + 12U]) = 0.0f;

#line 20
    *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->inv_0)[o_0 + 13U]) = 0.0f;

#line 20
    *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->inv_0)[o_0 + 14U]) = 0.0f;

#line 20
    *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->inv_0)[o_0 + 15U]) = 1.0f;
    return;
}

// [numthreads(64, 1, 1)]
SLANG_PRELUDE_EXPORT
void se3_inverse_Thread(ComputeThreadVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    _se3_inverse(varyingInput, entryPointParams, globalParams);
}
// [numthreads(64, 1, 1)]
SLANG_PRELUDE_EXPORT
void se3_inverse_Group(ComputeVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    ComputeThreadVaryingInput threadInput = {};
    threadInput.groupID = varyingInput->startGroupID;
    for (uint32_t x = 0; x < 64; ++x)
    {
        threadInput.groupThreadID.x = x;
        _se3_inverse(&threadInput, entryPointParams, globalParams);
    }
}
// [numthreads(64, 1, 1)]
SLANG_PRELUDE_EXPORT
void se3_inverse(ComputeVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
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
                se3_inverse_Group(&groupVaryingInput, entryPointParams, globalParams);
            }
        }
    }
}
