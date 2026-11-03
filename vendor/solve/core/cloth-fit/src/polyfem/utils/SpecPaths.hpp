#pragma once

#include <string>
#include <vector>

namespace polyfem::utils
{
	/// Directory holding the library this code is linked into, empty if it
	/// cannot be determined. A shipped binary is unpacked wherever the consumer
	/// puts it, so its own location is the only fixed point it has.
	std::string module_dir();

	/// Directories to search for JSON spec files, most specific first:
	/// $POLYFEM_JSON_SPEC_DIR, then json-specs/ beside the library, then the
	/// library's own directory, then the paths compiled in at build time.
	///
	/// The compiled-in paths are last rather than first because they name the
	/// machine that built the binary -- D:/a/cloth-fit on a Windows runner,
	/// /home/runner/work on a Linux one -- and exist nowhere else. A binary that
	/// consults them first works only where it was built.
	std::vector<std::string> spec_dirs();

	/// First readable `input-spec.json` across spec_dirs(), or the compiled-in
	/// path when none is readable, so the caller still reports a real filename.
	std::string input_spec_path();
}
