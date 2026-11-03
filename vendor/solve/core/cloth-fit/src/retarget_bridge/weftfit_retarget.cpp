// The weftfit_retarget SHARED bridge — the only unit that links libpolyfem + the
// garment core. It exports just the extern "C" WFRT_API surface (everything
// PolyFEM/Eigen/TBB stays hidden), so consumers dlopen it (dlsym stubs on POSIX,
// import lib on Windows) and link ZERO PolyFEM/TBB — the same design as the
// cloth_fit_usd bridge. Implementation is a thin wrapper over the garment core's
// run_retarget (the extracted NIF solve).

#define WFRT_BUILDING_DLL
#include "weftfit_retarget.h"

#include <polyfem/garment/run_retarget.hpp>

#include <string>

namespace
{
    // Message for the most recent wf_retarget_run failure on this thread.
    thread_local std::string g_last_error;
    // Result JSON from the most recent successful wf_retarget_run on this thread.
    thread_local std::string g_last_result;
} // namespace

extern "C" {

WFRT_API const char *wf_retarget_version(void)
{
    return "weftfit_retarget 0.1.0";
}

WFRT_API int32_t wf_retarget_run(const char *config_json, const char *output_dir)
{
    if (config_json == nullptr || output_dir == nullptr)
    {
        g_last_error = "wf_retarget_run: null config_json or output_dir";
        return 1;
    }
    std::string result;
    std::string error;
    const int rc = polyfem::garment::run_retarget(config_json, output_dir, result, error);
    g_last_error = (rc == 0) ? std::string() : error;
    g_last_result = (rc == 0) ? result : std::string();
    return rc;
}

WFRT_API const char *wf_retarget_last_error(void)
{
    return g_last_error.c_str();
}

WFRT_API const char *wf_retarget_last_result(void)
{
    return g_last_result.c_str();
}

} // extern "C"
