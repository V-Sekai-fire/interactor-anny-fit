#include "FitForm.hpp"

#include <polyfem/utils/MaybeParallelFor.hpp>
#include <polyfem/utils/Timer.hpp>
#include <polyfem/utils/Logger.hpp>

#include <ipc/utils/eigen_ext.hpp>

#include <igl/Timer.h>

#include <algorithm>
#include <array>
#include <vector>

using namespace polyfem::utils;

namespace {
    class LocalThreadMatStorage
    {
    public:
        Eigen::MatrixXd mat;
        // Eigen::MatrixXd samples;

        LocalThreadMatStorage(const int row, const int col)
        {
            mat.resize(row, col);
            mat.setZero();
        }
    };

    class LocalThreadSparseMatStorage
    {
    public:
        std::unique_ptr<MatrixCache> cache = nullptr;
        // Eigen::MatrixXd samples;

        LocalThreadSparseMatStorage() = delete;

        LocalThreadSparseMatStorage(const int buffer_size, const int rows, const int cols)
        {
            init(buffer_size, rows, cols);
        }

        LocalThreadSparseMatStorage(const int buffer_size, const MatrixCache &c)
        {
            init(buffer_size, c);
        }

        LocalThreadSparseMatStorage(const LocalThreadSparseMatStorage &other)
            : cache(other.cache->copy())
        {
        }

        LocalThreadSparseMatStorage &operator=(const LocalThreadSparseMatStorage &other)
        {
            assert(other.cache != nullptr);
            cache = other.cache->copy();
            return *this;
        }

        void init(const int buffer_size, const int rows, const int cols)
        {
            // assert(rows == cols);
            // cache = std::make_unique<DenseMatrixCache>();
            cache = std::make_unique<SparseMatrixCache>();
            cache->reserve(buffer_size);
            cache->init(rows, cols);
        }

        void init(const int buffer_size, const MatrixCache &c)
        {
            if (cache == nullptr)
                cache = c.copy();
            cache->reserve(buffer_size);
            cache->init(c);
        }
    };

    template <int N>
    constexpr Eigen::Matrix<double, ((N+1)*(N+2))/2, 4> upsample_standard()
    {
        constexpr int num = ((N+1)*(N+2))/2;

        Eigen::Matrix<double, num, 4> out;
        for (int i = 0, k = 0; i <= N; i++)
            for (int j = 0; i + j <= N; j++, k++)
            {
                std::array<int, 3> arr = {{i, j, N - i - j}};
                std::sort(arr.begin(), arr.end());

                double w = 6;
                if (arr[1] == 0)        // vertex node
                    w = 1;
                else if (arr[0] == 0)   // edge node
                    w = 3;
                else                    // face node
                    w = 6;

                out.row(k) << i, j, N - i - j, w;
            }

        out.template leftCols<3>() /= N;
        out.col(3) /= N * N;
        return out;
    }
}

namespace polyfem::solver
{
    template <int n_refs>
    FitForm<n_refs>::FitForm(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F, const Eigen::MatrixXd &surface_v, const Eigen::MatrixXi &surface_f, const double voxel_size, const std::vector<int> &not_fit_faces, const std::string &) : V_(V), F_(F), voxel_size_(voxel_size), totalP(std::vector<SdfHess>(F_.rows() * n_loc_samples))
    {
        // The avatar's SDF: a lazily filled brick grid in place of
        // meshToSignedDistanceField(xform, Vec3s points, tris, {}, 150, 1).
        // Shared across the FitForm rebuilt every substep on the same avatar.
        grid = SdfGrid::cached(surface_v, surface_f, voxel_size);

        // build upsampling scheme on the garment mesh
        {
            Eigen::Matrix<double, n_loc_samples, 4> tmp = upsample_standard<n_refs>();
            P = tmp.template leftCols<3>();
            weights = tmp.col(3);
        }

        {
            for (int i = 0; i < F.rows(); i++)
            {
                if (std::find(not_fit_faces.begin(), not_fit_faces.end(), i) == not_fit_faces.end())
                    fit_faces_ids.push_back(i);
            }
        }

        initial_distance.setZero(F_.rows() * n_loc_samples);
    }

    template <int n_refs>
    double FitForm<n_refs>::value_unweighted(const Eigen::VectorXd &x) const
    {
        const Eigen::MatrixXd V = unflatten(x, 3) + V_;

        double val = 0;
        for (int f : fit_faces_ids) {
            const double area = (V_.row(F_(f, 1)) - V_.row(F_(f, 0))).template head<3>().cross((V_.row(F_(f, 2)) - V_.row(F_(f, 0))).template head<3>()).norm() / 2;
            for (int i = 0; i < P.rows(); i++) {
                const double tmp = totalP[f * n_loc_samples + i].x;

                if (std::isnan(tmp))
                    log_and_throw_error("Invalid sdf values!");

                if (tmp > initial_distance(f * n_loc_samples + i))
                    val += area * weights(i) * pow(tmp - initial_distance(f * n_loc_samples + i), power);
            }
        }

        return val;
    }

    template <int n_refs>
    void FitForm<n_refs>::first_derivative_unweighted(const Eigen::VectorXd &x, Eigen::VectorXd &gradv) const
    {
        POLYFEM_SCOPED_TIMER("fit gradient");
        const Eigen::MatrixXd V = unflatten(x, 3) + V_;

        Eigen::MatrixXd g = Eigen::MatrixXd::Zero(V.rows(), V.cols());

        auto storage = create_thread_storage(LocalThreadMatStorage(g.rows(), g.cols()));

        maybe_parallel_for(fit_faces_ids.size(), [&](int start, int end, int thread_id) {
            LocalThreadMatStorage &local_storage = get_local_thread_storage(storage, thread_id);
            for (int f_aux = start; f_aux < end; f_aux++) {
                int f = fit_faces_ids[f_aux];
                const double area = (V_.row(F_(f, 1)) - V_.row(F_(f, 0))).template head<3>().cross((V_.row(F_(f, 2)) - V_.row(F_(f, 0))).template head<3>()).norm() / 2;
                for (int i = 0; i < P.rows(); i++) {
                    const auto &tmp = totalP[f * n_loc_samples + i];

                    if (tmp.x > initial_distance(f * n_loc_samples + i))
                        for (int d = 0; d < 3; d++)
                            local_storage.mat(F_.row(f), d) += (pow(tmp.x - initial_distance(f * n_loc_samples + i), power-1) * power * area * weights(i) * tmp.g(d)) * P.row(i);
                }
            }
        });

		// Serially merge local storages
		for (const LocalThreadMatStorage &local_storage : storage)
			g += local_storage.mat;

        gradv = flatten(g);
    }

    template <int n_refs>
    void FitForm<n_refs>::solution_changed(const Eigen::VectorXd &new_x)
    {
        POLYFEM_SCOPED_TIMER("sample SDF");

        const Eigen::MatrixXd V = unflatten(new_x, 3) + V_;

        // Gather each sample's 4^3 stencil from the brick grid, then evaluate
        // the spline kernel over the batch (SdfSpline.hpp). Chunks bound the
        // stencil buffer: 1024 samples = 512 KiB.
        constexpr int chunk_faces = 1024 / n_loc_samples;
        maybe_parallel_for((fit_faces_ids.size() + chunk_faces - 1) / chunk_faces, [&](int start, int end, int thread_id) {
            std::vector<double> stencil(sdf_spline::kStencil * chunk_faces * n_loc_samples);
            std::vector<double> uvw(3 * chunk_faces * n_loc_samples);
            std::vector<double> out(sdf_spline::kOut * chunk_faces * n_loc_samples);
            for (int c = start; c < end; c++) {
                const int f_begin = c * chunk_faces;
                const int f_end = std::min<int>(f_begin + chunk_faces, fit_faces_ids.size());
                std::size_t n = 0;
                for (int f_aux = f_begin; f_aux < f_end; f_aux++) {
                    int f = fit_faces_ids[f_aux];
                    const Eigen::Matrix3d M = V({F_(f, 0),F_(f, 1),F_(f, 2)}, Eigen::all);
                    Eigen::Matrix<double, n_loc_samples, 3> samples = P * M;
                    for (int i = 0; i < P.rows(); i++, n++) {
                        const double p[3] = {samples(i, 0), samples(i, 1), samples(i, 2)};
                        grid->sample_point(p, &stencil[sdf_spline::kStencil * n], &uvw[3 * n]);
                    }
                }
                sdf_spline::hessian_batch(stencil.data(), uvw.data(), out.data(), n);
                n = 0;
                for (int f_aux = f_begin; f_aux < f_end; f_aux++) {
                    int f = fit_faces_ids[f_aux];
                    for (int i = 0; i < P.rows(); i++, n++) {
                        auto &tmp = totalP[f * n_loc_samples + i];
                        tmp = sdf_spline::unpack(&out[sdf_spline::kOut * n]);
                        tmp.g = tmp.g / voxel_size_;
                        tmp.h = tmp.h * (1. / voxel_size_ / voxel_size_);
                    }
                }
            }
        });
    }

    template <int n_refs>
    void FitForm<n_refs>::second_derivative_unweighted(const Eigen::VectorXd &x, StiffnessMatrix &hessian) const
    {
        POLYFEM_SCOPED_TIMER("fit hessian");
        hessian.setZero();
        hessian.resize(x.size(), x.size());
        if (!use_spline)
            return;

        const Eigen::MatrixXd V = unflatten(x, 3) + V_;

        // One 9x9 block per fit face.
        auto storage = create_thread_storage(LocalThreadSparseMatStorage(long(fit_faces_ids.size()) * 81, hessian.rows(), hessian.cols()));

        igl::Timer timer;
        timer.start();
        maybe_parallel_for(fit_faces_ids.size(), [&](int start, int end, int thread_id) {
            LocalThreadSparseMatStorage &local_storage = get_local_thread_storage(storage, thread_id);
            Eigen::Matrix<double, 9, 9> local_hess;
            for (int f_aux = start; f_aux < end; f_aux++) {
                int f = fit_faces_ids[f_aux];
                const double area = (V_.row(F_(f, 1)) - V_.row(F_(f, 0))).template head<3>().cross((V_.row(F_(f, 2)) - V_.row(F_(f, 0))).template head<3>()).norm() / 2;
                local_hess.setZero();
                for (int i = 0; i < P.rows(); i++) {
                    const auto &tmp = totalP[f * n_loc_samples + i];

                    if (tmp.x <= initial_distance(f * n_loc_samples + i))
                        continue;

                    Eigen::Vector3d g;
                    g << tmp.g(0), tmp.g(1), tmp.g(2);
                    Eigen::Matrix3d h;
                    h << tmp.h(0, 0), tmp.h(0, 1), tmp.h(0, 2),
                        tmp.h(1, 0), tmp.h(1, 1), tmp.h(1, 2),
                        tmp.h(2, 0), tmp.h(2, 1), tmp.h(2, 2);
                    h *= pow(tmp.x - initial_distance(f * n_loc_samples + i), power-1) * power;
                    h += g * g.transpose() * (pow(tmp.x - initial_distance(f * n_loc_samples + i), power-2) * power * (power-1));
                    h *= area * weights(i);

                    for (int d = 0; d < 3; d++)
                        for (int k = 0; k < 3; k++)
                            for (int s = 0; s < 3; s++)
                                for (int l = 0; l < 3; l++)
                                    local_hess(s * 3 + d, l * 3 + k) += P(i, s) * P(i, l) * h(d, k);
                }
                for (int d = 0; d < 3; d++)
                    for (int k = 0; k < 3; k++)
                        for (int s = 0; s < 3; s++)
                            for (int l = 0; l < 3; l++)
                                local_storage.cache->add_value(f, F_(f, s) * 3 + d, F_(f, l) * 3 + k, local_hess(s * 3 + d, l * 3 + k));
            }
        });

        timer.stop();
        logger().trace("done separate assembly {}s...", timer.getElapsedTime());

        // Assemble the hessian matrix by concatenating the tuples in each local storage

        // Collect thread storages
        std::vector<LocalThreadSparseMatStorage *> storages(storage.size());
        long int index = 0;
        for (auto &local_storage : storage)
        {
            storages[index++] = &local_storage;
        }

        timer.start();
        maybe_parallel_for(storages.size(), [&](int i) {
            storages[i]->cache->prune();
        });
        timer.stop();
        logger().trace("done pruning triplets {}s...", timer.getElapsedTime());

        // Prepares for parallel concatenation
        std::vector<long int> offsets(storage.size());

        index = 0;
        long int triplet_count = 0;
        for (auto &local_storage : storage)
        {
            offsets[index++] = triplet_count;
            triplet_count += local_storage.cache->triplet_count();
        }

        std::vector<Eigen::Triplet<double>> triplets;

        assert(storages.size() >= 1);
        if (storages[0]->cache->is_dense())
        {
            timer.start();
            // Serially merge local storages
            Eigen::MatrixXd tmp(hessian);
            for (const LocalThreadSparseMatStorage &local_storage : storage)
                tmp += dynamic_cast<const DenseMatrixCache &>(*local_storage.cache).mat();
            hessian = tmp.sparseView();
            hessian.makeCompressed();
            timer.stop();

            logger().trace("Serial assembly time: {}s...", timer.getElapsedTime());
        }
        else if (triplet_count >= triplets.max_size())
        {
            // Serial fallback version in case the vector of triplets cannot be allocated

            logger().error("Cannot allocate space for triplets, switching to serial assembly.");

            timer.start();
            // Serially merge local storages
            for (LocalThreadSparseMatStorage &local_storage : storage)
                hessian += local_storage.cache->get_matrix(false); // will also prune
            hessian.makeCompressed();
            timer.stop();

            logger().trace("Serial assembly time: {}s...", timer.getElapsedTime());
        }
        else
        {
            timer.start();
            triplets.resize(triplet_count);
            timer.stop();

            logger().trace("done allocate triplets {}s...", timer.getElapsedTime());
            logger().trace("Triplets Count: {}", triplet_count);

            timer.start();
            // Parallel copy into triplets
            maybe_parallel_for(storages.size(), [&](int i) {
                const SparseMatrixCache &cache = dynamic_cast<const SparseMatrixCache &>(*storages[i]->cache);
                long int offset = offsets[i];

                std::copy(cache.entries().begin(), cache.entries().end(), triplets.begin() + offset);
                offset += cache.entries().size();

                if (cache.mat().nonZeros() > 0)
                {
                    long int count = 0;
                    for (int k = 0; k < cache.mat().outerSize(); ++k)
                    {
                        for (Eigen::SparseMatrix<double>::InnerIterator it(cache.mat(), k); it; ++it)
                        {
                            assert(count < cache.mat().nonZeros());
                            triplets[offset + count++] = Eigen::Triplet<double>(it.row(), it.col(), it.value());
                        }
                    }
                }
            });

            timer.stop();
            logger().trace("done concatenate triplets {}s...", timer.getElapsedTime());

            timer.start();
            // Sort and assemble
            hessian.setFromTriplets(triplets.begin(), triplets.end());
            timer.stop();

            logger().trace("done setFromTriplets assembly {}s...", timer.getElapsedTime());
        }
    }

    template class FitForm<4>;
}
