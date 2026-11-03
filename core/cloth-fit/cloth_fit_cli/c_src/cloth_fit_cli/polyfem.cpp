#include <fine.hpp>

// The garment retarget solve now lives entirely behind the weftfit_retarget
// dlopen bridge (src/retarget_bridge): the NIF calls wf_retarget_run through the
// bridge's import lib and LINKS ZERO PolyFEM/TBB — exactly the design already used
// for USD I/O via cloth_fit_usd. The remaining NIF functions (validate/info) only
// need the header-only coordinate convention + nlohmann/json + a small OBJ reader.
#include "weftfit_retarget.h"          // wf_retarget_* C ABI (bound via import lib)
#include "wfrt_loader.hpp"             // wfrt_loader::load_from_env — dlopen the bridge
#include <polyfem/garment/coordinate_system.hpp> // header-only (inline; Eigen/json only)

#include <nlohmann/json.hpp>

#include <string>
#include <cstring>
#include <memory>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <chrono>

using json = nlohmann::json;

// Helpers for building the {:ok, _} / {:error, reason} result tuples that the
// Elixir API (ClothFitCli.PolyFEM) pattern-matches on.
namespace {
    fine::Term ok_string(ErlNifEnv* env, const std::string& value) {
        return fine::encode(env, fine::Ok<std::string>(value));
    }
    fine::Term ok_bool(ErlNifEnv* env, bool value) {
        return fine::encode(env, fine::Ok<bool>(value));
    }
    fine::Term error(ErlNifEnv* env, const std::string& reason) {
        return fine::encode(env, fine::Error<std::string>(reason));
    }
}

// Simple JSON implementation for basic operations (fallback)
namespace simple_json {
    std::string escape_string(const std::string& str) {
        std::string result;
        for (char c : str) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    }
}

// Simple 3D vector class for basic operations
struct Vec3 {
    double x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    Vec3 cross(const Vec3& other) const {
        return Vec3(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
    }
    double norm() const { return std::sqrt(x*x + y*y + z*z); }
};

// Simple mesh structure for basic validation
struct SimpleMesh {
    std::vector<Vec3> vertices;
    std::vector<std::vector<int>> faces;

    bool load_obj(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "v") {
                double x, y, z;
                if (iss >> x >> y >> z) {
                    vertices.emplace_back(x, y, z);
                }
            } else if (prefix == "f") {
                std::vector<int> face;
                std::string vertex_data;
                while (iss >> vertex_data) {
                    size_t slash_pos = vertex_data.find('/');
                    std::string vertex_index_str = vertex_data.substr(0, slash_pos);
                    int vertex_index = std::stoi(vertex_index_str) - 1; // OBJ is 1-indexed
                    face.push_back(vertex_index);
                }
                if (face.size() >= 3) {
                    faces.push_back(face);
                }
            }
        }

        return !vertices.empty() && !faces.empty();
    }

    Vec3 get_bbox_min() const {
        if (vertices.empty()) return Vec3();
        Vec3 min_pt = vertices[0];
        for (const auto& v : vertices) {
            min_pt.x = std::min(min_pt.x, v.x);
            min_pt.y = std::min(min_pt.y, v.y);
            min_pt.z = std::min(min_pt.z, v.z);
        }
        return min_pt;
    }

    Vec3 get_bbox_max() const {
        if (vertices.empty()) return Vec3();
        Vec3 max_pt = vertices[0];
        for (const auto& v : vertices) {
            max_pt.x = std::max(max_pt.x, v.x);
            max_pt.y = std::max(max_pt.y, v.y);
            max_pt.z = std::max(max_pt.z, v.z);
        }
        return max_pt;
    }

    double calculate_surface_area() const {
        double total_area = 0.0;
        for (const auto& face : faces) {
            if (face.size() >= 3) {
                Vec3 v0 = vertices[face[0]];
                Vec3 v1 = vertices[face[1]];
                Vec3 v2 = vertices[face[2]];
                Vec3 cross_product = (v1 - v0).cross(v2 - v0);
                total_area += 0.5 * cross_product.norm();
            }
        }
        return total_area;
    }
};

fine::Term simulate(ErlNifEnv* env, std::string config, std::string output_path) {
    // The solve runs entirely inside the weftfit_retarget bridge (which links
    // PolyFEM + the garment core + TBB); the NIF just dlopens it and calls the C
    // ABI, so the NIF itself carries zero PolyFEM. Same pattern as cloth_fit_usd.
    if (!wfrt_loader::load_from_env()) {
        return error(env, "Failed to load the weftfit_retarget solve bridge");
    }
    int rc = wf_retarget_run(config.c_str(), output_path.c_str());
    if (rc != 0) {
        const char* msg = wf_retarget_last_error();
        return error(env, (msg != nullptr && msg[0] != '\0') ? std::string(msg)
                                                             : std::string("Simulation failed"));
    }
    const char* result = wf_retarget_last_result();
    return ok_string(env, (result != nullptr) ? std::string(result) : std::string());
}

fine::Term validate_garment_mesh(ErlNifEnv* env, std::string mesh_path) {
    // Input validation
    if (mesh_path.empty()) {
        return error(env, "Invalid mesh path");
    }

    try {
        // Load mesh using simple OBJ reader
        SimpleMesh mesh;
        if (!mesh.load_obj(mesh_path)) {
            return error(env, "Failed to read mesh file");
        }

        // Basic validation checks for garment meshes
        bool is_valid = true;
        std::string validation_errors;

        // Check if mesh has vertices and faces
        if (mesh.vertices.empty()) {
            is_valid = false;
            validation_errors += "No vertices found. ";
        }

        if (mesh.faces.empty()) {
            is_valid = false;
            validation_errors += "No faces found. ";
        }

        // Check for reasonable mesh size (garments should have some complexity)
        if (mesh.vertices.size() < 10) {
            is_valid = false;
            validation_errors += "Too few vertices for a garment mesh. ";
        }

        if (mesh.faces.size() < 10) {
            is_valid = false;
            validation_errors += "Too few faces for a garment mesh. ";
        }

        // Check for valid face indices
        for (const auto& face : mesh.faces) {
            for (int vertex_idx : face) {
                if (vertex_idx < 0 || vertex_idx >= static_cast<int>(mesh.vertices.size())) {
                    is_valid = false;
                    validation_errors += "Invalid face index found. ";
                    break;
                }
            }
            if (!is_valid) break;
        }

        // Check for degenerate faces (faces with zero area)
        int degenerate_count = 0;
        for (const auto& face : mesh.faces) {
            if (face.size() >= 3) {
                Vec3 v0 = mesh.vertices[face[0]];
                Vec3 v1 = mesh.vertices[face[1]];
                Vec3 v2 = mesh.vertices[face[2]];
                double area = 0.5 * (v1 - v0).cross(v2 - v0).norm();
                if (area < 1e-12) {
                    degenerate_count++;
                }
            }
        }

        if (degenerate_count > static_cast<int>(mesh.faces.size()) * 0.1) { // More than 10% degenerate faces
            is_valid = false;
            validation_errors += "Too many degenerate faces. ";
        }

        // Check bounding box (garment should have reasonable dimensions)
        Vec3 bbox_min = mesh.get_bbox_min();
        Vec3 bbox_max = mesh.get_bbox_max();
        Vec3 bbox_size = bbox_max - bbox_min;

        double max_dimension = std::max({bbox_size.x, bbox_size.y, bbox_size.z});
        if (max_dimension < 1e-6) {
            is_valid = false;
            validation_errors += "Mesh has zero or near-zero dimensions. ";
        }

        if (!is_valid) {
            return error(env, validation_errors);
        }

        return ok_bool(env, is_valid);

    } catch (const std::exception& e) {
        std::string error_msg = "Error validating garment mesh: " + std::string(e.what());
        return error(env, error_msg);
    }
}

fine::Term validate_avatar_mesh(ErlNifEnv* env, std::string mesh_path) {
    // Input validation
    if (mesh_path.empty()) {
        return error(env, "Invalid mesh path");
    }

    try {
        // Load mesh using simple OBJ reader
        SimpleMesh mesh;
        if (!mesh.load_obj(mesh_path)) {
            return error(env, "Failed to read mesh file");
        }

        // Basic validation checks for avatar meshes
        bool is_valid = true;
        std::string validation_errors;

        // Check if mesh has vertices and faces
        if (mesh.vertices.empty()) {
            is_valid = false;
            validation_errors += "No vertices found. ";
        }

        if (mesh.faces.empty()) {
            is_valid = false;
            validation_errors += "No faces found. ";
        }

        // Avatar meshes should be more complex than garment meshes
        if (mesh.vertices.size() < 100) {
            is_valid = false;
            validation_errors += "Too few vertices for an avatar mesh. ";
        }

        if (mesh.faces.size() < 100) {
            is_valid = false;
            validation_errors += "Too few faces for an avatar mesh. ";
        }

        // Check for valid face indices
        for (const auto& face : mesh.faces) {
            for (int vertex_idx : face) {
                if (vertex_idx < 0 || vertex_idx >= static_cast<int>(mesh.vertices.size())) {
                    is_valid = false;
                    validation_errors += "Invalid face index found. ";
                    break;
                }
            }
            if (!is_valid) break;
        }

        // Check bounding box and proportions (avatar should be humanoid-like)
        Vec3 bbox_min = mesh.get_bbox_min();
        Vec3 bbox_max = mesh.get_bbox_max();
        Vec3 bbox_size = bbox_max - bbox_min;

        double max_dimension = std::max({bbox_size.x, bbox_size.y, bbox_size.z});
        if (max_dimension < 1e-6) {
            is_valid = false;
            validation_errors += "Mesh has zero or near-zero dimensions. ";
        }

        // Proportion sanity check against the default input convention (Blender
        // Z-up): a humanoid's up axis should be its tallest extent. Checking the
        // declared up axis (not a hard-coded Y) avoids rejecting Z-up assets that
        // retarget fine, while still catching meshes authored in the wrong frame.
        // See src/polyfem/garment/coordinate_system.hpp.
        const double ext[3] = {bbox_size.x, bbox_size.y, bbox_size.z};
        const int up = polyfem::garment::up_axis_index(polyfem::garment::blender_default());
        const double up_extent = ext[up];
        const double max_horizontal = std::max(ext[(up + 1) % 3], ext[(up + 2) % 3]);
        const double height_to_width = up_extent / std::max(max_horizontal, 1e-9);
        if (height_to_width < 0.2 || height_to_width > 20.0) {
            is_valid = false;
            validation_errors += "Extreme aspect ratio for avatar mesh (the up axis is not the tallest extent — is the mesh in the declared input convention?). ";
        }

        // Check for degenerate faces
        int degenerate_count = 0;
        for (const auto& face : mesh.faces) {
            if (face.size() >= 3) {
                Vec3 v0 = mesh.vertices[face[0]];
                Vec3 v1 = mesh.vertices[face[1]];
                Vec3 v2 = mesh.vertices[face[2]];
                double area = 0.5 * (v1 - v0).cross(v2 - v0).norm();
                if (area < 1e-12) {
                    degenerate_count++;
                }
            }
        }

        if (degenerate_count > static_cast<int>(mesh.faces.size()) * 0.05) { // More than 5% degenerate faces
            is_valid = false;
            validation_errors += "Too many degenerate faces. ";
        }

        // Check mesh complexity (avatars should have reasonable detail)
        if (mesh.vertices.size() > 100000) {
            // Very high poly count - might cause performance issues
            // For now we'll allow it, but could add warnings
        }

        if (!is_valid) {
            return error(env, validation_errors);
        }

        return ok_bool(env, is_valid);

    } catch (const std::exception& e) {
        std::string error_msg = "Error validating avatar mesh: " + std::string(e.what());
        return error(env, error_msg);
    }
}

fine::Term load_garment_info(ErlNifEnv* env, std::string garment_path) {
    // Input validation
    if (garment_path.empty()) {
        return error(env, "Invalid garment path");
    }

    try {
        // Load garment mesh using simple OBJ reader
        SimpleMesh mesh;
        if (!mesh.load_obj(garment_path)) {
            return error(env, "Failed to read garment mesh file");
        }

        // Calculate mesh statistics
        int vertex_count = static_cast<int>(mesh.vertices.size());
        int face_count = static_cast<int>(mesh.faces.size());

        // Calculate bounding box
        Vec3 bbox_min = mesh.get_bbox_min();
        Vec3 bbox_max = mesh.get_bbox_max();
        Vec3 bbox_size = bbox_max - bbox_min;

        // Create JSON response with mesh information using nlohmann::json
        json info_json;
        info_json["vertex_count"] = vertex_count;
        info_json["face_count"] = face_count;
        info_json["bounding_box"]["min"] = {bbox_min.x, bbox_min.y, bbox_min.z};
        info_json["bounding_box"]["max"] = {bbox_max.x, bbox_max.y, bbox_max.z};
        info_json["bounding_box"]["size"] = {bbox_size.x, bbox_size.y, bbox_size.z};
        info_json["mesh_type"] = "garment";
        info_json["file_path"] = garment_path;

        std::string info_str = info_json.dump();
        return ok_string(env, info_str);

    } catch (const std::exception& e) {
        std::string error_msg = "Error loading garment info: " + std::string(e.what());
        return error(env, error_msg);
    }
}

fine::Term load_avatar_info(ErlNifEnv* env, std::string avatar_path) {
    // Input validation
    if (avatar_path.empty()) {
        return error(env, "Invalid avatar path");
    }

    try {
        // Load avatar mesh using simple OBJ reader
        SimpleMesh mesh;
        if (!mesh.load_obj(avatar_path)) {
            return error(env, "Failed to read avatar mesh file");
        }

        // Calculate mesh statistics
        int vertex_count = static_cast<int>(mesh.vertices.size());
        int face_count = static_cast<int>(mesh.faces.size());

        // Calculate bounding box
        Vec3 bbox_min = mesh.get_bbox_min();
        Vec3 bbox_max = mesh.get_bbox_max();
        Vec3 bbox_size = bbox_max - bbox_min;

        // Calculate additional avatar-specific metrics
        double surface_area = mesh.calculate_surface_area();

        // Check if this looks like a valid avatar mesh (basic heuristics)
        bool is_likely_avatar = (vertex_count > 1000 && face_count > 1000 &&
                                bbox_size.y > bbox_size.x && bbox_size.y > bbox_size.z);

        // Create JSON response with mesh information using nlohmann::json
        json info_json;
        info_json["vertex_count"] = vertex_count;
        info_json["face_count"] = face_count;
        info_json["surface_area"] = surface_area;
        info_json["bounding_box"]["min"] = {bbox_min.x, bbox_min.y, bbox_min.z};
        info_json["bounding_box"]["max"] = {bbox_max.x, bbox_max.y, bbox_max.z};
        info_json["bounding_box"]["size"] = {bbox_size.x, bbox_size.y, bbox_size.z};
        info_json["mesh_type"] = "avatar";
        info_json["is_likely_avatar"] = is_likely_avatar;
        info_json["file_path"] = avatar_path;

        std::string info_str = info_json.dump();
        return ok_string(env, info_str);

    } catch (const std::exception& e) {
        std::string error_msg = "Error loading avatar info: " + std::string(e.what());
        return error(env, error_msg);
    }
}

// NIF registration (Fine). Arity is derived automatically; the second argument
// is the scheduling flag. The solver is long-running and CPU-bound, so it runs
// on a dirty scheduler to avoid blocking the BEAM.
FINE_NIF(simulate, ERL_NIF_DIRTY_JOB_CPU_BOUND);
FINE_NIF(validate_garment_mesh, 0);
FINE_NIF(validate_avatar_mesh, 0);
FINE_NIF(load_garment_info, 0);
FINE_NIF(load_avatar_info, 0);

FINE_INIT("Elixir.PolyFem");
