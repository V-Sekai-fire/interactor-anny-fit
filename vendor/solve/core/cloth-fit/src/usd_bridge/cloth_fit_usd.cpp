// cloth_fit_usd bridge — the only unit that links OpenUSD. Implements the C ABI
// in cloth_fit_usd.h over polyfem::OBJData + io::USDReader/USDWriter. Everything
// pxr/TBB is hidden; only CFUSD_API symbols are exported.

#define CFUSD_BUILDING_DLL
#include "cloth_fit_usd.h"

#include <polyfem/io/OBJData.hpp>
#include <polyfem/io/USDReader.hpp>
#include <polyfem/io/USDWriter.hpp>
#include <polyfem/io/USDPlugins.hpp>

#include <pxr/base/plug/registry.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

using polyfem::OBJData;
using polyfem::OBJGroup;
using polyfem::OBJObject;

struct cfusd_mesh
{
    OBJData data;
};

namespace
{
    void copy_str(const std::string &s, char *out, int32_t cap)
    {
        if (out == nullptr || cap <= 0)
        {
            return;
        }
        const int32_t n = static_cast<int32_t>(std::min(static_cast<size_t>(cap - 1), s.size()));
        std::memcpy(out, s.data(), n);
        out[n] = '\0';
    }

    // Directory containing this bridge module, so the USD plugin registry can be
    // resolved next to it (relocatable — e.g. bundled into a Burrito binary).
    std::string bridge_module_dir()
    {
#ifdef _WIN32
        HMODULE hm = nullptr;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                   GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCSTR>(&cfusd_version), &hm) != 0)
        {
            char buf[MAX_PATH] = {0};
            GetModuleFileNameA(hm, buf, MAX_PATH);
            std::string p(buf);
            const auto slash = p.find_last_of("\\/");
            return (slash == std::string::npos) ? std::string() : p.substr(0, slash);
        }
        return "";
#else
        Dl_info info;
        if (dladdr(reinterpret_cast<void *>(&cfusd_version), &info) != 0 && info.dli_fname != nullptr)
        {
            std::string p(info.dli_fname);
            const auto slash = p.find_last_of('/');
            return (slash == std::string::npos) ? std::string() : p.substr(0, slash);
        }
        return "";
#endif
    }

    // Resolve a flat group index (across all objects, in order) to (object, group).
    const OBJGroup *group_at(const OBJData &d, int32_t flat, const OBJObject **owner)
    {
        int32_t i = 0;
        for (const OBJObject &obj : d.objects)
        {
            for (const OBJGroup &g : obj.groups)
            {
                if (i == flat)
                {
                    if (owner != nullptr)
                    {
                        *owner = &obj;
                    }
                    return &g;
                }
                ++i;
            }
        }
        return nullptr;
    }
} // namespace

extern "C" {

CFUSD_API const char *cfusd_version(void)
{
    return "0.1.0";
}

CFUSD_API int32_t cfusd_init(const char *plugin_dir)
{
    std::string dir;
    if (plugin_dir != nullptr && plugin_dir[0] != '\0')
    {
        dir = plugin_dir;
    }
    else
    {
        // Prefer the plugin tree bundled next to the bridge (relocatable);
        // otherwise fall back to the path compiled in at build time.
        const std::string self = bridge_module_dir();
        if (!self.empty())
        {
            const std::string cand = self + "/usd";
            std::error_code ec;
            if (std::filesystem::exists(cand + "/plugInfo.json", ec))
            {
                dir = cand;
            }
        }
    }

    if (!dir.empty())
    {
        PXR_NS::PlugRegistry::GetInstance().RegisterPlugins(dir);
    }
    else
    {
        polyfem::io::usd_detail::ensure_plugins_registered();
    }
    return 1;
}

CFUSD_API cfusd_mesh_t *cfusd_mesh_create(void)
{
    return new cfusd_mesh();
}

CFUSD_API void cfusd_mesh_destroy(cfusd_mesh_t *mesh)
{
    delete mesh;
}

// --- setters --------------------------------------------------------------

CFUSD_API void cfusd_mesh_set_positions(cfusd_mesh_t *mesh, int32_t vertex_count, const double *xyz)
{
    mesh->data.V.resize(vertex_count);
    for (int32_t i = 0; i < vertex_count; ++i)
    {
        mesh->data.V[i] = {xyz[3 * i], xyz[3 * i + 1], xyz[3 * i + 2]};
    }
}

CFUSD_API void cfusd_mesh_set_faces(
    cfusd_mesh_t *mesh, int32_t face_count, const int32_t *counts,
    int32_t index_count, const int32_t *indices)
{
    (void)index_count;
    mesh->data.F.resize(face_count);
    int32_t k = 0;
    for (int32_t f = 0; f < face_count; ++f)
    {
        mesh->data.F[f].assign(indices + k, indices + k + counts[f]);
        k += counts[f];
    }
}

CFUSD_API void cfusd_mesh_set_uvs(
    cfusd_mesh_t *mesh, int32_t uv_count, const double *uv,
    int32_t index_count, const int32_t *uv_indices)
{
    mesh->data.VT.resize(uv_count);
    for (int32_t i = 0; i < uv_count; ++i)
    {
        mesh->data.VT[i] = {uv[2 * i], uv[2 * i + 1]};
    }
    // Distribute the per-corner UV indices across faces to mirror F's shape.
    mesh->data.FT.resize(mesh->data.F.size());
    int32_t k = 0;
    for (size_t f = 0; f < mesh->data.F.size() && k < index_count; ++f)
    {
        const size_t n = mesh->data.F[f].size();
        mesh->data.FT[f].assign(uv_indices + k, uv_indices + k + n);
        k += static_cast<int32_t>(n);
    }
}

CFUSD_API void cfusd_mesh_set_normals(
    cfusd_mesh_t *mesh, int32_t normal_count, const double *nxyz,
    int32_t index_count, const int32_t *normal_indices)
{
    mesh->data.VN.resize(normal_count);
    for (int32_t i = 0; i < normal_count; ++i)
    {
        mesh->data.VN[i] = {nxyz[3 * i], nxyz[3 * i + 1], nxyz[3 * i + 2]};
    }
    mesh->data.FN.resize(mesh->data.F.size());
    int32_t k = 0;
    for (size_t f = 0; f < mesh->data.F.size() && k < index_count; ++f)
    {
        const size_t n = mesh->data.F[f].size();
        mesh->data.FN[f].assign(normal_indices + k, normal_indices + k + n);
        k += static_cast<int32_t>(n);
    }
}

CFUSD_API void cfusd_mesh_set_mtl_filename(cfusd_mesh_t *mesh, const char *mtl_filename)
{
    mesh->data.mtl_filename = (mtl_filename != nullptr) ? mtl_filename : "";
}

CFUSD_API void cfusd_mesh_add_group(
    cfusd_mesh_t *mesh, const char *object_name, const char *group_name,
    const char *material_name, int32_t face_count, const int32_t *face_indices)
{
    const std::string oname = (object_name != nullptr) ? object_name : "";
    OBJObject *obj = nullptr;
    if (!mesh->data.objects.empty() && mesh->data.objects.back().name == oname)
    {
        obj = &mesh->data.objects.back();
    }
    else
    {
        OBJObject o;
        o.name = oname;
        mesh->data.objects.push_back(o);
        obj = &mesh->data.objects.back();
    }
    OBJGroup g;
    g.name = (group_name != nullptr) ? group_name : "";
    g.material_name = (material_name != nullptr) ? material_name : "";
    g.face_indices.assign(face_indices, face_indices + face_count);
    obj->groups.push_back(g);
}

// --- getters --------------------------------------------------------------

CFUSD_API int32_t cfusd_mesh_get_vertex_count(const cfusd_mesh_t *mesh)
{
    return static_cast<int32_t>(mesh->data.V.size());
}

CFUSD_API void cfusd_mesh_get_positions(const cfusd_mesh_t *mesh, double *out_xyz)
{
    for (size_t i = 0; i < mesh->data.V.size(); ++i)
    {
        const auto &v = mesh->data.V[i];
        out_xyz[3 * i] = v.size() > 0 ? v[0] : 0.0;
        out_xyz[3 * i + 1] = v.size() > 1 ? v[1] : 0.0;
        out_xyz[3 * i + 2] = v.size() > 2 ? v[2] : 0.0;
    }
}

CFUSD_API int32_t cfusd_mesh_get_face_count(const cfusd_mesh_t *mesh)
{
    return static_cast<int32_t>(mesh->data.F.size());
}

CFUSD_API int32_t cfusd_mesh_get_index_count(const cfusd_mesh_t *mesh)
{
    int32_t n = 0;
    for (const auto &f : mesh->data.F)
    {
        n += static_cast<int32_t>(f.size());
    }
    return n;
}

CFUSD_API void cfusd_mesh_get_faces(
    const cfusd_mesh_t *mesh, int32_t *out_counts, int32_t *out_indices)
{
    int32_t k = 0;
    for (size_t f = 0; f < mesh->data.F.size(); ++f)
    {
        out_counts[f] = static_cast<int32_t>(mesh->data.F[f].size());
        for (int idx : mesh->data.F[f])
        {
            out_indices[k++] = idx;
        }
    }
}

CFUSD_API int32_t cfusd_mesh_get_uv_count(const cfusd_mesh_t *mesh)
{
    return static_cast<int32_t>(mesh->data.VT.size());
}

CFUSD_API void cfusd_mesh_get_uvs(const cfusd_mesh_t *mesh, double *out_uv, int32_t *out_uv_indices)
{
    for (size_t i = 0; i < mesh->data.VT.size(); ++i)
    {
        const auto &t = mesh->data.VT[i];
        out_uv[2 * i] = t.size() > 0 ? t[0] : 0.0;
        out_uv[2 * i + 1] = t.size() > 1 ? t[1] : 0.0;
    }
    int32_t k = 0;
    for (const auto &face : mesh->data.FT)
    {
        for (int idx : face)
        {
            out_uv_indices[k++] = idx;
        }
    }
}

CFUSD_API int32_t cfusd_mesh_get_normal_count(const cfusd_mesh_t *mesh)
{
    return static_cast<int32_t>(mesh->data.VN.size());
}

CFUSD_API void cfusd_mesh_get_normals(const cfusd_mesh_t *mesh, double *out_nxyz, int32_t *out_normal_indices)
{
    for (size_t i = 0; i < mesh->data.VN.size(); ++i)
    {
        const auto &n = mesh->data.VN[i];
        out_nxyz[3 * i] = n.size() > 0 ? n[0] : 0.0;
        out_nxyz[3 * i + 1] = n.size() > 1 ? n[1] : 0.0;
        out_nxyz[3 * i + 2] = n.size() > 2 ? n[2] : 0.0;
    }
    int32_t k = 0;
    for (const auto &face : mesh->data.FN)
    {
        for (int idx : face)
        {
            out_normal_indices[k++] = idx;
        }
    }
}

CFUSD_API void cfusd_mesh_get_mtl_filename(const cfusd_mesh_t *mesh, char *out, int32_t cap)
{
    copy_str(mesh->data.mtl_filename, out, cap);
}

CFUSD_API int32_t cfusd_mesh_get_group_count(const cfusd_mesh_t *mesh)
{
    int32_t n = 0;
    for (const OBJObject &obj : mesh->data.objects)
    {
        n += static_cast<int32_t>(obj.groups.size());
    }
    return n;
}

CFUSD_API void cfusd_mesh_get_group_names(
    const cfusd_mesh_t *mesh, int32_t group,
    char *out_object, int32_t object_cap,
    char *out_group, int32_t group_cap,
    char *out_material, int32_t material_cap)
{
    const OBJObject *owner = nullptr;
    const OBJGroup *g = group_at(mesh->data, group, &owner);
    if (g == nullptr)
    {
        return;
    }
    copy_str(owner->name, out_object, object_cap);
    copy_str(g->name, out_group, group_cap);
    copy_str(g->material_name, out_material, material_cap);
}

CFUSD_API int32_t cfusd_mesh_get_group_face_count(const cfusd_mesh_t *mesh, int32_t group)
{
    const OBJGroup *g = group_at(mesh->data, group, nullptr);
    return g != nullptr ? static_cast<int32_t>(g->face_indices.size()) : 0;
}

CFUSD_API void cfusd_mesh_get_group_faces(const cfusd_mesh_t *mesh, int32_t group, int32_t *out_face_indices)
{
    const OBJGroup *g = group_at(mesh->data, group, nullptr);
    if (g == nullptr)
    {
        return;
    }
    for (size_t i = 0; i < g->face_indices.size(); ++i)
    {
        out_face_indices[i] = g->face_indices[i];
    }
}

// --- I/O ------------------------------------------------------------------

CFUSD_API int32_t cfusd_write_usd(const cfusd_mesh_t *mesh, const char *path)
{
    return polyfem::io::USDWriter::write_with_groups(path, mesh->data) ? 1 : 0;
}

CFUSD_API cfusd_mesh_t *cfusd_read_usd(const char *path)
{
    cfusd_mesh_t *mesh = new cfusd_mesh();
    if (!polyfem::io::USDReader::read_with_groups(path, mesh->data))
    {
        delete mesh;
        return nullptr;
    }
    return mesh;
}

} // extern "C"
