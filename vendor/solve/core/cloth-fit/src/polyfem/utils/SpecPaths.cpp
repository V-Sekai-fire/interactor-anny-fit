#include "SpecPaths.hpp"

#include <cstdlib>
#include <fstream>

#ifndef POLYFEM_EMBEDDED_SPECS
#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#endif

#ifndef POLYFEM_JSON_SPEC_DIR
#define POLYFEM_JSON_SPEC_DIR ""
#endif
#ifndef POLYSOLVE_JSON_SPEC_DIR
#define POLYSOLVE_JSON_SPEC_DIR ""
#endif
#ifndef POLYFEM_INPUT_SPEC
#define POLYFEM_INPUT_SPEC ""
#endif

namespace polyfem::utils
{
#ifdef POLYFEM_EMBEDDED_SPECS
	// Embedded builds (fit.elf, fit_native) carry their specs inside the binary
	// (guest/fit/specs): no module path lookup (dladdr), no probing of the
	// build machine's directories.
	std::string module_dir() { return {}; }
	std::vector<std::string> spec_dirs() { return {}; }
	std::string input_spec_path() { return {}; }
#else
	namespace
	{
		std::string parent_of(const std::string &path)
		{
			const size_t cut = path.find_last_of("/\\");
			return cut == std::string::npos ? std::string() : path.substr(0, cut);
		}

		bool readable(const std::string &path)
		{
			std::ifstream f(path);
			return f.good();
		}
	}

	std::string module_dir()
	{
#if defined(_WIN32)
		HMODULE handle = nullptr;
		if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
									| GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
								reinterpret_cast<LPCSTR>(&module_dir), &handle))
			return {};
		char buffer[MAX_PATH] = {0};
		const DWORD n = GetModuleFileNameA(handle, buffer, MAX_PATH);
		if (n == 0 || n >= MAX_PATH)
			return {};
		return parent_of(std::string(buffer, n));
#else
		Dl_info info{};
		if (dladdr(reinterpret_cast<const void *>(&module_dir), &info) == 0 || !info.dli_fname)
			return {};
		return parent_of(info.dli_fname);
#endif
	}

	std::vector<std::string> spec_dirs()
	{
		std::vector<std::string> dirs;
		if (const char *env = std::getenv("POLYFEM_JSON_SPEC_DIR"))
			if (*env)
				dirs.emplace_back(env);

		const std::string self = module_dir();
		if (!self.empty())
		{
			dirs.push_back(self + "/json-specs");
			dirs.push_back(self);
		}

		for (const char *baked : {POLYFEM_JSON_SPEC_DIR, POLYSOLVE_JSON_SPEC_DIR})
			if (baked && *baked)
				dirs.emplace_back(baked);
		return dirs;
	}

	std::string input_spec_path()
	{
		for (const std::string &dir : spec_dirs())
			if (readable(dir + "/input-spec.json"))
				return dir + "/input-spec.json";
		return POLYFEM_INPUT_SPEC;
	}
#endif
}
