#pragma once
#include <polyfem/Common.hpp>
#include <polyfem/io/OBJData.hpp>
#include <map>

namespace ipc {
    class CollisionMesh;
}

namespace polyfem {
    namespace solver {
        class GarmentNLProblem;
    }

    struct OBJMesh {
        // vertices, triangle faces
        Eigen::MatrixXd v;
        Eigen::MatrixXi f;
        // normals
        Eigen::MatrixXi fn;
        Eigen::MatrixXd cn;
        // textures
        Eigen::MatrixXi ftc;
        Eigen::MatrixXd tc;

        // Material information
        std::string mtl_filename;

        void read(const std::string &path);
        void write(const std::string &path);
    };


    Eigen::Vector3d bbox_size(const Eigen::Matrix<double, -1, 3> &V);

    /// @brief Read a mesh with groups, dispatching by extension: OpenUSD
    /// (.usd/.usda/.usdc) via USDReader when built with POLYFEM_WITH_USD, else OBJ.
    bool read_mesh_with_groups(const std::string &path, OBJData &data);

    /// @brief Write a mesh with groups to `base_path_no_ext` + the extension for
    /// `format`: "usd" -> `.usda` via USDWriter (POLYFEM_WITH_USD), else `.obj`.
    bool write_mesh_with_groups(
        const std::string &base_path_no_ext, const std::string &format, const OBJData &data);

    /// @brief Build a triangulated OBJData (positions + faces only) from Eigen V/F,
    /// for the one-shot outputs that have no group/material structure.
    OBJData eigen_to_obj_data(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F);

    class GarmentSolver
    {
    public:
        void check_intersections(
            const ipc::CollisionMesh &collision_mesh,
            const Eigen::MatrixXd &collision_vertices) const;

        void load_garment_mesh(
            const std::string &mesh_path,
            const std::string &no_fit_spec_path);

        void read_meshes(
            const std::string &avatar_mesh_path,
            const std::string &source_skeleton_path,
            const std::string &target_skeleton_path,
            const std::string &target_avatar_skinning_weights_path);

        void project_avatar_to_skeleton();

        void normalize_meshes();

        void save_result(
            const std::string &path,
            const int index,
            solver::GarmentNLProblem &prob,
            const Eigen::MatrixXd &V,
            const Eigen::MatrixXi &F,
            const Eigen::VectorXd &sol);

        int n_garment_vertices() const { return garment.v.rows(); }
        int n_garment_faces() const { return garment.f.rows(); }

        std::string out_folder;

        // Output mesh format for save_result / one-shot writes: "obj" or "usd".
        std::string out_format = "obj";

        // How far the shrunk avatar sits from the real body at the start of the
        // solve. 0 is the full collapse onto the skeleton, 1 is no shrink.
        //
        // The full collapse puts neighbouring vertices on different bones and
        // stretches the triangle between them across the body; those slivers cut
        // through the garment and the solve refuses to start. Upstream repairs
        // that by exploding the mesh and subdividing, which discards the texture
        // coordinates and materials this fork keeps. A partial shrink keeps every
        // vertex, triangle and UV, and how much is enough depends on how loose
        // the garment is, so it is measured rather than fixed.
        double shrink_blend = 1e-2;

        // Shrink the starting body inward along its own normals by this distance
        // instead of collapsing it onto the skeleton. Zero keeps the collapse.
        //
        // The collapse folds: neighbouring vertices go to different bones and the
        // triangle between them stretches across the body, so the start
        // self-intersects and the solve refuses to begin. A normal offset moves
        // every vertex the same small distance along its own normal, so
        // neighbours stay neighbours and every texture coordinate survives
        // untouched. Too large an offset turns concave detail inside out, so the
        // useful range is below the body's local thickness.
        double shrink_normal_distance = 0.0;

        // Original avatar mesh
        Eigen::MatrixXd avatar_v;
        Eigen::MatrixXi avatar_f;

        // Non-conforming avatar mesh after projection
        Eigen::MatrixXd nc_avatar_v;
        Eigen::MatrixXi nc_avatar_f;

        // Projected avatar mesh that shares the same connectivity as nc_avatar
        Eigen::MatrixXd skinny_avatar_v;
        Eigen::MatrixXi skinny_avatar_f;

        OBJMesh garment;

        Eigen::MatrixXd skeleton_v, target_skeleton_v;
        Eigen::MatrixXi skeleton_b, target_skeleton_b;

        Eigen::MatrixXd target_avatar_skinning_weights;
        Eigen::MatrixXd garment_skinning_weights;

        std::vector<int> not_fit_fids;

        // Avatar and garment material information
        std::string avatar_mtl_filename;
        std::string avatar_mtl_source_path;
        std::string garment_mtl_source_path;

        // Parsed material information
        std::map<std::string, MTLMaterial> avatar_materials;
        std::map<std::string, MTLMaterial> garment_materials;

        // Avatar and garment group information
        std::vector<OBJObject> avatar_objects;
        std::vector<OBJObject> garment_objects;
        std::vector<int> avatar_face_to_group;
        std::vector<int> avatar_face_to_object;
        std::vector<int> garment_face_to_group;
        std::vector<int> garment_face_to_object;

        // Avatar texture coordinates and normals (preserved during projection)
        std::vector<std::vector<double>> avatar_VT;  // Texture coordinates
        std::vector<std::vector<double>> avatar_VN;  // Vertex normals
        std::vector<std::vector<int>> avatar_FT;     // Face texture coordinate indices
        std::vector<std::vector<int>> avatar_FN;     // Face normal indices
    };

	json init(const json &p_args_in, const bool strict_validation);
}
