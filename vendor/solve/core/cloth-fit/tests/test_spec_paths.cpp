////////////////////////////////////////////////////////////////////////////////
#include <polyfem/utils/SpecPaths.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
////////////////////////////////////////////////////////////////////////////////

using namespace polyfem::utils;

namespace
{
	bool is_baked(const std::string &dir)
	{
		return dir == std::string(POLYFEM_JSON_SPEC_DIR)
			   || dir == std::string(POLYSOLVE_JSON_SPEC_DIR);
	}

	size_t first_baked(const std::vector<std::string> &dirs)
	{
		for (size_t i = 0; i < dirs.size(); ++i)
			if (is_baked(dirs[i]))
				return i;
		return dirs.size();
	}
}

TEST_CASE("spec dirs know where the library lives", "[spec_paths]")
{
	// The library's own location is the only fixed point a shipped binary has.
	REQUIRE_FALSE(module_dir().empty());
	REQUIRE(!spec_dirs().empty());
}

TEST_CASE("the compiled-in spec paths come last", "[spec_paths]")
{
	// FALSIFICATION. This is the defect itself, not a refinement of it. The
	// compiled-in paths name the machine that built the binary -- D:/a/cloth-fit
	// on a Windows runner -- and exist on no other machine, so a binary that
	// consults them first is unusable everywhere it is shipped to. A test that
	// only asserted input_spec_path() is readable would pass on the build
	// machine, which is the one place the bug cannot be observed.
	const std::vector<std::string> dirs = spec_dirs();
	const size_t baked = first_baked(dirs);

	REQUIRE(baked > 0);
	REQUIRE(baked < dirs.size());
	for (size_t i = baked; i < dirs.size(); ++i)
		REQUIRE(is_baked(dirs[i]));
}

TEST_CASE("a spec beside the library wins over the compiled-in one", "[spec_paths]")
{
	// PROPERTY: relocatability, measured rather than asserted. A spec written
	// next to the library must be the one chosen.
	const std::string dir = module_dir() + "/json-specs";
	const std::string path = dir + "/input-spec.json";
	const bool pre_existing = std::ifstream(path).good();

	if (!pre_existing)
	{
		std::filesystem::create_directories(dir);
		std::ofstream(path) << "{}";
	}

	REQUIRE(input_spec_path() == path);

	if (!pre_existing)
		std::filesystem::remove(path);
}

TEST_CASE("the environment overrides everything", "[spec_paths]")
{
	// PROPERTY: an escape hatch for a layout nobody anticipated.
	const std::string dir = std::string(POLYFEM_TEST_DIR) + "/spec-override";
	std::filesystem::create_directories(dir);
	std::ofstream(dir + "/input-spec.json") << "{}";

#if defined(_WIN32)
	_putenv_s("POLYFEM_JSON_SPEC_DIR", dir.c_str());
#else
	setenv("POLYFEM_JSON_SPEC_DIR", dir.c_str(), 1);
#endif
	REQUIRE(spec_dirs().front() == dir);
	REQUIRE(input_spec_path() == dir + "/input-spec.json");
#if defined(_WIN32)
	_putenv_s("POLYFEM_JSON_SPEC_DIR", "");
#else
	unsetenv("POLYFEM_JSON_SPEC_DIR");
#endif
	std::filesystem::remove_all(dir);
}
