#include "loader.hpp"

#include <cstdlib>

#include "cloth_fit_usd.h" // C ABI (cfusd_init) — resolved via the stub after load

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <dlfcn.h>
#  include "cfusd/cloth_fit_usd_stubs.h" // generated POSIX dlsym table (cfusd::)
#endif

namespace cfusd_loader
{
    namespace
    {
        bool g_loaded = false;
    } // namespace

    bool loaded()
    {
        return g_loaded;
    }

    bool load(const std::string &bridge_path)
    {
        if (g_loaded)
        {
            return true;
        }
#ifdef _WIN32
        // LOAD_WITH_ALTERED_SEARCH_PATH so the bridge's own deps (usd_ms.dll,
        // tbb12.dll) resolve from the same dir; the delay-load thunks then bind
        // to this already-loaded module.
        if (!bridge_path.empty())
        {
            HMODULE h = LoadLibraryExA(bridge_path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
            g_loaded = (h != nullptr);
        }
        else
        {
            g_loaded = true; // rely on the OS loader / delay-load search
        }
#else
        cfusd::StubPathMap paths;
        paths[cfusd::kModuleCloth_fit_usd] = {bridge_path};
        g_loaded = cfusd::InitializeStubs(paths);
#endif
        return g_loaded;
    }

    // Resolve the bridge lib sitting next to the consumer module (the NIF / binary
    // that links this stub), so no CFUSD_BRIDGE env handoff is needed. On Windows
    // this is essential: the bridge is delay-loaded, so load() must LoadLibraryEx
    // its full path with LOAD_WITH_ALTERED_SEARCH_PATH for libusd_ms.dll + tbb to
    // resolve from the same dir before the delay thunks bind.
    static std::string bridge_next_to_self()
    {
#ifdef _WIN32
        HMODULE hm = nullptr;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                   GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCSTR>(&load), &hm) != 0)
        {
            char buf[MAX_PATH] = {0};
            if (GetModuleFileNameA(hm, buf, MAX_PATH) != 0)
            {
                std::string self(buf);
                const auto slash = self.find_last_of("\\/");
                const std::string dir = (slash == std::string::npos) ? std::string(".") : self.substr(0, slash);
                return dir + "\\libcloth_fit_usd.dll";
            }
        }
#else
        Dl_info info;
        if (dladdr(reinterpret_cast<void *>(&load), &info) != 0 && info.dli_fname != nullptr)
        {
            std::string self = info.dli_fname;
            const auto slash = self.find_last_of('/');
            const std::string dir = (slash == std::string::npos) ? std::string(".") : self.substr(0, slash);
#  ifdef __APPLE__
            return dir + "/libcloth_fit_usd.dylib";
#  else
            return dir + "/libcloth_fit_usd.so";
#  endif
        }
#endif
        return "";
    }

    bool load_from_env()
    {
        std::string bridge;
        const char *env = std::getenv("CFUSD_BRIDGE");
        if (env != nullptr && env[0] != '\0')
        {
            bridge = env;
        }
        else
        {
            bridge = bridge_next_to_self();
        }
        if (!load(bridge))
        {
            return false;
        }
        const char *plugin_dir = std::getenv("CFUSD_PLUGIN_DIR");
        cfusd_init(plugin_dir); // NULL -> the bridge's compiled-in default
        return true;
    }
} // namespace cfusd_loader
