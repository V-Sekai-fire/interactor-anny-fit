#pragma once

// Ensures the OpenUSD plugin registry (file-format + schema plugins: usda/usdc
// readers, UsdGeom, UsdShade, ...) is discoverable before any stage is opened.
//
// The prebuilt runtime ships its plugins under <usd_root>/lib/usd; that path is
// baked in at build time as POLYFEM_USD_PLUGIN_PATH (see the top CMakeLists.txt,
// fed from StageRuntime.root/0). Registering it explicitly means we do not rely
// on an ambient PXR_PLUGINPATH_NAME being set in the environment.

#include <mutex>

#include <pxr/base/plug/registry.h>

namespace polyfem::io::usd_detail
{
	inline void ensure_plugins_registered()
	{
#ifdef POLYFEM_USD_PLUGIN_PATH
		static std::once_flag once;
		std::call_once(once, []() {
			PXR_NS::PlugRegistry::GetInstance().RegisterPlugins(POLYFEM_USD_PLUGIN_PATH);
		});
#endif
	}
} // namespace polyfem::io::usd_detail
