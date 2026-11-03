#include "SdfSpline.hpp"

#include <stdexcept>

#ifdef FIT_KERNELS_PENDING

namespace polyfem::solver::sdf_spline
{
	bool kernel_available() { return false; }

	void hessian_batch(const double *, const double *, double *, std::size_t)
	{
		throw std::runtime_error(
			"SDF spline kernel left out: built with FIT_KERNELS_PENDING; configure with "
			"-DFIT_KERNELS_PENDING=OFF (the default) to compile in kernels/fit/cpp/sdf_spline_hessian_emit.cpp");
	}
} // namespace polyfem::solver::sdf_spline

#else

// The emit carries its own copy of the prelude behind SLANG_CPP_PRELUDE_H;
// including the prelude first at file scope and emptying the EXTERN_C macros
// lets the emit sit in a namespace as ordinary C++ (guest/avbd/avbd_cpu.cpp).
#include "slang-cpp-prelude.h"
#undef SLANG_PRELUDE_EXTERN_C
#undef SLANG_PRELUDE_EXTERN_C_START
#undef SLANG_PRELUDE_EXTERN_C_END
#define SLANG_PRELUDE_EXTERN_C
#define SLANG_PRELUDE_EXTERN_C_START
#define SLANG_PRELUDE_EXTERN_C_END

namespace k_sdf_spline
{
#include "sdf_spline_hessian_emit.cpp"
} // namespace k_sdf_spline

namespace polyfem::solver::sdf_spline
{
	bool kernel_available() { return true; }

	void hessian_batch(const double *stencil, const double *uvw, double *out, const std::size_t n)
	{
		k_sdf_spline::SdfSplineParams_0 prm{};
		prm.count_0 = uint32_t(n);
		k_sdf_spline::GlobalParams_0 gp{};
		gp.stencil_0.data = const_cast<double *>(stencil);
		gp.stencil_0.count = kStencil * n;
		gp.uvw_0.data = const_cast<double *>(uvw);
		gp.uvw_0.count = 3 * n;
		gp.result_0.data = out;
		gp.result_0.count = kOut * n;
		gp.params_0 = &prm;
		for (std::size_t lane = 0; lane < n; ++lane)
		{
			ComputeThreadVaryingInput t{};
			t.groupID = uint3(0u, 0u, 0u);
			t.groupThreadID = uint3(uint32_t(lane), 0u, 0u);
			k_sdf_spline::main_0_Thread(&t, nullptr, &gp);
		}
	}
} // namespace polyfem::solver::sdf_spline

#endif
