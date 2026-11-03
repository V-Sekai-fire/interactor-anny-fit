#pragma once

// The tricubic B-spline sampler FitForm evaluates on the SDF grid (value,
// gradient, symmetric Hessian over the -1..+2 stencil around floor(p/h)), as a
// call site for a Lean-emitted kernel. Local adaptation for fit.elf (see
// CITATION.cff); it replaces openvdb::tools::SplineSampler::sampleHessian of
// the Huangzizhou/openvdb fork (Interpolation.h, trilinearInterpolationHessian).
//
// AGENTS.md rule 2: the kernel is lean/Fit/SdfSplineHessian.lean (double
// scalar, lean-slang emit-fp) -> Slang -> `slangc -target cpp` ->
// kernels/fit/cpp/sdf_spline_hessian_emit.cpp, included by SdfSpline.cpp.
// No hand-written sampler is linked here. FIT_KERNELS_PENDING (CMake option,
// default OFF) leaves the emit out and makes hessian_batch() throw; it is kept
// as the negative control of the build. Gate 6a holds the emit to 0 ULP
// against OpenVDB's sampleHessian and against a reference used only by the
// native test (gates/6-fit/sdf/spline_ref.h).
//
// Kernel contract (what the emitted GlobalParams_0 must carry), one thread
// per sample, dispatched n times through main_0_Thread with lane = sample:
//   StructuredBuffer<double>   stencil;  // 64 per sample: data[i][j][k], i-major
//                                        // (i*16 + j*4 + k), offsets -1..2 in x,y,z
//   StructuredBuffer<double>   uvw;      // 3 per sample: index-space fraction
//   RWStructuredBuffer<double> result;   // 10 per sample, index space:
//                                        // x, gx, gy, gz, hxx, hxy, hxz, hyy, hyz, hzz
//   ConstantBuffer<SdfSplineParams> params;  // count: samples (lanes >= count return)
// Arithmetic and summation order are the fork's: basis/deriv tables per axis,
// then for i, j, k ascending acc += ((data*a)*b)*c for each of the ten terms.
// (`out` is a Slang keyword, hence `result`.)

#include <Eigen/Core>

#include <cstddef>

namespace polyfem::solver
{
	struct SdfHess
	{
		double x = 0;
		Eigen::Vector3d g = Eigen::Vector3d::Zero();
		Eigen::Matrix3d h = Eigen::Matrix3d::Zero();
	};

	namespace sdf_spline
	{
		constexpr int kStencil = 64;
		constexpr int kOut = 10;

		/// False under FIT_KERNELS_PENDING (no emitted kernel compiled in).
		bool kernel_available();

		/// n samples: stencil[64 n], uvw[3 n] -> out[10 n] (layout above).
		/// Throws std::runtime_error under FIT_KERNELS_PENDING.
		void hessian_batch(const double *stencil, const double *uvw, double *out, std::size_t n);

		/// Unpack one 10-wide record, mirroring the upper triangle.
		inline SdfHess unpack(const double *o)
		{
			SdfHess s;
			s.x = o[0];
			s.g << o[1], o[2], o[3];
			s.h << o[4], o[5], o[6],
				o[5], o[7], o[8],
				o[6], o[8], o[9];
			return s;
		}
	} // namespace sdf_spline
} // namespace polyfem::solver
