#include "SdfGrid.hpp"

#include <igl/AABB.h>
#include <igl/fast_winding_number.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

namespace polyfem::solver
{
	namespace
	{
		// Brick coordinates packed 21 bits each (+-2^20 bricks, +-8M voxels per axis).
		constexpr std::int64_t kBias = std::int64_t(1) << 20;

		inline std::uint64_t pack(std::int64_t bi, std::int64_t bj, std::int64_t bk)
		{
			return (std::uint64_t(bi + kBias) << 42) | (std::uint64_t(bj + kBias) << 21) | std::uint64_t(bk + kBias);
		}

		// Floor division by the brick size for negative indices too.
		inline std::int64_t brick_of(std::int64_t i) { return i >> SdfGrid::kBrickLog2; }
		inline int local_of(std::int64_t i) { return int(i & (SdfGrid::kBrick - 1)); }

		inline std::uint64_t fnv1a(std::uint64_t h, const void *data, std::size_t n)
		{
			const unsigned char *p = static_cast<const unsigned char *>(data);
			for (std::size_t i = 0; i < n; i++)
			{
				h ^= p[i];
				h *= 1099511628211ull;
			}
			return h;
		}

		// OpenVDB's FitForm fed the avatar as Vec3s: every vertex rounded to float.
		Eigen::MatrixXd round_to_float(const Eigen::MatrixXd &V)
		{
			return V.cast<float>().cast<double>();
		}

		std::mutex g_cache_mutex;
		std::shared_ptr<const SdfGrid> g_cache;
		std::uint64_t g_cache_key = 0;
	} // namespace

	SdfGrid::SdfGrid(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F, const double voxel_size)
		: h_(voxel_size), inv_h_(1.0 / voxel_size), V_(round_to_float(V)), Vf_(V.cast<float>()), F_(F),
		  key_(key(V, F, voxel_size)), tree_(std::make_unique<igl::AABB<Eigen::MatrixXd, 3>>()),
		  fwn_(std::make_unique<igl::FastWindingNumberBVH>())
	{
		tree_->init(V_, F_);
		igl::fast_winding_number(Vf_, F_, 2, *fwn_);
	}

	SdfGrid::~SdfGrid() = default;

	std::uint64_t SdfGrid::key(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F, const double voxel_size)
	{
		const Eigen::MatrixXd Vr = round_to_float(V);
		std::uint64_t h = 1469598103934665603ull;
		const std::int64_t dims[4] = {Vr.rows(), Vr.cols(), F.rows(), F.cols()};
		h = fnv1a(h, dims, sizeof(dims));
		h = fnv1a(h, Vr.data(), sizeof(double) * Vr.size());
		h = fnv1a(h, F.data(), sizeof(int) * F.size());
		h = fnv1a(h, &voxel_size, sizeof(double));
		return h;
	}

	std::shared_ptr<const SdfGrid> SdfGrid::cached(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F, const double voxel_size)
	{
		const std::uint64_t k = key(V, F, voxel_size);
		std::lock_guard<std::mutex> lock(g_cache_mutex);
		// The hash picks the entry; an exact compare guards against a collision.
		if (g_cache && g_cache_key == k && g_cache->h_ == voxel_size
			&& g_cache->F_.rows() == F.rows() && g_cache->F_.cols() == F.cols() && g_cache->F_ == F
			&& g_cache->V_.rows() == V.rows() && g_cache->V_.cols() == V.cols() && g_cache->V_ == round_to_float(V))
			return g_cache;
		g_cache = std::make_shared<const SdfGrid>(V, F, voxel_size);
		g_cache_key = k;
		return g_cache;
	}

	void SdfGrid::clear_cache()
	{
		std::lock_guard<std::mutex> lock(g_cache_mutex);
		g_cache.reset();
		g_cache_key = 0;
	}

	std::shared_ptr<const SdfGrid> SdfGrid::current()
	{
		std::lock_guard<std::mutex> lock(g_cache_mutex);
		return g_cache;
	}

	double SdfGrid::compute_voxel(const int i, const int j, const int k) const
	{
		const Eigen::RowVector3d p(double(i) * h_, double(j) * h_, double(k) * h_);
		const Eigen::RowVector3f pf = p.cast<float>();
		const bool inside = igl::fast_winding_number(*fwn_, 2.f, pf) > 0.5f;

		// Search only as far as the band: past it the value is the clamp, and
		// AABB::squared_distance returns up_sqr_d when nothing is closer, so the
		// result is still clamp(exact distance).
		const double band = (inside ? kInBand : kExBand) * h_;
		const double up = band * band;
		int fi = -1;
		Eigen::RowVector3d c;
		const double sq = tree_->squared_distance(V_, F_, p, 0.0, up, fi, c);
		const double d = (sq >= up) ? band : std::min(std::sqrt(sq), band);
		return inside ? -d : d;
	}

	void SdfGrid::fill(Brick &b, const std::int64_t bi, const std::int64_t bj, const std::int64_t bk) const
	{
		const auto t0 = std::chrono::steady_clock::now();
		const int i0 = int(bi * kBrick), j0 = int(bj * kBrick), k0 = int(bk * kBrick);
		for (int a = 0; a < kBrick; a++)
			for (int c = 0; c < kBrick; c++)
				for (int e = 0; e < kBrick; e++)
					b.v[(a * kBrick + c) * kBrick + e] = compute_voxel(i0 + a, j0 + c, k0 + e);
		distance_queries_ += kBrickVoxels;
		winding_queries_ += kBrickVoxels;
		fill_seconds_ += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
	}

	const SdfGrid::Brick &SdfGrid::brick_locked(const std::int64_t bi, const std::int64_t bj, const std::int64_t bk) const
	{
		auto &slot = bricks_[pack(bi, bj, bk)];
		if (!slot)
		{
			auto b = std::make_unique<Brick>();
			fill(*b, bi, bj, bk);
			slot = std::move(b);
		}
		return *slot;
	}

	double SdfGrid::value(const int i, const int j, const int k) const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		const Brick &b = brick_locked(brick_of(i), brick_of(j), brick_of(k));
		return b.v[(local_of(i) * kBrick + local_of(j)) * kBrick + local_of(k)];
	}

	void SdfGrid::stencil(const std::array<int, 3> &base, double data[64]) const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		// The 4-wide stencil touches at most two bricks per axis.
		const std::int64_t lo[3] = {brick_of(base[0] - 1), brick_of(base[1] - 1), brick_of(base[2] - 1)};
		const Brick *bp[2][2][2] = {};
		for (int a = 0; a < 4; a++)
		{
			const std::int64_t i = std::int64_t(base[0]) + a - 1;
			const int ba = int(brick_of(i) - lo[0]);
			for (int b = 0; b < 4; b++)
			{
				const std::int64_t j = std::int64_t(base[1]) + b - 1;
				const int bb = int(brick_of(j) - lo[1]);
				for (int c = 0; c < 4; c++)
				{
					const std::int64_t k = std::int64_t(base[2]) + c - 1;
					const int bc = int(brick_of(k) - lo[2]);
					const Brick *&br = bp[ba][bb][bc];
					if (!br)
						br = &brick_locked(lo[0] + ba, lo[1] + bb, lo[2] + bc);
					data[a * 16 + b * 4 + c] = br->v[(local_of(i) * kBrick + local_of(j)) * kBrick + local_of(k)];
				}
			}
		}
	}

	void SdfGrid::sample_point(const double p[3], double data[64], double uvw[3], std::array<int, 3> *base_out) const
	{
		std::array<int, 3> base;
		for (int d = 0; d < 3; d++)
		{
			// ScaleMap::applyInverseMap multiplies by the stored reciprocal;
			// SplineSampler floors with int(std::floor(.)) and subtracts in double.
			const double x = p[d] * inv_h_;
			base[d] = int(std::floor(x));
			uvw[d] = x - double(base[d]);
		}
		stencil(base, data);
		if (base_out)
			*base_out = base;
	}

	SdfGrid::Stats SdfGrid::stats() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		Stats s;
		s.bricks = bricks_.size();
		// Payload plus one map node (key, pointer, next) and one bucket per brick.
		s.bytes = s.bricks * (sizeof(Brick) + sizeof(std::uint64_t) + 2 * sizeof(void *)) + bricks_.bucket_count() * sizeof(void *);
		s.distance_queries = distance_queries_;
		s.winding_queries = winding_queries_;
		s.fill_seconds = fill_seconds_;
		return s;
	}
} // namespace polyfem::solver
