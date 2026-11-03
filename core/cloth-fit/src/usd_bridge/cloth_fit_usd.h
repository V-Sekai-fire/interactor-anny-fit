// cloth_fit_usd — engine-agnostic C ABI for reading/writing meshes as OpenUSD.
//
// All public symbols are extern "C" with primitive / opaque-handle types only:
// no pxr, no C++ STL in the API surface. cloth-fit's solver (libpolyfem) and NIF
// call these through generate_stubs.py-emitted dlopen stubs, so they link ZERO
// OpenUSD; only the cloth_fit_usd bridge shared lib links usd_ms. Mirrors the
// idtx_core port pattern in fabric-flow-adapters.

#ifndef CLOTH_FIT_USD_H
#define CLOTH_FIT_USD_H

#include <stddef.h>
#include <stdint.h>

// Three build configs (as idtx_core): building the shared lib -> export; consuming
// it -> import (Windows) / hidden default (ELF/Mach-O); static -> no decoration.
#if defined(CFUSD_STATIC)
#  define CFUSD_API
#elif defined(_WIN32)
#  ifdef CFUSD_BUILDING_DLL
#    define CFUSD_API __declspec(dllexport)
#  else
#    define CFUSD_API __declspec(dllimport)
#  endif
#else
#  define CFUSD_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Opaque mesh handle; layout is private to the bridge.
typedef struct cfusd_mesh cfusd_mesh_t;

// NUL-terminated version string (stable pointer; do not free).
CFUSD_API const char* cfusd_version(void);

// Register the OpenUSD plugin registry so stages can open. Pass the lib/usd dir,
// or NULL to resolve it relative to this bridge module. Returns 1 ok / 0 fail.
CFUSD_API int32_t cfusd_init(const char* plugin_dir);

CFUSD_API cfusd_mesh_t* cfusd_mesh_create(void);
CFUSD_API void cfusd_mesh_destroy(cfusd_mesh_t* mesh);

// --- setters: build a mesh to write ---------------------------------------
// Positions: vertex_count * 3 doubles (x,y,z).
CFUSD_API void cfusd_mesh_set_positions(cfusd_mesh_t* mesh, int32_t vertex_count, const double* xyz);
// Topology: per-face vertex counts (face_count) + flattened vertex indices (index_count).
CFUSD_API void cfusd_mesh_set_faces(cfusd_mesh_t* mesh, int32_t face_count, const int32_t* face_vertex_counts, int32_t index_count, const int32_t* face_vertex_indices);
// UVs: uv_count * 2 doubles + per-corner indices (index_count) into them.
CFUSD_API void cfusd_mesh_set_uvs(cfusd_mesh_t* mesh, int32_t uv_count, const double* uv, int32_t index_count, const int32_t* uv_indices);
// Normals: normal_count * 3 doubles + per-corner indices (index_count) into them.
CFUSD_API void cfusd_mesh_set_normals(cfusd_mesh_t* mesh, int32_t normal_count, const double* nxyz, int32_t index_count, const int32_t* normal_indices);
CFUSD_API void cfusd_mesh_set_mtl_filename(cfusd_mesh_t* mesh, const char* mtl_filename);
// A group (OBJ o/g/usemtl) of faces; call once per group, in order.
CFUSD_API void cfusd_mesh_add_group(cfusd_mesh_t* mesh, const char* object_name, const char* group_name, const char* material_name, int32_t face_count, const int32_t* face_indices);

// --- getters: read a mesh (two-call: get count, then fill caller buffer) ---
CFUSD_API int32_t cfusd_mesh_get_vertex_count(const cfusd_mesh_t* mesh);
CFUSD_API void cfusd_mesh_get_positions(const cfusd_mesh_t* mesh, double* out_xyz);
CFUSD_API int32_t cfusd_mesh_get_face_count(const cfusd_mesh_t* mesh);
CFUSD_API int32_t cfusd_mesh_get_index_count(const cfusd_mesh_t* mesh);
CFUSD_API void cfusd_mesh_get_faces(const cfusd_mesh_t* mesh, int32_t* out_face_vertex_counts, int32_t* out_face_vertex_indices);
CFUSD_API int32_t cfusd_mesh_get_uv_count(const cfusd_mesh_t* mesh);
CFUSD_API void cfusd_mesh_get_uvs(const cfusd_mesh_t* mesh, double* out_uv, int32_t* out_uv_indices);
CFUSD_API int32_t cfusd_mesh_get_normal_count(const cfusd_mesh_t* mesh);
CFUSD_API void cfusd_mesh_get_normals(const cfusd_mesh_t* mesh, double* out_nxyz, int32_t* out_normal_indices);
CFUSD_API void cfusd_mesh_get_mtl_filename(const cfusd_mesh_t* mesh, char* out, int32_t cap);
CFUSD_API int32_t cfusd_mesh_get_group_count(const cfusd_mesh_t* mesh);
CFUSD_API void cfusd_mesh_get_group_names(const cfusd_mesh_t* mesh, int32_t group, char* out_object, int32_t object_cap, char* out_group, int32_t group_cap, char* out_material, int32_t material_cap);
CFUSD_API int32_t cfusd_mesh_get_group_face_count(const cfusd_mesh_t* mesh, int32_t group);
CFUSD_API void cfusd_mesh_get_group_faces(const cfusd_mesh_t* mesh, int32_t group, int32_t* out_face_indices);

// --- I/O ------------------------------------------------------------------
// Write the mesh to a USD file (extension selects the format). 1 ok / 0 fail.
CFUSD_API int32_t cfusd_write_usd(const cfusd_mesh_t* mesh, const char* path);
// Read a USD file into a new mesh handle (caller destroys). NULL on failure.
CFUSD_API cfusd_mesh_t* cfusd_read_usd(const char* path);

#ifdef __cplusplus
}
#endif

#endif  // CLOTH_FIT_USD_H
