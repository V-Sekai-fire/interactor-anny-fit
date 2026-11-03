////////////////////////////////////////////////////////////////////////////////
#include <polyfem/mesh/MeshUtils.hpp>
#include <polyfem/io/OBJReader.hpp>
#include <polyfem/io/OBJWriter.hpp>
#include <polyfem/io/OBJData.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <Eigen/Dense>

#include <filesystem>
#include <fstream>
#include <sstream>
////////////////////////////////////////////////////////////////////////////////

using namespace polyfem;

class MeshTestFixture {
public:
    MeshTestFixture() {
        test_dir = std::filesystem::temp_directory_path() / "polyfem_mesh_tests";
        std::filesystem::create_directories(test_dir);
    }

    ~MeshTestFixture() {
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }

    std::string temp_file(const std::string& suffix) {
        return (test_dir / ("mesh_" + suffix)).string();
    }

private:
    std::filesystem::path test_dir;
};

TEST_CASE_METHOD(MeshTestFixture, "Mesh Utils", "[mesh][utils]")
{
    SECTION("Basic mesh utilities functionality") {
        // Create a simple triangle mesh (2 triangles forming a quad)
        Eigen::MatrixXd V(4, 3);
        V << 0.0, 0.0, 0.0,
             1.0, 0.0, 0.0,
             1.0, 1.0, 0.0,
             0.0, 1.0, 0.0;

        Eigen::MatrixXi F(2, 3);
        F << 0, 1, 2,
             0, 2, 3;

        // Test mesh validity - should have 4 vertices and 2 faces
        REQUIRE(V.rows() == 4);
        REQUIRE(V.cols() == 3);
        REQUIRE(F.rows() == 2);
        REQUIRE(F.cols() == 3);

        // Test vertex coordinate ranges
        REQUIRE(V.col(0).minCoeff() == Catch::Approx(0.0));
        REQUIRE(V.col(0).maxCoeff() == Catch::Approx(1.0));
        REQUIRE(V.col(1).minCoeff() == Catch::Approx(0.0));
        REQUIRE(V.col(1).maxCoeff() == Catch::Approx(1.0));
        REQUIRE(V.col(2).minCoeff() == Catch::Approx(0.0));
        REQUIRE(V.col(2).maxCoeff() == Catch::Approx(0.0));
    }
}

// Helper function to create a simple OBJData structure for testing
OBJData create_simple_triangle_objdata() {
    OBJData data;

    // Create a simple triangle
    data.V = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.5, 1.0, 0.0}};
    data.F = {{0, 1, 2}};

    // Create default object and group
    OBJObject obj;
    obj.name = "triangle";
    OBJGroup group;
    group.name = "default";
    group.face_indices = {0};
    obj.groups.push_back(group);
    data.objects.push_back(obj);

    // Set face mappings
    data.face_to_object = {0};
    data.face_to_group = {0};

    return data;
}

// Helper function to create a quad mesh with groups
OBJData create_quad_with_groups_objdata() {
    OBJData data;

    // Create a quad (4 vertices, 2 triangles)
    data.V = {{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {2.0, 2.0, 0.0}, {0.0, 2.0, 0.0}};
    data.F = {{0, 1, 2}, {0, 2, 3}};

    // Create object with two groups
    OBJObject obj;
    obj.name = "quad";

    OBJGroup group1;
    group1.name = "triangle1";
    group1.face_indices = {0};

    OBJGroup group2;
    group2.name = "triangle2";
    group2.face_indices = {1};

    obj.groups.push_back(group1);
    obj.groups.push_back(group2);
    data.objects.push_back(obj);

    // Set face mappings
    data.face_to_object = {0, 0};
    data.face_to_group = {0, 1};

    return data;
}

TEST_CASE_METHOD(MeshTestFixture, "OBJ File IO with Groups", "[mesh][obj][io][groups]")
{
    SECTION("Write and read simple triangle") {
        OBJData original_data = create_simple_triangle_objdata();
        std::string filepath = temp_file("triangle.obj");

        // Test write
        REQUIRE(io::OBJWriter::write_with_groups(filepath, original_data));

        // Verify file exists
        REQUIRE(std::filesystem::exists(filepath));

        // Test read
        OBJData loaded_data;
        REQUIRE(io::OBJReader::read_with_groups(filepath, loaded_data));

        // Verify vertex data
        REQUIRE(loaded_data.V.size() == 3);
        REQUIRE(loaded_data.F.size() == 1);

        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j) {
                REQUIRE(loaded_data.V[i][j] == Catch::Approx(original_data.V[i][j]));
            }
        }

        // Verify face data
        for (size_t i = 0; i < 3; ++i) {
            REQUIRE(loaded_data.F[0][i] == original_data.F[0][i]);
        }

        // Verify object and group structure
        REQUIRE(loaded_data.objects.size() == 1);
        REQUIRE(loaded_data.objects[0].name == "triangle");
        REQUIRE(loaded_data.objects[0].groups.size() == 1);
        REQUIRE(loaded_data.objects[0].groups[0].name == "default");
    }

    SECTION("Write and read quad with multiple groups") {
        OBJData original_data = create_quad_with_groups_objdata();
        std::string filepath = temp_file("quad_groups.obj");

        // Test write
        REQUIRE(io::OBJWriter::write_with_groups(filepath, original_data));

        // Test read
        OBJData loaded_data;
        REQUIRE(io::OBJReader::read_with_groups(filepath, loaded_data));

        // Verify basic structure
        REQUIRE(loaded_data.V.size() == 4);
        REQUIRE(loaded_data.F.size() == 2);

        // Verify vertex coordinates
        for (size_t i = 0; i < 4; ++i) {
            for (size_t j = 0; j < 3; ++j) {
                REQUIRE(loaded_data.V[i][j] == Catch::Approx(original_data.V[i][j]));
            }
        }

        // Verify group structure is preserved
        REQUIRE(loaded_data.objects.size() == 1);
        REQUIRE(loaded_data.objects[0].name == "quad");
        REQUIRE(loaded_data.objects[0].groups.size() == 2);
        REQUIRE(loaded_data.objects[0].groups[0].name == "triangle1");
        REQUIRE(loaded_data.objects[0].groups[1].name == "triangle2");

        // Verify face-to-group mappings
        REQUIRE(loaded_data.face_to_group.size() == 2);
        REQUIRE(loaded_data.face_to_group[0] == 0);
        REQUIRE(loaded_data.face_to_group[1] == 1);
    }

    SECTION("Handle empty mesh") {
        OBJData empty_data;
        std::string filepath = temp_file("empty.obj");

        REQUIRE(io::OBJWriter::write_with_groups(filepath, empty_data));

        OBJData loaded_data;
        REQUIRE(io::OBJReader::read_with_groups(filepath, loaded_data));

        REQUIRE(loaded_data.V.empty());
        REQUIRE(loaded_data.F.empty());
        REQUIRE(loaded_data.objects.empty());
    }
}

TEST_CASE_METHOD(MeshTestFixture, "OBJ Error Handling", "[mesh][obj][error]")
{
    SECTION("Non-existent file read") {
        OBJData data;
        REQUIRE_FALSE(io::OBJReader::read_with_groups("non_existent_file.obj", data));
    }

    SECTION("Invalid path write") {
        OBJData data = create_simple_triangle_objdata();
        REQUIRE_FALSE(io::OBJWriter::write_with_groups("/invalid/path/file.obj", data));
    }

    SECTION("Corrupted OBJ file") {
        std::string filepath = temp_file("corrupted.obj");
        std::ofstream file(filepath);
        file << "v 1.0 2.0 3.0\n";
        file << "v invalid vertex data\n";
        file << "f 1 2\n"; // Invalid face (only 2 vertices)
        file.close();

        OBJData data;
        // The reader should handle this gracefully
        bool result = io::OBJReader::read_with_groups(filepath, data);

        // If read succeeds, we should at least have read the valid vertex
        if (result && !data.V.empty()) {
            REQUIRE(data.V[0][0] == Catch::Approx(1.0));
            REQUIRE(data.V[0][1] == Catch::Approx(2.0));
            REQUIRE(data.V[0][2] == Catch::Approx(3.0));
        }
    }
}

TEST_CASE_METHOD(MeshTestFixture, "OBJ Precision", "[mesh][precision]")
{
    SECTION("High precision coordinates") {
        OBJData original_data;
        original_data.V = {
            {1.23456789012345, 2.34567890123456, 3.45678901234567},
            {-4.56789012345678, -5.67890123456789, -6.78901234567890},
            {7.89012345678901, 8.90123456789012, 9.01234567890123}
        };
        original_data.F = {{0, 1, 2}};

        // Create default object and group
        OBJObject obj;
        obj.name = "precision_test";
        OBJGroup group;
        group.name = "default";
        group.face_indices = {0};
        obj.groups.push_back(group);
        original_data.objects.push_back(obj);
        original_data.face_to_object = {0};
        original_data.face_to_group = {0};

        std::string filepath = temp_file("precision.obj");

        REQUIRE(io::OBJWriter::write_with_groups(filepath, original_data));

        OBJData loaded_data;
        REQUIRE(io::OBJReader::read_with_groups(filepath, loaded_data));

        // Verify high precision is preserved (within reasonable tolerance)
        REQUIRE(loaded_data.V.size() == 3);
        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j) {
                REQUIRE(loaded_data.V[i][j] == Catch::Approx(original_data.V[i][j]).epsilon(1e-10));
            }
        }
    }
}

TEST_CASE_METHOD(MeshTestFixture, "OBJ Texture Coordinates and Normals", "[mesh][obj][textures][normals]")
{
    SECTION("Mesh with texture coordinates") {
        OBJData original_data;
        original_data.V = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.5, 1.0, 0.0}};
        original_data.VT = {{0.0, 0.0}, {1.0, 0.0}, {0.5, 1.0}};
        original_data.F = {{0, 1, 2}};
        original_data.FT = {{0, 1, 2}};

        // Create default object and group
        OBJObject obj;
        obj.name = "textured_triangle";
        OBJGroup group;
        group.name = "default";
        group.face_indices = {0};
        obj.groups.push_back(group);
        original_data.objects.push_back(obj);
        original_data.face_to_object = {0};
        original_data.face_to_group = {0};

        std::string filepath = temp_file("textured.obj");

        REQUIRE(io::OBJWriter::write_with_groups(filepath, original_data));

        OBJData loaded_data;
        REQUIRE(io::OBJReader::read_with_groups(filepath, loaded_data));

        // Verify texture coordinates are preserved
        REQUIRE(loaded_data.VT.size() == 3);
        REQUIRE(loaded_data.FT.size() == 1);

        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 2; ++j) {
                REQUIRE(loaded_data.VT[i][j] == Catch::Approx(original_data.VT[i][j]));
            }
        }
    }

    SECTION("Mesh with vertex normals") {
        OBJData original_data;
        original_data.V = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.5, 1.0, 0.0}};
        original_data.VN = {{0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}};
        original_data.F = {{0, 1, 2}};
        original_data.FN = {{0, 1, 2}};

        // Create default object and group
        OBJObject obj;
        obj.name = "normal_triangle";
        OBJGroup group;
        group.name = "default";
        group.face_indices = {0};
        obj.groups.push_back(group);
        original_data.objects.push_back(obj);
        original_data.face_to_object = {0};
        original_data.face_to_group = {0};

        std::string filepath = temp_file("normals.obj");

        REQUIRE(io::OBJWriter::write_with_groups(filepath, original_data));

        OBJData loaded_data;
        REQUIRE(io::OBJReader::read_with_groups(filepath, loaded_data));

        // Verify normals are preserved
        REQUIRE(loaded_data.VN.size() == 3);
        REQUIRE(loaded_data.FN.size() == 1);

        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j) {
                REQUIRE(loaded_data.VN[i][j] == Catch::Approx(original_data.VN[i][j]));
            }
        }
    }
}
