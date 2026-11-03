#!/usr/bin/env bash
# The drape kernels, from Lean to both targets (the kernels/avbd/gen.sh shape).
#
#   kernels/drape/gen.sh                 # emit from Lean, then cpp + spirv + table + embed
#   kernels/drape/gen.sh --no-emit       # use the committed slang/ (no lake)
#   kernels/drape/gen.sh --from <dir>    # use .slang already emitted into <dir>
#
#   Lean (lean/, `lake exe emit_drape`)                    ->  slang/<k>.slang  (committed)
#     slangc -target cpp    ->  cpp/<k>_emit.cpp                                 (committed)
#     slangc -target spirv  ->  <build>/spv-drape/<k>.spv + .refl.json           (build artefact)
#       ../avbd/gen_avbd_kernel_table.py --namespace drape_table
#                           ->  DrapeKernelTable.inc                             (committed)
#       ../embed_spv.py --namespace drape_kernels
#                           ->  <build>/drape_kernels.inc                        (build artefact)
#
# kernels.txt gives each kernel's targets: groupshared kernels are spirv-only
# (slangc -target cpp rejects GroupMemoryBarrierWithGroupSync), and their
# one-thread *_serial siblings are cpp-only. The SPIR-V goes to its own
# spv-drape/ so the AVBD embed (which takes every .spv in spv/) is unchanged.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
RUNTIME="${GUEST_RUNTIME_ROOT:-$(cd "$ROOT/../../2-contract/guest-runtime" && pwd)}"
LEAN="${CLOTH_LEAN:-$ROOT/lean}"
BUILD="${BUILD_DIR:-$ROOT/build}"
SPV="$BUILD/spv-drape"
SLANGC="${SLANGC:-slangc}"
command -v "$SLANGC" >/dev/null 2>&1 || SLANGC="$HOME/scoop/apps/vulkan/current/Bin/slangc"

MODE=emit
FROM=""
case "${1:-}" in
	--no-emit) MODE=none ;;
	--from) MODE=from; FROM="$2" ;;
	"") ;;
	*) echo "unknown option: $1" >&2; exit 2 ;;
esac

ALL=$(grep -v '^#' "$HERE/kernels.txt" | awk 'NF {print $1}' | tr '\n' ' ')
CPP=$(grep -v '^#' "$HERE/kernels.txt" | awk '$2 == "both" || $2 == "cpp" {print $1}' | tr '\n' ' ')
SPIRV=$(grep -v '^#' "$HERE/kernels.txt" | awk '$2 == "both" || $2 == "spirv" {print $1}' | tr '\n' ' ')

if [ "$MODE" = emit ]; then
	command -v lake >/dev/null 2>&1 || { echo "error: lake not on PATH (or pass --no-emit)" >&2; exit 1; }
	FROM="$(mktemp -d)"
	echo "== emitting Slang from Lean at $LEAN =="
	( cd "$LEAN" && lake exe emit_drape "$FROM" >/dev/null )
fi
if [ -n "$FROM" ]; then
	mkdir -p "$HERE/slang"
	for k in $ALL; do
		cp "$FROM/$k.slang" "$HERE/slang/$k.slang"
	done
	echo "== $(echo $ALL | wc -w) kernels into slang/ =="
fi

mkdir -p "$HERE/cpp" "$SPV"
echo "== slangc -target cpp ($(echo $CPP | wc -w)) =="
# Relative paths from $HERE: slangc writes the input path into a #line
# directive, and an absolute one would make the committed cpp depend on
# where the checkout (or worktree) lives.
for k in $CPP; do
	( cd "$HERE" && "$SLANGC" -target cpp -stage compute -entry main -o "cpp/${k}_emit.cpp" "slang/$k.slang" )
done
# -fp-mode precise decorates every float op NoContraction: the driver may not
# fuse a*b+c into an FMA, so the GPU rounds in the order the cpp path does
# (the guest builds vec_cpu.cpp -ffp-contract=off). The Cauchy sweep of a
# tight box amplifies that rounding: fixture comp_03's xcp sat 1.4e-4 from
# LBFGSpp's on the GPU without it, 6.6e-6 on the uncontracted CPU
# (gates/5-drape/lbfgsb/README.md).
echo "== slangc -target spirv (+ reflection) ($(echo $SPIRV | wc -w)) =="
rm -f "$SPV"/*.spv "$SPV"/*.refl.json
for k in $SPIRV; do
	"$SLANGC" -target spirv -profile sm_6_5 -stage compute -entry main -fp-mode precise \
		-reflection-json "$SPV/$k.refl.json" -o "$SPV/$k.spv" "$HERE/slang/$k.slang"
done
echo "== binding table =="
python "$RUNTIME/kernels/avbd/gen_avbd_kernel_table.py" --namespace drape_table --build-dir "$SPV" \
	--out "$HERE/DrapeKernelTable.inc" $SPIRV
echo "== embedding SPIR-V =="
python "$RUNTIME/kernels/embed_spv.py" --namespace drape_kernels "$SPV" "$BUILD/drape_kernels.inc"
