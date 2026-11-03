#pragma once

#include <string>

namespace cfusd_loader
{
    /// @brief Load the cloth_fit_usd bridge shared lib and bind the C-ABI dispatch
    /// table (POSIX: dlopen + dlsym via the generated umbrella; Windows: the
    /// delay-load imports bind on first call). Idempotent. Returns true on success.
    /// @param bridge_path full path to the bridge lib (libcloth_fit_usd.so / .dylib
    ///   / cloth_fit_usd.dll). On Windows an empty path relies on the search order.
    bool load(const std::string &bridge_path);

    /// @brief Whether the bridge has been loaded (its symbols are callable).
    bool loaded();

    /// @brief Convenience: load the bridge from $CFUSD_BRIDGE and register the USD
    /// plugin tree from $CFUSD_PLUGIN_DIR (both optional). Call once before USD I/O.
    /// Returns true if the bridge is usable.
    bool load_from_env();
} // namespace cfusd_loader
