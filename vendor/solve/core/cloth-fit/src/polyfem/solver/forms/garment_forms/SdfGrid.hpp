#pragma once

// A lazily filled signed-distance brick grid, the stand-in for the OpenVDB
// narrow band FitForm used to build (meshToSignedDistanceField(xform, points,
// tris, {}, 150, 1)). Local adaptation for fit.elf (see CITATION.cff).
//
// Voxel (i,j,k) sits at world (i*h, j*h, k*h), as under OpenVDB's linear
// transform of voxel size h. Its value is
//     clamp(sd, -1*h, +150*h),   sd = s * d
// where d is the exact unsigned distance (igl::AABB, point-triangle, double)
// to the avatar with its vertices rounded to float (OpenVDB takes Vec3s), and
// s = -1 inside (fast winding number w > 0.5), +1 outside. Values are a pure
// function of the voxel index: 8^3 bricks are filled whole on first touch, in
// any order, and nothing depends on which brick came first.
//
// No OpenVDB, no file I/O, no threads of its own. A mutex guards the brick map
// so a threaded native build of the same code stays correct.

#include <Eigen/Core>

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace igl
{
	template <typename DerivedV, int DIM>
	class AABB;
	struct FastWindingNumberBVH;
} // namespace igl

namespace polyfem::solver
{
	class SdfGrid
	{
	public:
		static constexpr int kBrickLog2 = 3;
		static constexpr int kBrick = 1 << kBrickLog2; // 8
		static constexpr int kBrickVoxels = kBrick * kBrick * kBrick;
		static constexpr double kExBand = 150.; // voxels, exterior
		static constexpr double kInBand = 1.;   // voxels, interior

		struct Stats
		{
			std::size_t bricks = 0;
			std::size_t bytes = 0;            // brick payload + map nodes (estimate)
			std::size_t distance_queries = 0; // igl::AABB::squared_distance calls
			std::size_t winding_queries = 0;  // igl::fast_winding_number calls
			double fill_seconds = 0;          // wall time spent filling bricks
		};

		/// Build the acceleration structures (AABB, winding-number BVH) over the
		/// float-rounded avatar. No voxel is computed here.
		SdfGrid(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F, double voxel_size);
		~SdfGrid();

		SdfGrid(const SdfGrid &) = delete;
		SdfGrid &operator=(const SdfGrid &) = delete;

		/// The grid for (V, F, voxel_size), shared across FitForm instances:
		/// run_retarget rebuilds FitForm every substep against the same avatar,
		/// so the bricks filled in one substep serve the next. One entry is
		/// kept; a different avatar or voxel size replaces it.
		static std::shared_ptr<const SdfGrid> cached(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F, double voxel_size);
		static void clear_cache();
		/// The cached grid, or null (for reporting its Stats after a solve).
		static std::shared_ptr<const SdfGrid> current();

		double voxel_size() const { return h_; }

		/// Value of voxel (i,j,k); fills its brick on first touch.
		double value(int i, int j, int k) const;

		/// The 4x4x4 stencil around base, data[a][b][c] = value(base + (a-1, b-1, c-1)),
		/// flattened a*16 + b*4 + c: SplineSampler::getValues' order.
		void stencil(const std::array<int, 3> &base, double data[64]) const;

		/// World point -> index coordinate p * (1/h) (OpenVDB's ScaleMap inverse),
		/// base = floor, uvw = ijk - base; then the stencil at base.
		void sample_point(const double p[3], double data[64], double uvw[3], std::array<int, 3> *base = nullptr) const;

		/// The value of voxel (i,j,k) computed afresh, without touching the map.
		double compute_voxel(int i, int j, int k) const;

		Stats stats() const;

		/// Hash of (float-rounded V, F, voxel size); the cache key.
		static std::uint64_t key(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F, double voxel_size);

	private:
		struct Brick
		{
			double v[kBrickVoxels];
		};

		const Brick &brick_locked(std::int64_t bi, std::int64_t bj, std::int64_t bk) const;
		void fill(Brick &b, std::int64_t bi, std::int64_t bj, std::int64_t bk) const;

		const double h_;
		const double inv_h_;
		Eigen::MatrixXd V_; // float-rounded, stored as double
		Eigen::MatrixXf Vf_;
		Eigen::MatrixXi F_;
		std::uint64_t key_;

		std::unique_ptr<igl::AABB<Eigen::MatrixXd, 3>> tree_;
		std::unique_ptr<igl::FastWindingNumberBVH> fwn_;

		mutable std::mutex mutex_;
		mutable std::unordered_map<std::uint64_t, std::unique_ptr<Brick>> bricks_;
		mutable std::size_t distance_queries_ = 0;
		mutable std::size_t winding_queries_ = 0;
		mutable double fill_seconds_ = 0;
	};
} // namespace polyfem::solver
