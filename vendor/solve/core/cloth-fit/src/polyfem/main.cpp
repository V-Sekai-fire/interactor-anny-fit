#include <filesystem>

#include <CLI/CLI.hpp>

#include <igl/read_triangle_mesh.h>
#include <igl/write_triangle_mesh.h>
#include <igl/edges.h>

#include <polysolve/nonlinear/Solver.hpp>

#include <polyfem/utils/StringUtils.hpp>
#include <polyfem/solver/forms/ContactForm.hpp>
#include <polyfem/solver/forms/garment_forms/GarmentForm.hpp>
#include <polyfem/solver/forms/garment_forms/GarmentALForm.hpp>
#include <polyfem/solver/forms/garment_forms/CurveConstraintForm.hpp>
#include <polyfem/solver/forms/garment_forms/CurveCenterTargetForm.hpp>
#include <polyfem/solver/forms/garment_forms/FitForm.hpp>
#include <polyfem/garment/GarmentNLProblem.hpp>
#include <polyfem/solver/ALSolver.hpp>
#include <polyfem/utils/JSONUtils.hpp>
#include <polyfem/utils/Logger.hpp>
#include <polyfem/mesh/MeshUtils.hpp>
#include <polyfem/garment/optimize.hpp>
#ifdef POLYFEM_WITH_USD
#include "loader.hpp"
#endif

#include <fstream>

using namespace polyfem;
using namespace solver;
using namespace mesh;

bool has_arg(const CLI::App &command_line, const std::string &value)
{
	const auto *opt = command_line.get_option_no_throw(value.size() == 1 ? ("-" + value) : ("--" + value));
	if (!opt)
		return false;

	return opt->count() > 0;
}

bool load_json(const std::string &json_file, json &out)
{
	std::ifstream file(json_file);

	if (!file.is_open())
		return false;

	file >> out;

	if (!out.contains("root_path"))
		out["root_path"] = json_file;

	return true;
}

int main(int argc, char **argv)
{
	using namespace polyfem;

	CLI::App command_line{"polyfem"};

	command_line.ignore_case();
	command_line.ignore_underscore();

	// Eigen::setNbThreads(1);
	unsigned max_threads = 16;
	command_line.add_option("--max_threads", max_threads, "Maximum number of threads");

	auto input = command_line.add_option_group("input");

	std::string json_file = "";
	input->add_option("-j,--json", json_file, "Simulation JSON file")->check(CLI::ExistingFile);

	input->require_option(1);

	std::string output_dir = "";
	command_line.add_option("-o,--output_dir", output_dir, "Directory for output files")->check(CLI::ExistingDirectory | CLI::NonexistentPath);

	const std::vector<std::pair<std::string, spdlog::level::level_enum>>
		SPDLOG_LEVEL_NAMES_TO_LEVELS = {
			{"trace", spdlog::level::trace},
			{"debug", spdlog::level::debug},
			{"info", spdlog::level::info},
			{"warning", spdlog::level::warn},
			{"error", spdlog::level::err},
			{"critical", spdlog::level::critical},
			{"off", spdlog::level::off}};
	spdlog::level::level_enum log_level = spdlog::level::debug;
	command_line.add_option("--log_level", log_level, "Log level")
		->transform(CLI::CheckedTransformer(SPDLOG_LEVEL_NAMES_TO_LEVELS, CLI::ignore_case));

	CLI11_PARSE(command_line, argc, argv);

	json in_args = json({});
	{
		if (!json_file.empty())
		{
			const bool ok = load_json(json_file, in_args);

			if (!ok)
				log_and_throw_error(fmt::format("unable to open {} file", json_file));
		}

		if (in_args.empty())
		{
			logger().error("No input file specified!");
			return command_line.exit(CLI::RequiredError("--json"));
		}

		json tmp = json::object();
		if (has_arg(command_line, "log_level"))
			tmp["/output/log/level"_json_pointer] = int(log_level);
		if (has_arg(command_line, "max_threads"))
			tmp["/solver/max_threads"_json_pointer] = max_threads;
		if (has_arg(command_line, "output_dir"))
			tmp["/output/directory"_json_pointer] = std::filesystem::absolute(output_dir);

		assert(tmp.is_object());
		in_args.merge_patch(tmp);
	}

	{
		in_args = init(in_args, false);
	}

	GarmentSolver gstate;

	const std::string out_folder = in_args["/output/directory"_json_pointer];
	const std::string avatar_mesh_path = in_args["avatar_mesh_path"];
	const std::string garment_mesh_path = in_args["garment_mesh_path"];
	const std::string source_skeleton_path = in_args["source_skeleton_path"];
	const std::string target_skeleton_path = in_args["target_skeleton_path"];
	const std::string avatar_skin_weights_path = in_args["avatar_skin_weights_path"];
	const bool self_collision = in_args["contact"]["enabled"];

	if (!std::filesystem::exists(avatar_mesh_path))
		log_and_throw_error("Invalid avatar mesh path: {}", avatar_mesh_path);

	if (!std::filesystem::exists(garment_mesh_path))
		log_and_throw_error("Invalid garment mesh path: {}", garment_mesh_path);

	if (!std::filesystem::exists(source_skeleton_path))
		log_and_throw_error("Invalid source skeleton mesh path: {}", source_skeleton_path);

	if (!std::filesystem::exists(target_skeleton_path))
		log_and_throw_error("Invalid target skeleton mesh path: {}", target_skeleton_path);

	gstate.out_folder = out_folder;
	gstate.out_format = in_args.value("/output/format"_json_pointer, std::string("obj"));
	gstate.shrink_blend = in_args.value("shrink_blend", 1e-2);
	gstate.shrink_normal_distance = in_args.value("shrink_normal_distance", 0.0);
#ifdef POLYFEM_WITH_USD
	cfusd_loader::load_from_env(); // dlopen the USD bridge (no-op if already loaded)
#endif

	gstate.read_meshes(avatar_mesh_path, source_skeleton_path, target_skeleton_path, avatar_skin_weights_path);
	gstate.load_garment_mesh(in_args["garment_mesh_path"], in_args["no_fit_spec_path"]);
	gstate.normalize_meshes();
	gstate.project_avatar_to_skeleton();

	write_mesh_with_groups(out_folder + "/target_avatar", gstate.out_format, eigen_to_obj_data(gstate.avatar_v, gstate.avatar_f));
	write_mesh_with_groups(out_folder + "/projected_avatar", gstate.out_format, eigen_to_obj_data(gstate.skinny_avatar_v, gstate.nc_avatar_f));
	write_edge_mesh(out_folder + "/target_skeleton.obj", gstate.target_skeleton_v, gstate.target_skeleton_b);
	write_edge_mesh(out_folder + "/source_skeleton.obj", gstate.skeleton_v, gstate.skeleton_b);

	logger().info("avatar n_verts: {}, garment n_verts: {}, total n_verts: {}", gstate.nc_avatar_v.rows(), gstate.n_garment_vertices(), gstate.nc_avatar_v.rows() + gstate.n_garment_vertices());

	Eigen::MatrixXi collision_triangles(gstate.nc_avatar_f.rows() + gstate.n_garment_faces(), gstate.garment.f.cols());
	collision_triangles << gstate.nc_avatar_f, gstate.garment.f.array() + gstate.nc_avatar_v.rows();
	Eigen::MatrixXi collision_edges;
	igl::edges(collision_triangles, collision_edges);

	Eigen::MatrixXd collision_vertices(gstate.nc_avatar_v.rows() + gstate.n_garment_vertices(), gstate.garment.v.cols());
	collision_vertices << gstate.skinny_avatar_v, gstate.garment.v;

	ipc::CollisionMesh collision_mesh;
	{
		collision_mesh = ipc::CollisionMesh(
			collision_vertices, collision_edges, collision_triangles);

		const int n_avatar_verts = gstate.nc_avatar_v.rows();
		collision_mesh.can_collide = [n_avatar_verts, self_collision](size_t vi, size_t vj) {
			if (self_collision)
				return vi >= n_avatar_verts || vj >= n_avatar_verts;
			else
				return (vi >= n_avatar_verts && vj < n_avatar_verts) || (vi < n_avatar_verts && vj >= n_avatar_verts);
		};

		gstate.check_intersections(collision_mesh, collision_vertices);
	}

	{
		const double dhat = in_args["contact"]["dhat"];

		Eigen::MatrixXi collision_edges_tmp;
		igl::edges(gstate.garment.f, collision_edges_tmp);

		ipc::CollisionMesh collision_mesh_tmp;
		collision_mesh_tmp = ipc::CollisionMesh(
			gstate.garment.v, collision_edges_tmp, gstate.garment.f);

		ipc::Collisions collision_set;
		Eigen::MatrixXd displaced_surface = collision_mesh_tmp.displace_vertices(Eigen::MatrixXd::Zero(collision_mesh_tmp.full_num_vertices(), 3));
		collision_set.build(collision_mesh_tmp, displaced_surface, dhat, 0, in_args["solver"]["contact"]["CCD"]["broad_phase"]);
		double dist = collision_set.compute_minimum_distance(collision_mesh_tmp, displaced_surface);
		logger().info("Initial distance {}, dhat {}", sqrt(dist), dhat);
	}

	auto curves = boundary_curves(collision_triangles.bottomRows(gstate.n_garment_faces()));
	const Eigen::MatrixXd source_curve_centers = extract_curve_center_targets(collision_vertices, curves, gstate.skeleton_v, gstate.skeleton_b, gstate.skeleton_v);
	const Eigen::MatrixXd target_curve_centers = extract_curve_center_targets(collision_vertices, curves, gstate.skeleton_v, gstate.skeleton_b, gstate.target_skeleton_v);

	const Eigen::MatrixXd initial_garment_v = gstate.garment.v;
	Eigen::MatrixXd cur_garment_v = gstate.garment.v;
	int save_id = 0;
	const int total_steps = in_args["incremental_steps"];
	const int stride = in_args["output"]["skip_frame"];

	std::vector<std::shared_ptr<Form>> persistent_forms;
	std::vector<std::shared_ptr<Form>> persistent_full_forms;
	std::shared_ptr<CurveSizeForm> curve_size_form;
	{
		auto similarity_form = std::make_shared<SimilarityForm>(collision_vertices, collision_triangles.bottomRows(gstate.n_garment_faces()));
		similarity_form->set_weight(in_args["similarity_penalty_weight"]);
		persistent_forms.push_back(similarity_form);

		if (in_args["curvature_penalty_weight"] > 0)
		{
			auto curvature_form = std::make_shared<CurveCurvatureForm>(collision_vertices, curves);
			curvature_form->set_weight(in_args["curvature_penalty_weight"]);
			persistent_forms.push_back(curvature_form);
		}

		if (in_args["twist_penalty_weight"] > 0)
		{
			auto twist_form = std::make_shared<CurveTorsionForm>(collision_vertices, curves);
			twist_form->set_weight(in_args["twist_penalty_weight"]);
			persistent_forms.push_back(twist_form);
		}

		if (in_args["symmetry_weight"] > 0)
		{
			auto sym_form = std::make_shared<SymmetryForm>(collision_vertices, curves);
			sym_form->set_weight(in_args["symmetry_weight"]);
			if (sym_form->enabled())
				persistent_forms.push_back(sym_form);
		}

		if (in_args["curve_size_weight"] > 0)
		{
			curve_size_form = std::make_shared<CurveSizeForm>(collision_vertices, curves);
			curve_size_form->disable();
			curve_size_form->set_weight(in_args["curve_size_weight"]);
			persistent_forms.push_back(curve_size_form);
		}

		{
			const double dhat = in_args["contact"]["dhat"];

			std::shared_ptr<ContactForm> contact_form = std::make_shared<ContactForm>(collision_mesh, dhat, 1, false, false, false, false, in_args["solver"]["contact"]["CCD"]["broad_phase"], in_args["solver"]["contact"]["CCD"]["tolerance"], in_args["solver"]["contact"]["CCD"]["max_iterations"]);
			contact_form->set_weight(1);
			contact_form->set_barrier_stiffness(in_args["solver"]["contact"]["barrier_stiffness"]);
			contact_form->save_ccd_debug_meshes = false;
			persistent_forms.push_back(contact_form);
		}

		const auto tmp_curves = boundary_curves(gstate.garment.f);
		auto center_target_form = std::make_shared<CurveTargetForm>(initial_garment_v, tmp_curves, gstate.skeleton_v, gstate.target_skeleton_v, gstate.skeleton_b, in_args["is_skirt"], in_args["curve_center_target_automatic_bone_generation"]);
		center_target_form->set_weight(in_args["curve_center_target_weight"]);
		persistent_full_forms.push_back(center_target_form);
	}

	Eigen::MatrixXd sol = Eigen::MatrixXd::Zero(1 + initial_garment_v.size(), 1);
	for (int substep = 0; substep < total_steps; ++substep)
	{
		const double prev_alpha = substep / (double)total_steps;
		const double next_alpha = (substep + 1) / (double)total_steps;

		logger().info("Start substep {} out of {}", substep + 1, total_steps);

		// continuation
		const Eigen::MatrixXd next_avatar_v = (gstate.nc_avatar_v - gstate.skinny_avatar_v) * next_alpha + gstate.skinny_avatar_v;
		const Eigen::MatrixXd next_curve_centers = (target_curve_centers - source_curve_centers) * next_alpha + source_curve_centers;

		std::vector<std::shared_ptr<Form>> forms = persistent_forms;
		std::shared_ptr<PointPenaltyForm> pen_form;
		std::shared_ptr<PointLagrangianForm> lagr_form;
		std::shared_ptr<FitForm<4>> fit_form;
		{
			std::vector<int> indices(gstate.nc_avatar_v.size());
			for (int i = 0; i < indices.size(); i++)
				indices[i] = i;
			pen_form = std::make_shared<PointPenaltyForm>(utils::flatten(next_avatar_v - gstate.skinny_avatar_v), indices);
			forms.push_back(pen_form);

			lagr_form = std::make_shared<PointLagrangianForm>(utils::flatten(next_avatar_v - gstate.skinny_avatar_v), indices);
			forms.push_back(lagr_form);

			fit_form = std::make_shared<FitForm<4>>(collision_vertices, collision_triangles.bottomRows(gstate.n_garment_faces()), gstate.avatar_v, gstate.avatar_f, in_args["voxel_size"], gstate.not_fit_fids, out_folder);
			fit_form->disable();
			fit_form->set_weight(in_args["fit_weight"]);
			forms.push_back(fit_form);

			if (in_args["curve_size_weight"] > 0)
				curve_size_form->disable();
		}

		GarmentNLProblem nl_problem(1 + initial_garment_v.size(), utils::flatten(gstate.nc_avatar_v - gstate.skinny_avatar_v), forms, persistent_full_forms);
		nl_problem.set_target_value(next_alpha);

		nl_problem.line_search_begin(sol, sol);
		if (!std::isfinite(nl_problem.value(sol))
			|| !nl_problem.is_step_valid(sol, sol)
			|| !nl_problem.is_step_collision_free(sol, sol))
			log_and_throw_error("Failed to apply boundary conditions!");

		std::shared_ptr<polysolve::nonlinear::Solver> nl_solver = polysolve::nonlinear::Solver::create(in_args["solver"]["augmented_lagrangian"]["nonlinear"], in_args["solver"]["linear"], 1., logger());

		double initial_weight = in_args["solver"]["augmented_lagrangian"]["initial_weight"];
		const double scaling = in_args["solver"]["augmented_lagrangian"]["scaling"];
		const double max_weight = in_args["solver"]["augmented_lagrangian"]["max_weight"].get<double>();
		logger().debug("Set initial AL weight to {}", initial_weight);

		ALSolver<GarmentNLProblem, PointLagrangianForm, PointPenaltyForm> al_solver(
			lagr_form, pen_form,
			initial_weight, scaling, max_weight,
			in_args["solver"]["augmented_lagrangian"]["eta"],
			in_args["solver"]["augmented_lagrangian"]["error_threshold"],
			[](const Eigen::VectorXd &x) {});

		nl_problem.post_step_call_back = [&](const Eigen::VectorXd &sol) {
			if (save_id % stride == 0)
				gstate.save_result(out_folder, save_id / stride, nl_problem, collision_vertices, collision_triangles, sol);
			++save_id;
		};

		al_solver.solve_al(nl_solver, nl_problem, sol);

		fit_form->enable();
		if (in_args["curve_size_weight"] > 0 && substep == total_steps - 1)
			curve_size_form->enable();

		nl_solver = polysolve::nonlinear::Solver::create(in_args["solver"]["nonlinear"], in_args["solver"]["linear"], 1., logger());
		al_solver.solve_reduced(nl_solver, nl_problem, sol);

		cur_garment_v = initial_garment_v + utils::unflatten(sol.bottomRows(cur_garment_v.size()), 3);
	}

	logger().info("Garment retargeting succeeded!");

	return EXIT_SUCCESS;
}
