#include "slang_rt/slang-cpp-prelude.h"

#ifdef SLANG_PRELUDE_NAMESPACE
using namespace SLANG_PRELUDE_NAMESPACE;
#endif


#line 6993 "hlsl.meta.slang"
struct GlobalParams_0
{
    StructuredBuffer<Vector<float, 3> > v0_0;
    StructuredBuffer<uint32_t> crow_0;
    StructuredBuffer<uint32_t> col_0;
    StructuredBuffer<float> val_0;
    RWStructuredBuffer<Vector<float, 3> > jpos_0;
};


#line 6993
struct KernelContext_0
{
    GlobalParams_0* globalParams_0;
};


#line 12 "E:/sinew-moved/viz_native/../spec/slang/pheno_rbf.slang"
void _rbf(void* _S1, void* entryPointParams_0, void* globalParams_1)
{

#line 12
    ComputeThreadVaryingInput * _S2 = (slang_bit_cast<ComputeThreadVaryingInput *>(_S1));

#line 12
    KernelContext_0 kernelContext_0;

#line 12
    (&kernelContext_0)->globalParams_0 = (slang_bit_cast<GlobalParams_0*>(globalParams_1));

    uint32_t i_0 = (_S2->groupID * Vector<uint32_t, 3> (64U, 1U, 1U) + _S2->groupThreadID).x;
    uint _elementCount_0;
    uint _stride_0;
    (slang_bit_cast<GlobalParams_0*>(globalParams_1))->jpos_0.GetDimensions(&_elementCount_0, &_stride_0);
    Vector<uint32_t, 2>  _S3 = uint2(_elementCount_0, _stride_0);

#line 16
    if(i_0 >= (_S3.x))
    {

#line 16
        return;
    }

#line 17
    Vector<float, 3>  _S4 = (Vector<float, 3> )0.0f;

#line 17
    uint32_t k_0 = (&kernelContext_0)->globalParams_0->crow_0.Load(i_0);

#line 17
    Vector<float, 3>  acc_0 = _S4;
    for(;;)
    {

#line 18
        if(k_0 < ((&kernelContext_0)->globalParams_0->crow_0).Load(i_0 + 1U))
        {
        }
        else
        {

#line 18
            break;
        }

#line 19
        Vector<float, 3>  acc_1 = acc_0 + (Vector<float, 3> )(&kernelContext_0)->globalParams_0->val_0.Load(k_0) * (&kernelContext_0)->globalParams_0->v0_0.Load((&kernelContext_0)->globalParams_0->col_0.Load(k_0));

#line 18
        k_0 = k_0 + 1U;

#line 18
        acc_0 = acc_1;

#line 18
    }

    *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->jpos_0)[i_0]) = acc_0;
    return;
}

// [numthreads(64, 1, 1)]
SLANG_PRELUDE_EXPORT
void rbf_Thread(ComputeThreadVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    _rbf(varyingInput, entryPointParams, globalParams);
}
// [numthreads(64, 1, 1)]
SLANG_PRELUDE_EXPORT
void rbf_Group(ComputeVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    ComputeThreadVaryingInput threadInput = {};
    threadInput.groupID = varyingInput->startGroupID;
    for (uint32_t x = 0; x < 64; ++x)
    {
        threadInput.groupThreadID.x = x;
        _rbf(&threadInput, entryPointParams, globalParams);
    }
}
// [numthreads(64, 1, 1)]
SLANG_PRELUDE_EXPORT
void rbf(ComputeVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
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
                rbf_Group(&groupVaryingInput, entryPointParams, globalParams);
            }
        }
    }
}
