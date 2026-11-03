#!/usr/bin/env bash
# Regenerate the LBS Slang kernel, compile it to the CPU (C++) target, build the
# host harness, and run it.  Verifies Sinew.SlangCodegen.Lbs end to end on CPU.
set -e
here="$(cd "$(dirname "$0")" && pwd)"
spec="$here/../.."

(cd "$spec" && lake exe emit_shaders slang)
slangc "$spec/slang/lbs.slang" -entry lbs -stage compute -target cpp -o "$here/lbs.gen.cpp"
g++ -std=c++17 "$here/test.cpp" -o "$here/lbs_cpu_test"
"$here/lbs_cpu_test"
