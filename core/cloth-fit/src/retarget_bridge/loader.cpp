// Consumer-side loader for the weftfit_retarget bridge. Mirrors the cloth_fit_usd
// loader: on Windows (llvm-mingw, no dlfcn) consumers bind wf_retarget_* via the
// bridge's delay-loaded import lib, and this loader LoadLibraryEx's the bridge from
// next-to-self so the delay thunks resolve; on POSIX the bridge is dlopen'd
// RTLD_GLOBAL so its exported wf_retarget_* satisfy the consumer's undefined refs.
// The bridge is self-contained (PolyFEM + garment core + TBB inside), so no ALTERED
// search path is needed for its deps — unlike cloth_fit_usd, which had to resolve
// usd_ms/tbb from priv/.

#include "wfrt_loader.hpp"

#include <cstdlib>
#include <string>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <dlfcn.h>
#  include "wfrt/weftfit_retarget_stubs.h" // generated POSIX dlsym table (wfrt::)
#endif

namespace wfrt_loader
{
    namespace
    {
        bool g_loaded = false;

        // Absolute path to libweftfit_retarget sitting next to THIS module (the
        // consumer that statically linked the loader). Uses the address of a local
        // symbol to find the containing module, so it works whether the consumer is
        // an .exe or a NIF .dll/.so.
        std::string bridge_next_to_self()
        {
#if defined(_WIN32)
            HMODULE hm = nullptr;
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   reinterpret_cast<LPCSTR>(&g_loaded), &hm) != 0)
            {
                char buf[MAX_PATH] = {0};
                if (GetModuleFileNameA(hm, buf, MAX_PATH) != 0)
                {
                    std::string self(buf);
                    const auto slash = self.find_last_of("\\/");
                    const std::string dir =
                        (slash == std::string::npos) ? std::string(".") : self.substr(0, slash);
                    return dir + "\\libweftfit_retarget.dll";
                }
            }
            return "";
#else
            Dl_info info;
            if (dladdr(reinterpret_cast<void *>(&g_loaded), &info) != 0 && info.dli_fname != nullptr)
            {
                std::string self(info.dli_fname);
                const auto slash = self.find_last_of('/');
                const std::string dir =
                    (slash == std::string::npos) ? std::string(".") : self.substr(0, slash);
#  if defined(__APPLE__)
                return dir + "/libweftfit_retarget.dylib";
#  else
                return dir + "/libweftfit_retarget.so";
#  endif
            }
            return "";
#endif
        }
    } // namespace

    bool loaded()
    {
        return g_loaded;
    }

    bool load_from_env()
    {
        if (g_loaded)
        {
            return true;
        }
        const char *env = std::getenv("WFRT_BRIDGE");
        std::string bridge = (env != nullptr && env[0] != '\0') ? std::string(env) : bridge_next_to_self();
        if (bridge.empty())
        {
            return false;
        }
#if defined(_WIN32)
        HMODULE h = LoadLibraryExA(bridge.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        g_loaded = (h != nullptr);
#else
        // POSIX: InitializeStubs dlopen's the bridge and dlsym's each wf_retarget_*
        // symbol into the generated weak-stub table, so the consumer's forwarders
        // dispatch through it (mirrors cfusd_loader).
        wfrt::StubPathMap paths;
        paths[wfrt::kModuleWeftfit_retarget] = {bridge};
        g_loaded = wfrt::InitializeStubs(paths);
#endif
        return g_loaded;
    }
} // namespace wfrt_loader
