#include "slang_rt/slang-cpp-prelude.h"

#ifdef SLANG_PRELUDE_NAMESPACE
using namespace SLANG_PRELUDE_NAMESPACE;
#endif


#line 6993 "hlsl.meta.slang"
struct GlobalParams_0
{
    StructuredBuffer<Vector<float, 3> > vanny_0;
    StructuredBuffer<uint32_t> Fsrc_0;
    RWStructuredBuffer<Vector<float, 3> > p3_0;
};


#line 6993
struct KernelContext_0
{
    GlobalParams_0* globalParams_0;
};


#line 9310
static Vector<float, 3>  cross_0(Vector<float, 3>  left_0, Vector<float, 3>  right_0)
{

#line 9324
    float _S1 = left_0.y;

#line 9324
    float _S2 = right_0.z;

#line 9324
    float _S3 = left_0.z;

#line 9324
    float _S4 = right_0.y;
    float _S5 = right_0.x;

#line 9325
    float _S6 = left_0.x;

#line 9323
    return Vector<float, 3> (_S1 * _S2 - _S3 * _S4, _S3 * _S5 - _S6 * _S2, _S6 * _S4 - _S1 * _S5);
}


#line 10 "E:/sinew-moved/viz_native/../spec/slang/pheno_bary_tet.slang"
void _bary_tet(void* _S7, void* entryPointParams_0, void* globalParams_1)
{

#line 10
    ComputeThreadVaryingInput * _S8 = (slang_bit_cast<ComputeThreadVaryingInput *>(_S7));

#line 10
    KernelContext_0 kernelContext_0;

#line 10
    (&kernelContext_0)->globalParams_0 = (slang_bit_cast<GlobalParams_0*>(globalParams_1));

    uint32_t f_0 = (_S8->groupID * Vector<uint32_t, 3> (64U, 1U, 1U) + _S8->groupThreadID).x;
    uint _elementCount_0;
    uint _stride_0;
    (slang_bit_cast<GlobalParams_0*>(globalParams_1))->p3_0.GetDimensions(&_elementCount_0, &_stride_0);
    Vector<uint32_t, 2>  _S9 = uint2(_elementCount_0, _stride_0);

#line 14
    if(f_0 >= (_S9.x))
    {

#line 14
        return;
    }

#line 15
    uint32_t _S10 = f_0 * 3U;

#line 15
    Vector<float, 3>  a_0 = (&kernelContext_0)->globalParams_0->vanny_0.Load((&kernelContext_0)->globalParams_0->Fsrc_0.Load(_S10));


    *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->p3_0)[f_0]) = a_0 + cross_0((&kernelContext_0)->globalParams_0->vanny_0.Load((&kernelContext_0)->globalParams_0->Fsrc_0.Load(_S10 + 1U)) - a_0, (&kernelContext_0)->globalParams_0->vanny_0.Load((&kernelContext_0)->globalParams_0->Fsrc_0.Load(_S10 + 2U)) - a_0);
    return;
}

// [numthreads(64, 1, 1)]
SLANG_PRELUDE_EXPORT
void bary_tet_Thread(ComputeThreadVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    _bary_tet(varyingInput, entryPointParams, globalParams);
}
// [numthreads(64, 1, 1)]
SLANG_PRELUDE_EXPORT
void bary_tet_Group(ComputeVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    ComputeThreadVaryingInput threadInput = {};
    threadInput.groupID = varyingInput->startGroupID;
    for (uint32_t x = 0; x < 64; ++x)
    {
        threadInput.groupThreadID.x = x;
        _bary_tet(&threadInput, entryPointParams, globalParams);
    }
}
// [numthreads(64, 1, 1)]
SLANG_PRELUDE_EXPORT
void bary_tet(ComputeVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
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
                bary_tet_Group(&groupVaryingInput, entryPointParams, globalParams);
            }
        }
    }
}
