#pragma once

#include <polyfem/solver/forms/Form.hpp>

#include <polyfem/Common.hpp>
#include <polyfem/utils/Types.hpp>
#include <polyfem/utils/MatrixUtils.hpp>

namespace polyfem::solver
{
	class AreaForm : public Form
	{
	public:
		AreaForm(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F, const double threshold = 1e-5) : V_(V), F_(F), threshold_(threshold) {}
		virtual ~AreaForm() = default;

		std::string name() const override { return "area"; }

	protected:
		/// @brief Compute the potential value
		/// @param x Current solution
		/// @return Value of the contact barrier potential
		double value_unweighted(const Eigen::VectorXd &x) const override;

		/// @brief Compute the first derivative of the value wrt x
		/// @param[in] x Current solution
		/// @param[out] gradv Output gradient of the value wrt x
		void first_derivative_unweighted(const Eigen::VectorXd &x, Eigen::VectorXd &gradv) const override;

		/// @brief Compute the second derivative of the value wrt x
		/// @param x Current solution
		/// @param hessian Output Hessian of the value wrt x
		void second_derivative_unweighted(const Eigen::VectorXd &x, StiffnessMatrix &hessian) const override;

	private:
		const Eigen::MatrixXd V_;
		const Eigen::MatrixXi F_;
		const double threshold_;
	};

	class DefGradForm : public Form
	{
	public:
		DefGradForm(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F) : V_(V), F_(F) {}
		virtual ~DefGradForm() = default;

		std::string name() const override { return "deformation"; }

	protected:
		/// @brief Compute the potential value
		/// @param x Current solution
		/// @return Value of the contact barrier potential
		double value_unweighted(const Eigen::VectorXd &x) const override;

		/// @brief Compute the first derivative of the value wrt x
		/// @param[in] x Current solution
		/// @param[out] gradv Output gradient of the value wrt x
		void first_derivative_unweighted(const Eigen::VectorXd &x, Eigen::VectorXd &gradv) const override;

		/// @brief Compute the second derivative of the value wrt x
		/// @param x Current solution
		/// @param hessian Output Hessian of the value wrt x
		void second_derivative_unweighted(const Eigen::VectorXd &x, StiffnessMatrix &hessian) const override;

	private:
		const Eigen::MatrixXd V_;
		const Eigen::MatrixXi F_;
	};

	class AngleForm : public Form
	{
	public:
		AngleForm(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F);
		virtual ~AngleForm() = default;

		std::string name() const override { return "angle"; }

	protected:
		/// @brief Compute the potential value
		/// @param x Current solution
		/// @return Value of the contact barrier potential
		double value_unweighted(const Eigen::VectorXd &x) const override;

		/// @brief Compute the first derivative of the value wrt x
		/// @param[in] x Current solution
		/// @param[out] gradv Output gradient of the value wrt x
		void first_derivative_unweighted(const Eigen::VectorXd &x, Eigen::VectorXd &gradv) const override;

		/// @brief Compute the second derivative of the value wrt x
		/// @param x Current solution
		/// @param hessian Output Hessian of the value wrt x
		void second_derivative_unweighted(const Eigen::VectorXd &x, StiffnessMatrix &hessian) const override;

	private:
		const Eigen::MatrixXd V_;
		const Eigen::MatrixXi F_;
		Eigen::MatrixXi TT, TTi;
		Eigen::VectorXd areas;
		Eigen::MatrixXd orig_angles;
	};


	class RelativeScalingForm : public Form
	{
	public:
		RelativeScalingForm(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F);
		virtual ~RelativeScalingForm() = default;

		std::string name() const override { return "relative-scaling"; }

	protected:
		/// @brief Compute the potential value
		/// @param x Current solution
		/// @return Value of the contact barrier potential
		double value_unweighted(const Eigen::VectorXd &x) const override;

		/// @brief Compute the first derivative of the value wrt x
		/// @param[in] x Current solution
		/// @param[out] gradv Output gradient of the value wrt x
		void first_derivative_unweighted(const Eigen::VectorXd &x, Eigen::VectorXd &gradv) const override;

		/// @brief Compute the second derivative of the value wrt x
		/// @param x Current solution
		/// @param hessian Output Hessian of the value wrt x
		void second_derivative_unweighted(const Eigen::VectorXd &x, StiffnessMatrix &hessian) const override;

	private:

		const Eigen::MatrixXd V_;
		const Eigen::MatrixXi F_;
		Eigen::MatrixXi TT, TTi;
		Eigen::VectorXd orig_areas;
		Eigen::MatrixXd orig_dists;
	};


	class SimilarityForm : public Form
	{
	public:
		SimilarityForm(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F);
		virtual ~SimilarityForm() = default;

		std::string name() const override { return "similarity"; }

	protected:
		/// @brief Compute the potential value
		/// @param x Current solution
		/// @return Value of the contact barrier potential
		double value_unweighted(const Eigen::VectorXd &x) const override;

		/// @brief Compute the first derivative of the value wrt x
		/// @param[in] x Current solution
		/// @param[out] gradv Output gradient of the value wrt x
		void first_derivative_unweighted(const Eigen::VectorXd &x, Eigen::VectorXd &gradv) const override;

		/// @brief Compute the second derivative of the value wrt x
		/// @param x Current solution
		/// @param hessian Output Hessian of the value wrt x
		void second_derivative_unweighted(const Eigen::VectorXd &x, StiffnessMatrix &hessian) const override;

	private:

		const Eigen::MatrixXd V_;
		const Eigen::MatrixXi F_;
		Eigen::MatrixXi TT, TTi;
		Eigen::VectorXd orig_areas;
		Eigen::MatrixXd orig_coeffs;
	};

	class NormalForm : public Form
	{
	public:
		NormalForm(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F);
		virtual ~NormalForm() = default;

		std::string name() const override { return "normal"; }

	protected:
		/// @brief Compute the potential value
		/// @param x Current solution
		/// @return Value of the contact barrier potential
		double value_unweighted(const Eigen::VectorXd &x) const override;

		/// @brief Compute the first derivative of the value wrt x
		/// @param[in] x Current solution
		/// @param[out] gradv Output gradient of the value wrt x
		void first_derivative_unweighted(const Eigen::VectorXd &x, Eigen::VectorXd &gradv) const override;

		/// @brief Compute the second derivative of the value wrt x
		/// @param x Current solution
		/// @param hessian Output Hessian of the value wrt x
		void second_derivative_unweighted(const Eigen::VectorXd &x, StiffnessMatrix &hessian) const override;

	private:
		const Eigen::MatrixXd V_;
		const Eigen::MatrixXi F_;

		Eigen::VectorXd orig_areas;
	};

	// class GlobalPositionalForm : public Form
	// {
	// public:
	// 	GlobalPositionalForm(
	// 		const Eigen::MatrixXd &V,
	// 		const Eigen::MatrixXi &F,
	// 		const Eigen::MatrixXd &source_skeleton_v,
	// 		const Eigen::MatrixXd &target_skeleton_v,
	// 		const Eigen::MatrixXi &skeleton_edges,
    //     	const Eigen::MatrixXd &skin_weights);
	// 	virtual ~GlobalPositionalForm() = default;

	// 	std::string name() const override { return "global-relative-position"; }

	// protected:
	// 	/// @brief Compute the potential value
	// 	/// @param x Current solution
	// 	/// @return Value of the contact barrier potential
	// 	double value_unweighted(const Eigen::VectorXd &x) const override;

	// 	/// @brief Compute the first derivative of the value wrt x
	// 	/// @param[in] x Current solution
	// 	/// @param[out] gradv Output gradient of the value wrt x
	// 	void first_derivative_unweighted(const Eigen::VectorXd &x, Eigen::VectorXd &gradv) const override;

	// 	/// @brief Compute the second derivative of the value wrt x
	// 	/// @param x Current solution
	// 	/// @param hessian Output Hessian of the value wrt x
	// 	void second_derivative_unweighted(const Eigen::VectorXd &x, StiffnessMatrix &hessian) const override;

	// private:
	// 	const Eigen::MatrixXd V_;

	// 	const Eigen::MatrixXd source_skeleton_v_;
	// 	const Eigen::MatrixXd target_skeleton_v_;
	// 	const Eigen::MatrixXi skeleton_edges_;
	// 	const Eigen::MatrixXd skin_weights_;

	// 	Eigen::VectorXi bones;
	// 	Eigen::VectorXd relative_positions;
	// };
} // namespace polyfem::solver
