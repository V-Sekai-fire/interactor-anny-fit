#include "slang_rt/slang-cpp-prelude.h"

#ifdef SLANG_PRELUDE_NAMESPACE
using namespace SLANG_PRELUDE_NAMESPACE;
#endif


#line 6993 "hlsl.meta.slang"
struct GlobalParams_0
{
    StructuredBuffer<Vector<float, 3> > vanny_0;
    StructuredBuffer<Vector<float, 3> > p3_0;
    StructuredBuffer<uint32_t> Ftet_0;
    StructuredBuffer<uint32_t> fids_0;
    StructuredBuffer<float> bary_0;
    RWStructuredBuffer<Vector<float, 3> > v0_0;
};


#line 6993
struct KernelContext_0
{
    GlobalParams_0* globalParams_0;
};


#line 13 "E:/sinew-moved/viz_native/../spec/slang/pheno_bary_gather.slang"
void _bary_gather(void* _S1, void* entryPointParams_0, void* globalParams_1)
{

#line 13
    ComputeThreadVaryingInput * _S2 = (slang_bit_cast<ComputeThreadVaryingInput *>(_S1));

#line 13
    KernelContext_0 kernelContext_0;

#line 13
    (&kernelContext_0)->globalParams_0 = (slang_bit_cast<GlobalParams_0*>(globalParams_1));

    uint32_t v_0 = (_S2->groupID * Vector<uint32_t, 3> (64U, 1U, 1U) + _S2->groupThreadID).x;
    uint _elementCount_0;
    uint _stride_0;
    (slang_bit_cast<GlobalParams_0*>(globalParams_1))->v0_0.GetDimensions(&_elementCount_0, &_stride_0);
    Vector<uint32_t, 2>  _S3 = uint2(_elementCount_0, _stride_0);

#line 17
    if(v_0 >= (_S3.x))
    {

#line 17
        return;
    }

#line 18
    uint _elementCount_1;
    uint _stride_1;
    (&kernelContext_0)->globalParams_0->vanny_0.GetDimensions(&_elementCount_1, &_stride_1);
    Vector<uint32_t, 2>  _S4 = uint2(_elementCount_1, _stride_1);

#line 18
    uint32_t _S5 = _S4.x;
    uint32_t _S6 = (&kernelContext_0)->globalParams_0->fids_0.Load(v_0);
    Vector<float, 3>  _S7 = (Vector<float, 3> )0.0f;

#line 20
    uint32_t k_0 = 0U;

#line 20
    Vector<float, 3>  r_0 = _S7;
    for(;;)
    {

#line 21
        if(k_0 < 4U)
        {
        }
        else
        {

#line 21
            break;
        }
        uint32_t idx_0 = (&kernelContext_0)->globalParams_0->Ftet_0.Load(_S6 * 4U + k_0);

#line 23
        Vector<float, 3>  pt_0;
        if(idx_0 < _S5)
        {

#line 24
            pt_0 = (&kernelContext_0)->globalParams_0->vanny_0.Load(idx_0);

#line 24
        }
        else
        {

#line 24
            pt_0 = (&kernelContext_0)->globalParams_0->p3_0.Load(idx_0 - _S5);

#line 24
        }
        Vector<float, 3>  r_1 = r_0 + (Vector<float, 3> )(&kernelContext_0)->globalParams_0->bary_0.Load(v_0 * 4U + k_0) * pt_0;

#line 21
        k_0 = k_0 + 1U;

#line 21
        r_0 = r_1;

#line 21
    }

#line 27
    *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->v0_0)[v_0]) = Vector<float, 3> (r_0.x, r_0.z, - r_0.y);
    return;
}

// [numthreads(64, 1, 1)]
SLANG_PRELUDE_EXPORT
void bary_gather_Thread(ComputeThreadVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    _bary_gather(varyingInput, entryPointParams, globalParams);
}
// [numthreads(64, 1, 1)]
SLANG_PRELUDE_EXPORT
void bary_gather_Group(ComputeVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    ComputeThreadVaryingInput threadInput = {};
    threadInput.groupID = varyingInput->startGroupID;
    for (uint32_t x = 0; x < 64; ++x)
    {
        threadInput.groupThreadID.x = x;
        _bary_gather(&threadInput, entryPointParams, globalParams);
    }
}
// [numthreads(64, 1, 1)]
SLANG_PRELUDE_EXPORT
void bary_gather(ComputeVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
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
                bary_gather_Group(&groupVaryingInput, entryPointParams, globalParams);
            }
        }
    }
}
