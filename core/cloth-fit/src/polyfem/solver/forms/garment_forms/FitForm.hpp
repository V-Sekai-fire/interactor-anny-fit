#pragma once

#include <polyfem/solver/forms/Form.hpp>

#include <polyfem/Common.hpp>
#include <polyfem/utils/Types.hpp>
#include <polyfem/utils/MatrixUtils.hpp>
#include <polyfem/utils/MatrixCache.hpp>

#include "SdfGrid.hpp"
#include "SdfSpline.hpp"

namespace polyfem::solver
{
	template <int n_refs>
	class FitForm : public Form
	{
	public:
		constexpr static int n_loc_samples = ((n_refs+1)*(n_refs+2))/2;

		FitForm(
			const Eigen::MatrixXd &V,
			const Eigen::MatrixXi &F,
			const Eigen::MatrixXd &surface_v,
			const Eigen::MatrixXi &surface_f,
			const double voxel_size,
			const std::vector<int> &not_fit_faces,
			const std::string &out_dir = ""); // unused: the debug OBJ writes it fed are gone

		std::string name() const override { return "garment-fit"; }

		/// @brief Compute the value of the form
		/// @param x Current solution
		/// @return Computed value
		double value_unweighted(const Eigen::VectorXd &x) const override;

		/// @brief Compute the first derivative of the value wrt x
		/// @param[in] x Current solution
		/// @param[out] gradv Output gradient of the value wrt x
		void first_derivative_unweighted(const Eigen::VectorXd &x, Eigen::VectorXd &gradv) const override;

		/// @brief Compute the second derivative of the value wrt x
		/// @param[in] x Current solution
		/// @param[out] hessian Output Hessian of the value wrt x
		void second_derivative_unweighted(const Eigen::VectorXd &x, StiffnessMatrix &hessian) const override;

		void solution_changed(const Eigen::VectorXd &new_x) override;

		/// The SDF this form samples (shared with every FitForm on the same avatar).
		const std::shared_ptr<const SdfGrid> &sdf() const { return grid; }

    private:
        const Eigen::MatrixXd V_;
		const Eigen::MatrixXi F_;
        const double voxel_size_;
        const bool use_spline = true;

		Eigen::Matrix<double, n_loc_samples, 3> P;
		Eigen::Vector<double, n_loc_samples> weights;

		std::vector<SdfHess> totalP;

        std::shared_ptr<const SdfGrid> grid;

		mutable std::unique_ptr<utils::MatrixCache> mat_cache_;

		const int power = 2;

		Eigen::VectorXd initial_distance;

		std::vector<int> fit_faces_ids;
	};
} // namespace polyfem::solver
