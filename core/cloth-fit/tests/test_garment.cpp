////////////////////////////////////////////////////////////////////////////////
// M_PI came in through the OpenVDB headers FitForm.hpp no longer includes.
#define _USE_MATH_DEFINES
#include <polyfem/solver/forms/garment_forms/GarmentForm.hpp>
#include <polyfem/solver/forms/garment_forms/CurveConstraintForm.hpp>
#include <polyfem/solver/forms/garment_forms/CurveCenterTargetForm.hpp>
#include <polyfem/solver/forms/garment_forms/FitForm.hpp>
#include <polyfem/io/MatrixIO.hpp>
#include <polyfem/mesh/MeshUtils.hpp>
#include <polyfem/utils/par_for.hpp>

#include <finitediff.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <igl/read_triangle_mesh.h>
#include <igl/remove_duplicate_vertices.h>

#include <iostream>
#include <memory>
#include <math.h>
////////////////////////////////////////////////////////////////////////////////

using namespace polyfem;
using namespace polyfem::solver;


TEST_CASE("Garment forms invariance", "[form][form_derivatives][garment]")
{
	Eigen::Matrix3d R;
	{
		Eigen::AngleAxisd rollAngle(0.31*M_PI, Eigen::Vector3d::UnitZ());
		Eigen::AngleAxisd yawAngle(0.28*M_PI, Eigen::Vector3d::UnitY());
		Eigen::AngleAxisd pitchAngle(0.13*M_PI, Eigen::Vector3d::UnitX());
		Eigen::Quaternion<double> q = rollAngle * yawAngle * pitchAngle;
		R = q.matrix();
	}

	const int dim = 3;

	Eigen::MatrixXd V;
	Eigen::MatrixXi F;
    igl::read_triangle_mesh(POLYFEM_SOURCE_DIR + std::string("/tests/garment.obj"), V, F);

	auto curves = boundary_curves(F);

	std::vector<std::unique_ptr<Form>> forms;
    forms.push_back(std::make_unique<CurveCurvatureForm>(V, curves));
	forms.push_back(std::make_unique<AngleForm>(V, F));
	// forms.push_back(std::make_unique<SimilarityForm>(V, F));
	forms.push_back(std::make_unique<CurveTorsionForm>(V, curves));

	Eigen::VectorXd x = utils::flatten(1.2 * V * R - V);

	for (auto &form : forms)
	{
		form->init(x);
		form->init_lagging(x);

        Eigen::VectorXd grad;
        form->first_derivative(x, grad);

		// std::cout << form->name() << " " << form->value(x) << " " << grad.norm() << "\n";
        REQUIRE(form->value(x) < 1e-10);
		REQUIRE(grad.norm() < 1e-6);
    }
}

TEST_CASE("Garment full forms derivatives", "[form][form_derivatives][garment]")
{
	const int dim = 3;

	utils::NThread::get().set_num_threads(16);

	Eigen::MatrixXd V;
	Eigen::MatrixXi F;
    igl::read_triangle_mesh(POLYFEM_SOURCE_DIR + std::string("/tests/garment.obj"), V, F);

	auto curves = boundary_curves(F);

	static const int n_rand = 5;
	const double tol = 1e-5;

	Eigen::MatrixXd target_skeleton_v, source_skeleton_v;
	Eigen::MatrixXi skeleton_edges;
	mesh::read_edge_mesh(POLYFEM_SOURCE_DIR + std::string("/tests/skeleton.obj"), source_skeleton_v, skeleton_edges);
	target_skeleton_v = source_skeleton_v + Eigen::MatrixXd::Random(source_skeleton_v.rows(), source_skeleton_v.cols()) * (source_skeleton_v.norm() / 100.);

	std::vector<std::unique_ptr<Form>> forms;
	forms.push_back(std::make_unique<CurveTargetForm>(V, curves, source_skeleton_v, target_skeleton_v, skeleton_edges));
	forms.push_back(std::make_unique<CurveCenterTargetForm>(V, curves, source_skeleton_v, target_skeleton_v, skeleton_edges));

	for (auto &form : forms)
	{
		Eigen::VectorXd x = Eigen::VectorXd::Zero(V.size() + 1);

		std::cout << "Test on " << form->name() << " next ...\n";

		for (int rand = 0; rand < n_rand; ++rand)
		{
			x.setRandom();
			x.tail(x.size()-1) /= 100;
			x(0) = x(0) / 2 + 0.5;

			double step = 1e-7;

			form->init(x);
			form->init_lagging(x);
			// Test gradient with finite differences
			{
				Eigen::VectorXd grad;
				form->solution_changed(x);
				form->first_derivative(x, grad);

				Eigen::VectorXd fgrad;
				fd::finite_gradient(
					x, [&form](const Eigen::VectorXd &x) -> double { form->solution_changed(x); return form->value(x); }, fgrad,
					fd::AccuracyOrder::SECOND, step);

				std::cout << std::setprecision(12) << "grad: " << grad.norm() << ", fd: " << fgrad.norm() << "\n";
				std::cout << std::setprecision(12) << "relative error: " << (grad - fgrad).norm() / fgrad.norm() << "\n";
				REQUIRE((grad - fgrad).norm() < tol * std::max(1.e-4, fgrad.norm()));
				// std::cout << grad.transpose() << "\n\n" << fgrad.transpose() << "\n\n";
			}

			// Test hessian with finite differences
			{
				StiffnessMatrix hess;
				form->solution_changed(x);
				form->second_derivative(x, hess);

				Eigen::VectorXd theta1(x.size());
				theta1.setRandom();

				Eigen::MatrixXd fhess;
				fd::finite_jacobian(
					x,
					[&form](const Eigen::VectorXd &x) -> Eigen::VectorXd {
						Eigen::VectorXd grad;
						form->solution_changed(x);
						form->first_derivative(x, grad);
						return grad;
					},
					fhess,
					fd::AccuracyOrder::SECOND, step);

				std::cout << std::setprecision(12) << "hess: " << hess.norm() << ", fd: " << fhess.norm() << "\n";
				std::cout << std::setprecision(12) << "relative error: " << (hess - fhess).norm() / fhess.norm() << "\n";
				REQUIRE((hess - fhess).norm() < tol * std::max(1.e-4, fhess.norm()));
				// std::cout << (Eigen::MatrixXd)hess << "\n\n" << fhess << "\n\n";
			}
		}
	}
}

#if defined(NDEBUG)
std::string tagsdiff = "[form][form_derivatives][garment]";
#else
std::string tagsdiff = "[.][form][form_derivatives][garment]";
#endif

TEST_CASE("Garment forms derivatives", tagsdiff)
{
	const int dim = 3;

	utils::NThread::get().set_num_threads(16);

	Eigen::MatrixXd V;
	Eigen::MatrixXi F;
    igl::read_triangle_mesh(POLYFEM_SOURCE_DIR + std::string("/tests/garment.obj"), V, F);

	auto curves = boundary_curves(F);
	Eigen::MatrixXd target(curves.size(), 3);
	target.setRandom();

	static const int n_rand = 5;
	const double tol = 1e-5;

	std::vector<std::unique_ptr<Form>> forms;
	forms.push_back(std::make_unique<SimilarityForm>(V, F));
	forms.push_back(std::make_unique<CurveTorsionForm>(V, curves));
    forms.push_back(std::make_unique<CurveCurvatureForm>(V, curves));
	forms.push_back(std::make_unique<AngleForm>(V, F));
	forms.push_back(std::make_unique<RelativeScalingForm>(V, F));

	for (auto &form : forms)
	{
		Eigen::VectorXd x = Eigen::VectorXd::Zero(V.size());

		form->init(x);
		form->init_lagging(x);

        Eigen::VectorXd grad;
        form->first_derivative(x, grad);

        REQUIRE(grad.norm() / 1e-10 < 1.);
    }

	forms.push_back(std::make_unique<NormalForm>(V, F));
	forms.push_back(std::make_unique<CurveSizeForm>(V, curves));
    forms.push_back(std::make_unique<AreaForm>(V, F, 1));
	forms.push_back(std::make_unique<DefGradForm>(V, F));

	auto form = std::make_unique<SymmetryForm>(V, curves);
	if (form->enabled())
		forms.push_back(std::move(form));

	{
		Eigen::MatrixXd avatar_v;
		Eigen::MatrixXi avatar_f;
		igl::read_triangle_mesh(POLYFEM_SOURCE_DIR + std::string("/tests/cage.obj"), avatar_v, avatar_f);

		forms.push_back(std::make_unique<FitForm<4>>(V, F, avatar_v, avatar_f, 0.1, std::vector<int>(), "."));
	}

	for (auto &form : forms)
	{
		Eigen::VectorXd x = Eigen::VectorXd::Zero(V.size());

		std::cout << "Test on " << form->name() << " next ...\n";

		for (int rand = 0; rand < n_rand; ++rand)
		{
			x.setRandom();
			x /= 100;

			double step;
			if (form->name() != "garment-fit")
				step = 1e-8;
			else
				step = 1e-7;

			form->init(x);
			form->init_lagging(x);
			// Test gradient with finite differences
			{
				Eigen::VectorXd grad;
				form->solution_changed(x);
				form->first_derivative(x, grad);

				Eigen::VectorXd fgrad;
				fd::finite_gradient(
					x, [&form](const Eigen::VectorXd &x) -> double { form->solution_changed(x); return form->value(x); }, fgrad,
					fd::AccuracyOrder::SECOND, step);

				std::cout << std::setprecision(12) << "grad: " << grad.norm() << ", fd: " << fgrad.norm() << "\n";
				std::cout << std::setprecision(12) << "relative error: " << (grad - fgrad).norm() / fgrad.norm() << "\n";
				REQUIRE((grad - fgrad).norm() < tol * std::max(1.e-4, fgrad.norm()));
				// std::cout << grad.transpose() << "\n\n" << fgrad.transpose() << "\n\n";
			}

			// Test hessian with finite differences
			{
				StiffnessMatrix hess;
				form->solution_changed(x);
				form->second_derivative(x, hess);

				Eigen::VectorXd theta1(x.size());
				theta1.setRandom();

				Eigen::MatrixXd fhess;
				fd::finite_jacobian(
					x,
					[&form](const Eigen::VectorXd &x) -> Eigen::VectorXd {
						Eigen::VectorXd grad;
						form->solution_changed(x);
						form->first_derivative(x, grad);
						return grad;
					},
					fhess,
					fd::AccuracyOrder::SECOND, step);

				std::cout << std::setprecision(12) << "hess: " << hess.norm() << ", fd: " << fhess.norm() << "\n";
				std::cout << std::setprecision(12) << "relative error: " << (hess - fhess).norm() / fhess.norm() << "\n";
				REQUIRE((hess - fhess).norm() < tol * std::max(1.e-4, fhess.norm()));
				// std::cout << (Eigen::MatrixXd)hess << "\n\n" << fhess << "\n\n";
			}
		}
	}
}

TEST_CASE("PCA", "[form][form_derivatives][garment]")
{
	const int N = 20;
	Eigen::MatrixXd points(N, 3);
	for (int i = 0; i < N; i++)
	{
		const double theta = (double)i / N * 2. * M_PI;
		points.row(i) << cos(theta), sin(theta), (rand() % 100) / 1e3;
	}

	Eigen::Vector3d center = points.colwise().sum() / N;
	Eigen::Matrix3d A = Eigen::Matrix3d::Zero();
	for (int i = 0; i < N; i++)
	{
		Eigen::Vector3d v = points.row(i).transpose() - center;
		A += v * v.transpose();
	}

	Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> eigensolver(A);
	std::cout << eigensolver.eigenvalues().transpose() << std::endl;
	std::cout << eigensolver.eigenvectors() << std::endl;
}
