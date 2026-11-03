////////////////////////////////////////////////////////////////////////////////
#include <polyfem/io/MatrixIO.hpp>
#include <polyfem/io/OBJReader.hpp>
#include <polyfem/io/OBJWriter.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <Eigen/Dense>
#include <Eigen/Sparse>

#include <filesystem>
#include <fstream>
#include <random>
#include <string>
////////////////////////////////////////////////////////////////////////////////

using namespace polyfem;

class TestFixture {
public:
    TestFixture() {
        // Create temporary test directory
        test_dir = std::filesystem::temp_directory_path() / "polyfem_io_tests";
        std::filesystem::create_directories(test_dir);
    }

    ~TestFixture() {
        // Clean up test files
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }

    std::string temp_file(const std::string& suffix) {
        return (test_dir / ("test_" + suffix)).string();
    }

private:
    std::filesystem::path test_dir;
};

TEST_CASE_METHOD(TestFixture, "Matrix IO - ASCII format", "[io][matrix]")
{
    SECTION("Double matrix read/write") {
        Eigen::MatrixXd original(3, 4);
        original << 1.1, 2.2, 3.3, 4.4,
                   5.5, 6.6, 7.7, 8.8,
                   9.9, 10.10, 11.11, 12.12;

        std::string filepath = temp_file("matrix_ascii.txt");

        // Test write
        REQUIRE(io::write_matrix_ascii(filepath, original));

        // Test read
        Eigen::MatrixXd loaded;
        REQUIRE(io::read_matrix_ascii<double>(filepath, loaded));

        // Verify dimensions and values
        REQUIRE(loaded.rows() == original.rows());
        REQUIRE(loaded.cols() == original.cols());

        for (int i = 0; i < original.rows(); ++i) {
            for (int j = 0; j < original.cols(); ++j) {
                REQUIRE(loaded(i, j) == Catch::Approx(original(i, j)));
            }
        }
    }

    SECTION("Integer matrix read/write") {
        Eigen::MatrixXi original(2, 3);
        original << 1, 2, 3,
                   4, 5, 6;

        std::string filepath = temp_file("matrix_int_ascii.txt");

        REQUIRE(io::write_matrix_ascii(filepath, original));

        Eigen::MatrixXi loaded;
        REQUIRE(io::read_matrix_ascii<int>(filepath, loaded));

        REQUIRE(loaded.rows() == original.rows());
        REQUIRE(loaded.cols() == original.cols());
        REQUIRE(loaded == original);
    }

    SECTION("Empty matrix") {
        Eigen::MatrixXd empty(0, 0);
        std::string filepath = temp_file("empty_matrix.txt");

        REQUIRE(io::write_matrix_ascii(filepath, empty));

        Eigen::MatrixXd loaded;
        REQUIRE(io::read_matrix_ascii<double>(filepath, loaded));
        REQUIRE(loaded.rows() == 0);
        REQUIRE(loaded.cols() == 0);
    }

    SECTION("Single element matrix") {
        Eigen::MatrixXd single(1, 1);
        single << 42.0;

        std::string filepath = temp_file("single_matrix.txt");

        REQUIRE(io::write_matrix_ascii(filepath, single));

        Eigen::MatrixXd loaded;
        REQUIRE(io::read_matrix_ascii<double>(filepath, loaded));
        REQUIRE(loaded.rows() == 1);
        REQUIRE(loaded.cols() == 1);
        REQUIRE(loaded(0, 0) == Catch::Approx(42.0));
    }
}

TEST_CASE_METHOD(TestFixture, "Matrix IO - Binary format", "[io][matrix]")
{
    SECTION("Large random matrix") {
        const int rows = 100;
        const int cols = 50;

        Eigen::MatrixXd original = Eigen::MatrixXd::Random(rows, cols);
        std::string filepath = temp_file("matrix_binary.bin");

        REQUIRE(io::write_matrix_binary(filepath, original));

        Eigen::MatrixXd loaded;
        REQUIRE(io::read_matrix_binary<double>(filepath, loaded));

        REQUIRE(loaded.rows() == rows);
        REQUIRE(loaded.cols() == cols);

        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                REQUIRE(loaded(i, j) == Catch::Approx(original(i, j)));
            }
        }
    }

    SECTION("Precision preservation") {
        Eigen::MatrixXd original(2, 2);
        original << 1.23456789012345, -2.34567890123456,
                   3.45678901234567, -4.56789012345678;

        std::string filepath = temp_file("precision_matrix.bin");

        REQUIRE(io::write_matrix_binary(filepath, original));

        Eigen::MatrixXd loaded;
        REQUIRE(io::read_matrix_binary<double>(filepath, loaded));

        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                REQUIRE(loaded(i, j) == Catch::Approx(original(i, j)).epsilon(1e-15));
            }
        }
    }
}

TEST_CASE_METHOD(TestFixture, "Matrix IO - Generic interface", "[io][matrix]")
{
    SECTION("Auto-detect ASCII format") {
        Eigen::MatrixXd original = Eigen::MatrixXd::Random(5, 3);
        std::string filepath = temp_file("auto_ascii.txt");

        REQUIRE(io::write_matrix(filepath, original));

        Eigen::MatrixXd loaded;
        REQUIRE(io::read_matrix<double>(filepath, loaded));

        REQUIRE(loaded.rows() == original.rows());
        REQUIRE(loaded.cols() == original.cols());

        for (int i = 0; i < original.rows(); ++i) {
            for (int j = 0; j < original.cols(); ++j) {
                REQUIRE(loaded(i, j) == Catch::Approx(original(i, j)));
            }
        }
    }

    SECTION("Auto-detect binary format") {
        Eigen::MatrixXd original = Eigen::MatrixXd::Random(5, 3);
        std::string filepath = temp_file("auto_binary.bin");

        REQUIRE(io::write_matrix(filepath, original));

        Eigen::MatrixXd loaded;
        REQUIRE(io::read_matrix<double>(filepath, loaded));

        REQUIRE(loaded.rows() == original.rows());
        REQUIRE(loaded.cols() == original.cols());

        for (int i = 0; i < original.rows(); ++i) {
            for (int j = 0; j < original.cols(); ++j) {
                REQUIRE(loaded(i, j) == Catch::Approx(original(i, j)));
            }
        }
    }
}

TEST_CASE_METHOD(TestFixture, "Sparse Matrix CSV export", "[io][sparse]")
{
    SECTION("Basic sparse matrix") {
        Eigen::SparseMatrix<double> sparse(4, 4);
        std::vector<Eigen::Triplet<double>> triplets;
        triplets.push_back(Eigen::Triplet<double>(0, 0, 1.0));
        triplets.push_back(Eigen::Triplet<double>(1, 1, 2.0));
        triplets.push_back(Eigen::Triplet<double>(2, 2, 3.0));
        triplets.push_back(Eigen::Triplet<double>(0, 3, 4.0));
        sparse.setFromTriplets(triplets.begin(), triplets.end());

        std::string filepath = temp_file("sparse_matrix.csv");
        REQUIRE(io::write_sparse_matrix_csv(filepath, sparse));

        // Verify file exists and has content
        std::ifstream file(filepath);
        REQUIRE(file.is_open());

        std::string line;
        std::getline(file, line);
        REQUIRE(line == "shape,4,4");  // First line is shape info

        std::getline(file, line);
        REQUIRE(line == "Row,Col,Val");  // Second line is header

        // Verify we have the expected number of non-zero entries
        int line_count = 0;
        while (std::getline(file, line)) {
            line_count++;
        }
        REQUIRE(line_count == 4); // 4 non-zero entries
    }

    SECTION("Empty sparse matrix") {
        Eigen::SparseMatrix<double> sparse(3, 3);

        std::string filepath = temp_file("empty_sparse.csv");
        REQUIRE(io::write_sparse_matrix_csv(filepath, sparse));

        std::ifstream file(filepath);
        REQUIRE(file.is_open());

        std::string line;
        std::getline(file, line);
        REQUIRE(line == "shape,3,3");  // First line is shape info

        std::getline(file, line);
        REQUIRE(line == "Row,Col,Val");  // Second line is header

        REQUIRE_FALSE(std::getline(file, line)); // Should be empty after header
    }
}

TEST_CASE_METHOD(TestFixture, "Error handling", "[io][error]")
{
    SECTION("Non-existent file read") {
        Eigen::MatrixXd mat;
        REQUIRE_FALSE(io::read_matrix<double>("non_existent_file.txt", mat));
    }

    SECTION("Invalid path write") {
        Eigen::MatrixXd mat = Eigen::MatrixXd::Random(2, 2);
        REQUIRE_FALSE(io::write_matrix("/invalid/path/file.txt", mat));
    }

    SECTION("Corrupted matrix file") {
        std::string filepath = temp_file("corrupted.txt");
        std::ofstream file(filepath);
        file << "1.0 2.0\n";
        file << "3.0 invalid_number\n";  // This should cause parsing to fail
        file.close();

        Eigen::MatrixXd mat;
        REQUIRE_FALSE(io::read_matrix_ascii<double>(filepath, mat));
    }
}
