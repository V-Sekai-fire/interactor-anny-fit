#ifndef WEFTFIT_RETARGET_H
#define WEFTFIT_RETARGET_H

// C ABI for the weftfit garment-retargeting solve bridge. The SHARED library
// weftfit_retarget.cpp is the only unit that links libpolyfem + the garment core
// and exports these symbols; consumers dlopen it (dlsym stubs on POSIX, import
// lib on Windows) and link ZERO PolyFEM/TBB. Keep in sync with
// weftfit_retarget.sigs (the single source of truth for the exported ABI).

#include <stdint.h>

// Only the extern "C" surface is exported; PolyFEM/Eigen/TBB stay hidden. Set
// WFRT_BUILDING_DLL inside weftfit_retarget.cpp when building the bridge.
#if defined(_WIN32)
#  if defined(WFRT_BUILDING_DLL)
#    define WFRT_API __declspec(dllexport)
#  else
#    define WFRT_API __declspec(dllimport)
#  endif
#else
#  if defined(WFRT_BUILDING_DLL)
#    define WFRT_API __attribute__((visibility("default")))
#  else
#    define WFRT_API
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Bridge version string (static storage; do not free).
WFRT_API const char *wf_retarget_version(void);

// Run a full garment retarget from a setup JSON config, writing per-step output
// into output_dir in the config's output.format ("obj" or "usd"). Returns 0 on
// success, non-zero on failure.
WFRT_API int32_t wf_retarget_run(const char *config_json, const char *output_dir);

// Message for the most recent wf_retarget_run failure on this thread (static
// storage; valid until the next call). Empty string if none.
WFRT_API const char *wf_retarget_last_error(void);

// Result JSON from the most recent successful wf_retarget_run on this thread
// (static storage; valid until the next call). Empty string if the last run
// failed.
WFRT_API const char *wf_retarget_last_result(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WEFTFIT_RETARGET_H
