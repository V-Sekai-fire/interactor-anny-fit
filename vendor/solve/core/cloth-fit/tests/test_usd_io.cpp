////////////////////////////////////////////////////////////////////////////////
// Smoke-tests the cloth_fit_usd bridge via its C ABI (linked directly here, the
// way the standalone CLI links it; the solver/NIF instead dlopen it). Proves the
// USD read/write + the OBJData<->flat-array marshalling round-trip, and that the
// bridge loads usd_ms without a TBB double-instance abort against PolyFEM's TBB.
#include <cloth_fit_usd.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <filesystem>
#include <string>
#include <vector>
////////////////////////////////////////////////////////////////////////////////

using Catch::Approx;

namespace
{
    std::string temp_usd(const std::string &name)
    {
        auto dir = std::filesystem::temp_directory_path() / "cfusd_bridge_tests";
        std::filesystem::create_directories(dir);
        return (dir / name).string();
    }
} // namespace

TEST_CASE("cfusd bridge mesh round trip", "[usd][io]")
{
    REQUIRE(cfusd_init(nullptr) == 1);

    // A quad + a triangle, per-corner UVs + normals, two material groups.
    cfusd_mesh_t *m = cfusd_mesh_create();
    const double pos[] = {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 2, 0.5, 0};
    cfusd_mesh_set_positions(m, 5, pos);
    const int32_t counts[] = {4, 3};
    const int32_t idx[] = {0, 1, 2, 3, 1, 4, 2};
    cfusd_mesh_set_faces(m, 2, counts, 7, idx);
    const double uv[] = {0, 0, 1, 0, 1, 1, 0, 1, 1.5, 0.5};
    const int32_t uvi[] = {0, 1, 2, 3, 1, 4, 2};
    cfusd_mesh_set_uvs(m, 5, uv, 7, uvi);
    const double nrm[] = {0, 0, 1};
    const int32_t ni[] = {0, 0, 0, 0, 0, 0, 0};
    cfusd_mesh_set_normals(m, 1, nrm, 7, ni);
    cfusd_mesh_set_mtl_filename(m, "sample.mtl");
    const int32_t g0[] = {0};
    const int32_t g1[] = {1};
    cfusd_mesh_add_group(m, "Obj", "quad_grp", "red", 1, g0);
    cfusd_mesh_add_group(m, "Obj", "tri_grp", "blue", 1, g1);

    const std::string path = temp_usd("bridge_rt.usda");
    REQUIRE(cfusd_write_usd(m, path.c_str()) == 1);
    cfusd_mesh_destroy(m);

    cfusd_mesh_t *r = cfusd_read_usd(path.c_str());
    REQUIRE(r != nullptr);

    REQUIRE(cfusd_mesh_get_vertex_count(r) == 5);
    REQUIRE(cfusd_mesh_get_face_count(r) == 2);
    REQUIRE(cfusd_mesh_get_index_count(r) == 7);

    int32_t rc[2] = {0, 0};
    int32_t ri[7] = {0};
    cfusd_mesh_get_faces(r, rc, ri);
    REQUIRE(rc[0] == 4); // quad stays a quad
    REQUIRE(rc[1] == 3);

    std::vector<double> rp(15);
    cfusd_mesh_get_positions(r, rp.data());
    REQUIRE(rp[12] == Approx(2.0)); // last vertex x

    REQUIRE(cfusd_mesh_get_uv_count(r) == 5);
    REQUIRE(cfusd_mesh_get_normal_count(r) == 1);

    char mtl[64] = {0};
    cfusd_mesh_get_mtl_filename(r, mtl, 64);
    REQUIRE(std::string(mtl) == "sample.mtl");

    REQUIRE(cfusd_mesh_get_group_count(r) == 2);
    char obj[64] = {0}, grp[64] = {0}, mat[64] = {0};
    cfusd_mesh_get_group_names(r, 0, obj, 64, grp, 64, mat, 64);
    REQUIRE(std::string(grp) == "quad_grp");
    REQUIRE(std::string(mat) == "red");
    cfusd_mesh_get_group_names(r, 1, obj, 64, grp, 64, mat, 64);
    REQUIRE(std::string(grp) == "tri_grp");
    REQUIRE(std::string(mat) == "blue");

    cfusd_mesh_destroy(r);
}
