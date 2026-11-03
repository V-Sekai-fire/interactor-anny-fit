#include <polyfem/garment/run_retarget.hpp>

// Pure garment-retargeting solve, extracted from the NIF simulate() so the NIF
// and the weftfit_retarget bridge share one implementation. 0 = success (result
// JSON in result_out); non-zero sets error_out. No Erlang/NIF types — lives in
// the garment core (libpolyfem, POLYFEM_WITH_GARMENT).

#include <polyfem/garment/optimize.hpp>
#include <polyfem/garment/coordinate_system.hpp>
#include <polyfem/garment/GarmentNLProblem.hpp>
#include <polyfem/solver/forms/ContactForm.hpp>
#include <polyfem/solver/forms/garment_forms/GarmentForm.hpp>
#include <polyfem/solver/forms/garment_forms/GarmentALForm.hpp>
#include <polyfem/solver/forms/garment_forms/CurveConstraintForm.hpp>
#include <polyfem/solver/forms/garment_forms/CurveCenterTargetForm.hpp>
#include <polyfem/solver/forms/garment_forms/FitForm.hpp>
#include <polyfem/solver/ALSolver.hpp>
#include <polyfem/utils/JSONUtils.hpp>
#include <polyfem/utils/Logger.hpp>
#include <polyfem/mesh/MeshUtils.hpp>
#include <polyfem/io/OBJWriter.hpp>
#include <polyfem/io/MatrixIO.hpp>
#ifdef POLYFEM_WITH_USD
#include "loader.hpp"
#endif
#include <igl/edges.h>
#include <igl/read_triangle_mesh.h>
#include <igl/readOBJ.h>
#include <polysolve/nonlinear/Solver.hpp>
#include <ipc/ipc.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include <filesystem>
#include <chrono>

using json = nlohmann::json;
using namespace polyfem;
using namespace polyfem::solver;
using namespace polyfem::mesh;

namespace polyfem::garment
{
    int run_retarget(const std::string &config, const std::string &output_path,
                     std::string &result_out, std::string &error_out)
    {
    // Input validation
    if (output_path.empty()) {
        { error_out = "Output path cannot be empty"; return 1; }
    }

    try {
        auto start_time = std::chrono::high_resolution_clock::now();

        // Extract JSON configuration from payload
        if (config.empty()) {
            { error_out = "Empty configuration payload"; return 1; }
        }
        std::string config_json = config;

        // Parse JSON configuration
        json in_args;
        try {
            in_args = json::parse(config_json);
        } catch (const json::parse_error& e) {
            std::string error_msg = "JSON parse error: " + std::string(e.what());
            { error_out = error_msg; return 1; }
        }

        // Create output directory
        std::string output_dir(output_path);
        std::filesystem::create_directories(output_dir);

        // Set up logging to be less verbose for NIF usage
        spdlog::set_level(spdlog::level::warn);
        polyfem::logger().set_level(spdlog::level::warn);

        // Initialize PolyFEM with the configuration
        json args;
        try {
            args = polyfem::init(in_args, false);
        } catch (const std::exception& e) {
            std::string error_msg = "PolyFEM initialization error: " + std::string(e.what());
            { error_out = error_msg; return 1; }
        }

        // Create GarmentSolver instance
        GarmentSolver gstate;
        gstate.out_folder = output_dir;
        gstate.out_format = args.value("/output/format"_json_pointer, std::string("obj"));
        gstate.shrink_blend = args.value("shrink_blend", 1e-2);
        gstate.shrink_normal_distance = args.value("shrink_normal_distance", 0.0);
#ifdef POLYFEM_WITH_USD
        cfusd_loader::load_from_env(); // dlopen the USD bridge (no-op if already loaded)
#endif

        // Extract paths from configuration
        const std::string avatar_mesh_path = args["avatar_mesh_path"];
        const std::string garment_mesh_path = args["garment_mesh_path"];
        const std::string source_skeleton_path = args["source_skeleton_path"];
        const std::string target_skeleton_path = args["target_skeleton_path"];
        const std::string avatar_skin_weights_path = args["avatar_skin_weights_path"];
        const bool self_collision = args["contact"]["enabled"];

        // Validate input files exist
        if (!std::filesystem::exists(avatar_mesh_path)) {
            { error_out = "Invalid avatar mesh path: " + avatar_mesh_path; return 1; }
        }
        if (!std::filesystem::exists(garment_mesh_path)) {
            { error_out = "Invalid garment mesh path: " + garment_mesh_path; return 1; }
        }
        if (!std::filesystem::exists(source_skeleton_path)) {
            { error_out = "Invalid source skeleton path: " + source_skeleton_path; return 1; }
        }
        if (!std::filesystem::exists(target_skeleton_path)) {
            { error_out = "Invalid target skeleton path: " + target_skeleton_path; return 1; }
        }

        // Load meshes and prepare simulation
        gstate.read_meshes(avatar_mesh_path, source_skeleton_path, target_skeleton_path, avatar_skin_weights_path);
        gstate.load_garment_mesh(garment_mesh_path, args["no_fit_spec_path"]);

        // Normalize inputs from their declared authoring convention into the
        // canonical Godot frame (+Y up, +Z front, right-handed). Everything
        // downstream — solve, collision, and USD/OBJ output — is then Godot/
        // glTF-ready. Default input convention is Blender (+Z up, -Y front, RH);
        // override via setup.json "input_coordinate_system".
        {
            const polyfem::garment::Convention input_cs = polyfem::garment::parse_convention(
                args.contains("input_coordinate_system") ? args["input_coordinate_system"] : json::object(),
                polyfem::garment::blender_default());
            const Eigen::Matrix3d M = polyfem::garment::to_canonical(input_cs);
            if (!M.isIdentity(1e-9)) {
                polyfem::garment::apply_to_rows(M, gstate.avatar_v);
                polyfem::garment::apply_to_rows(M, gstate.garment.v);
                polyfem::garment::apply_to_rows(M, gstate.skeleton_v);
                polyfem::garment::apply_to_rows(M, gstate.target_skeleton_v);
                polyfem::garment::apply_to_normals(M, gstate.avatar_VN);
                if (M.determinant() < 0) {
                    // Handedness flip: reverse winding so normals stay outward.
                    polyfem::garment::flip_winding(gstate.avatar_f);
                    polyfem::garment::flip_winding(gstate.garment.f);
                }
            }
        }

        gstate.normalize_meshes();
        gstate.project_avatar_to_skeleton();

        // Save initial meshes (OBJ or USD per out_format)
        polyfem::write_mesh_with_groups(output_dir + "/target_avatar", gstate.out_format, polyfem::eigen_to_obj_data(gstate.avatar_v, gstate.avatar_f));
        polyfem::write_mesh_with_groups(output_dir + "/projected_avatar", gstate.out_format, polyfem::eigen_to_obj_data(gstate.skinny_avatar_v, gstate.nc_avatar_f));
        write_edge_mesh(output_dir + "/target_skeleton.obj", gstate.target_skeleton_v, gstate.target_skeleton_b);
        write_edge_mesh(output_dir + "/source_skeleton.obj", gstate.skeleton_v, gstate.skeleton_b);

        // Set up collision mesh
        Eigen::MatrixXi collision_triangles(gstate.nc_avatar_f.rows() + gstate.n_garment_faces(), gstate.garment.f.cols());
        collision_triangles << gstate.nc_avatar_f, gstate.garment.f.array() + gstate.nc_avatar_v.rows();
        Eigen::MatrixXi collision_edges;
        igl::edges(collision_triangles, collision_edges);

        Eigen::MatrixXd collision_vertices(gstate.nc_avatar_v.rows() + gstate.n_garment_vertices(), gstate.garment.v.cols());
        collision_vertices << gstate.skinny_avatar_v, gstate.garment.v;

        ipc::CollisionMesh collision_mesh;
        collision_mesh = ipc::CollisionMesh(collision_vertices, collision_edges, collision_triangles);

        const int n_avatar_verts = gstate.nc_avatar_v.rows();
        collision_mesh.can_collide = [n_avatar_verts, self_collision](size_t vi, size_t vj) {
            if (self_collision)
                return vi >= n_avatar_verts || vj >= n_avatar_verts;
            else
                return (vi >= n_avatar_verts && vj < n_avatar_verts) || (vi < n_avatar_verts && vj >= n_avatar_verts);
        };

        // The intersection-free start is the barrier's precondition.
        if (args["contact"]["enabled"])
            gstate.check_intersections(collision_mesh, collision_vertices);
        else
            logger().warn("contact disabled: starting from a possibly intersecting "
                          "state, penalties only, no intersection-free guarantee");

        // Set up boundary curves and targets
        auto curves = boundary_curves(collision_triangles.bottomRows(gstate.n_garment_faces()));
        const Eigen::MatrixXd source_curve_centers = extract_curve_center_targets(collision_vertices, curves, gstate.skeleton_v, gstate.skeleton_b, gstate.skeleton_v);
        const Eigen::MatrixXd target_curve_centers = extract_curve_center_targets(collision_vertices, curves, gstate.skeleton_v, gstate.skeleton_b, gstate.target_skeleton_v);

        const Eigen::MatrixXd initial_garment_v = gstate.garment.v;
        int save_id = 0;
        const int total_steps = args["incremental_steps"];
        const int stride = args["output"]["skip_frame"];

        // Set up persistent forms
        std::vector<std::shared_ptr<Form>> persistent_forms;
        std::vector<std::shared_ptr<Form>> persistent_full_forms;
        std::shared_ptr<CurveSizeForm> curve_size_form;

        // Similarity form
        auto similarity_form = std::make_shared<SimilarityForm>(collision_vertices, collision_triangles.bottomRows(gstate.n_garment_faces()));
        similarity_form->set_weight(args["similarity_penalty_weight"]);
        persistent_forms.push_back(similarity_form);

        // Optional forms based on configuration
        if (args["curvature_penalty_weight"] > 0) {
            auto curvature_form = std::make_shared<CurveCurvatureForm>(collision_vertices, curves);
            curvature_form->set_weight(args["curvature_penalty_weight"]);
            persistent_forms.push_back(curvature_form);
        }

        if (args["twist_penalty_weight"] > 0) {
            auto twist_form = std::make_shared<CurveTorsionForm>(collision_vertices, curves);
            twist_form->set_weight(args["twist_penalty_weight"]);
            persistent_forms.push_back(twist_form);
        }

        if (args["symmetry_weight"] > 0) {
            auto sym_form = std::make_shared<SymmetryForm>(collision_vertices, curves);
            sym_form->set_weight(args["symmetry_weight"]);
            if (sym_form->enabled())
                persistent_forms.push_back(sym_form);
        }

        if (args["curve_size_weight"] > 0) {
            curve_size_form = std::make_shared<CurveSizeForm>(collision_vertices, curves);
            curve_size_form->disable();
            curve_size_form->set_weight(args["curve_size_weight"]);
            persistent_forms.push_back(curve_size_form);
        }

        // Contact form
        const double dhat = args["contact"]["dhat"];
        std::shared_ptr<ContactForm> contact_form = std::make_shared<ContactForm>(
            collision_mesh, dhat, 1, false, false, false, false,
            args["solver"]["contact"]["CCD"]["broad_phase"],
            args["solver"]["contact"]["CCD"]["tolerance"],
            args["solver"]["contact"]["CCD"]["max_iterations"]);
        contact_form->set_weight(1);
        contact_form->set_barrier_stiffness(args["solver"]["contact"]["barrier_stiffness"]);
        contact_form->save_ccd_debug_meshes = false;
        persistent_forms.push_back(contact_form);

        // Curve target form
        const auto tmp_curves = boundary_curves(gstate.garment.f);
        auto center_target_form = std::make_shared<CurveTargetForm>(
            initial_garment_v, tmp_curves, gstate.skeleton_v, gstate.target_skeleton_v,
            gstate.skeleton_b, args["is_skirt"], args["curve_center_target_automatic_bone_generation"]);
        center_target_form->set_weight(args["curve_center_target_weight"]);
        persistent_full_forms.push_back(center_target_form);

        // Main simulation loop
        Eigen::MatrixXd sol = Eigen::MatrixXd::Zero(1 + initial_garment_v.size(), 1);

        for (int substep = 0; substep < total_steps; ++substep) {
            const double next_alpha = (substep + 1) / (double)total_steps;

            // Continuation
            const Eigen::MatrixXd next_avatar_v = (gstate.nc_avatar_v - gstate.skinny_avatar_v) * next_alpha + gstate.skinny_avatar_v;

            std::vector<std::shared_ptr<Form>> forms = persistent_forms;
            std::shared_ptr<PointPenaltyForm> pen_form;
            std::shared_ptr<PointLagrangianForm> lagr_form;
            std::shared_ptr<FitForm<4>> fit_form;

            // Set up step-specific forms
            std::vector<int> indices(gstate.nc_avatar_v.size());
            for (int i = 0; i < indices.size(); i++)
                indices[i] = i;
            pen_form = std::make_shared<PointPenaltyForm>(utils::flatten(next_avatar_v - gstate.skinny_avatar_v), indices);
            forms.push_back(pen_form);

            lagr_form = std::make_shared<PointLagrangianForm>(utils::flatten(next_avatar_v - gstate.skinny_avatar_v), indices);
            forms.push_back(lagr_form);

            fit_form = std::make_shared<FitForm<4>>(collision_vertices, collision_triangles.bottomRows(gstate.n_garment_faces()),
                                                   gstate.avatar_v, gstate.avatar_f, args["voxel_size"], gstate.not_fit_fids, output_dir);
            fit_form->disable();
            fit_form->set_weight(args["fit_weight"]);
            forms.push_back(fit_form);

            if (args["curve_size_weight"] > 0)
                curve_size_form->disable();

            // Create and solve NL problem
            GarmentNLProblem nl_problem(1 + initial_garment_v.size(), utils::flatten(gstate.nc_avatar_v - gstate.skinny_avatar_v), forms, persistent_full_forms);
            nl_problem.set_target_value(next_alpha);

            nl_problem.line_search_begin(sol, sol);
            if (!std::isfinite(nl_problem.value(sol)) || !nl_problem.is_step_valid(sol, sol) || !nl_problem.is_step_collision_free(sol, sol)) {
                { error_out = "Failed to apply boundary conditions!"; return 1; }
            }

            // Set up solvers
            std::shared_ptr<polysolve::nonlinear::Solver> nl_solver = polysolve::nonlinear::Solver::create(
                args["solver"]["augmented_lagrangian"]["nonlinear"], args["solver"]["linear"], 1., polyfem::logger());

            double initial_weight = args["solver"]["augmented_lagrangian"]["initial_weight"];
            const double scaling = args["solver"]["augmented_lagrangian"]["scaling"];
            const double max_weight = args["solver"]["augmented_lagrangian"]["max_weight"].get<double>();

            ALSolver<GarmentNLProblem, PointLagrangianForm, PointPenaltyForm> al_solver(
                lagr_form, pen_form, initial_weight, scaling, max_weight,
                args["solver"]["augmented_lagrangian"]["eta"],
                args["solver"]["augmented_lagrangian"]["error_threshold"],
                [](const Eigen::VectorXd &x) {});

            // Set up save callback
            nl_problem.post_step_call_back = [&](const Eigen::VectorXd &sol) {
                if (save_id % stride == 0)
                    gstate.save_result(output_dir, save_id / stride, nl_problem, collision_vertices, collision_triangles, sol);
                ++save_id;
            };

            // Solve AL problem
            al_solver.solve_al(nl_solver, nl_problem, sol);

            // Enable fit form and solve reduced problem
            fit_form->enable();
            if (args["curve_size_weight"] > 0 && substep == total_steps - 1)
                curve_size_form->enable();

            nl_solver = polysolve::nonlinear::Solver::create(args["solver"]["nonlinear"], args["solver"]["linear"], 1., polyfem::logger());
            al_solver.solve_reduced(nl_solver, nl_problem, sol);
        }

        // Calculate simulation time
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        double simulation_time = duration.count() / 1000.0;

        // Create result JSON
        json result;
        result["status"] = "completed";
        result["output_path"] = output_dir;
        result["config_size"] = config.size();
        result["simulation_time"] = simulation_time;
        result["iterations"] = total_steps;
        result["message"] = "Garment retargeting succeeded!";
        result["avatar_vertices"] = gstate.nc_avatar_v.rows();
        result["garment_vertices"] = gstate.n_garment_vertices();
        result["total_vertices"] = gstate.nc_avatar_v.rows() + gstate.n_garment_vertices();

        std::string result_str = result.dump();
        { result_out = result_str; return 0; }

    } catch (const std::exception& e) {
        std::string error_msg = "Simulation error: " + std::string(e.what());
        { error_out = error_msg; return 1; }
    }
        return 0;
    }
} // namespace polyfem::garment
