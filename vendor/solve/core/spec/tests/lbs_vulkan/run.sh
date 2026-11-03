#!/usr/bin/env bash
# Compile the Lean LBS kernel to SPIR-V, build the headless Vulkan compute host
# (volk + Vulkan-Headers, both vendored), and run it on the GPU.  Verifies
# Sinew.SlangCodegen.Lbs on a real Vulkan device — no window.
set -e
here="$(cd "$(dirname "$0")" && pwd)"
spec="$here/../.."
root="$spec/.."

(cd "$spec" && lake exe emit_shaders slang)
slangc "$spec/slang/lbs.slang" -entry lbs -stage compute -fvk-use-entrypoint-name \
    -target spirv -o "$here/lbs.spv"

g++ -std=c++17 -DVK_NO_PROTOTYPES \
    -I"$root/../thirdparty/volk" -I"$root/../thirdparty/Vulkan-Headers/include" \
    "$here/vk_lbs.cpp" "$root/../thirdparty/volk/volk.c" -o "$here/vk_lbs"

"$here/vk_lbs" "$here/lbs.spv"        # 4-vertex hand-checked case
"$here/vk_lbs" "$here/lbs.spv" scale  # ANNY size (V=18056, J=77), GPU vs CPU
