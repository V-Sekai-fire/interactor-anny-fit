#pragma once

#include <string>
#include <vector>
#include <array>

namespace polyfem {

    /// @brief Structure to hold parsed MTL material information
    struct MTLMaterial {
        std::string name;
        std::array<double, 3> Ka = {{1.0, 1.0, 1.0}}; // ambient color
        std::array<double, 3> Kd = {{1.0, 1.0, 1.0}}; // diffuse color
        std::array<double, 3> Ks = {{0.5, 0.5, 0.5}}; // specular color
        std::array<double, 3> Ke = {{0.0, 0.0, 0.0}}; // emissive color
        double Ns = 200.0; // specular exponent
        double Ni = 1.5;   // optical density
        double d = 1.0;    // dissolve (transparency)
        int illum = 2;     // illumination model

        // Texture maps
        std::string map_Kd; // diffuse texture
        std::string map_d;  // dissolve texture
        std::string map_Ks; // specular texture
        std::string map_Ka; // ambient texture
        std::string bump;   // bump map
    };

    /// @brief Structure to hold OBJ group information
    struct OBJGroup {
        std::string name;
        std::string material_name;
        std::vector<int> face_indices; // Indices of faces belonging to this group
    };

    /// @brief Structure to hold OBJ object information
    struct OBJObject {
        std::string name;
        std::vector<OBJGroup> groups;
    };

    /// @brief Structure to hold complete OBJ data with groups and materials
    struct OBJData {
        std::vector<std::vector<double>> V;   // Vertices (positions)
        std::vector<std::vector<double>> VT;  // Texture coordinates
        std::vector<std::vector<double>> VN;  // Vertex normals
        std::vector<std::vector<int>> F;      // Face vertex indices
        std::vector<std::vector<int>> FT;     // Face texture coordinate indices
        std::vector<std::vector<int>> FN;     // Face normal indices
        std::vector<OBJObject> objects;       // Objects with groups
        std::vector<int> face_to_group;       // Face index to group index mapping
        std::vector<int> face_to_object;      // Face index to object index mapping
        std::string mtl_filename;             // Referenced MTL file
    };

} // namespace polyfem
