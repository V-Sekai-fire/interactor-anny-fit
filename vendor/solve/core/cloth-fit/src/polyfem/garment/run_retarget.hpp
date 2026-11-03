#pragma once
#include <string>

namespace polyfem::garment
{
    // Run a full garment retarget from a setup JSON config, writing per-step
    // output into output_path in the config's output.format ("obj" or "usd").
    // Returns 0 on success (result JSON in result_out); non-zero sets error_out.
    int run_retarget(const std::string &config, const std::string &output_path,
                     std::string &result_out, std::string &error_out);
}
