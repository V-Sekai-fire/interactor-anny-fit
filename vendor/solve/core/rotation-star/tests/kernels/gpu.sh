#!/bin/sh
# Run the rotation-star kernels on the GPU (Godot RenderingDevice, in a window) and gate their
# outputs against the Lean oracle with the same test as the CPU path.
#
#   GODOT=<godot 4 binary> tests/kernels/gpu.sh
#
# Uses the committed kernels/spv. Godot runs windowed on purpose: the GPU path is tested as an
# application uses it, never --headless.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
GODOT=${GODOT:?set GODOT to a Godot 4 binary}
PROJ="$ROOT/build/rs_gpu_project"
IO="$ROOT/build/rs_gpu_io"
mkdir -p "$PROJ" "$IO"
"$HERE/build.sh" --write "$IO" >/dev/null
printf 'config_version=5\n[application]\nconfig/name="rotation_star_gpu"\n' > "$PROJ/project.godot"
cp "$HERE/gpu_run.gd" "$PROJ/gpu_run.gd"
RS_IN="$IO" RS_SPV="$ROOT/kernels/spv" "$GODOT" --path "$PROJ" --rendering-method forward_plus -s gpu_run.gd
"$ROOT/build/kernel_test/rs_kernel_test" "$ROOT/tests/oracle/oracle.txt" --gpu "$IO"
