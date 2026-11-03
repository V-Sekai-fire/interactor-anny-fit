# contract-rotation-star

The workspace's rotation solver, the rotational star: its Lean specification and proofs, its Slang kernels for the GPU and the CPU, and a C port.

## What it is for

A frame is a star of 7 points: its origin and the ends of its 6 unit axes, ±X, ±Y and ±Z. A rotation is recovered by matching two stars as point sets with Kabsch: the SVD of their cross-covariance, by one-sided Jacobi. When the covariance has rank 1, only one direction is pinned, and the solver completes the frame with the shortest arc, so it adds no twist. The swing-twist split uses the same completion: the swing is the shortest arc from the axis to its image, and the twist is the swing's transpose times the rotation. Rotations stay 3×3 matrices throughout. No quaternion or Euler angle is formed.

| Part | Where |
|---|---|
| Star, Kabsch fit, completion, swing-twist split | `lean/RotationStar/{Vec,Completion,Kabsch,SwingTwist}.lean` |
| sinew's vector-alignment recipe (Rodrigues, Newton–Schulz, Jacobi fallback) | `lean/RotationStar/Align.lean` |
| Proofs: the star's centroid is its origin, its cross-covariance is exactly 2 R Qᵀ; `native_decide` pins of exact recovery on the 24 signed-permutation frames | `lean/RotationStar/Spec.lean` |
| Plausible properties, each with a planted control that must be refuted | `lean/RotationStar/Props.lean`, `lake exe rotation_star_test` |
| Slang emitters, `native_decide` pins of the emitted text | `lean/RotationStar/Slang/` |
| Emitted kernels `star_kabsch`, `swing_twist` (Slang, C++, SPIR-V) | `kernels/` |
| C port with its unit test against the Lean oracle | `c/` |

contract-anny-kernels takes its swing-twist split from `RotationStar.Slang.Split.swingTwistAngle`.

## Build

```sh
cd lean && lake build && lake exe rotation_star_test   # proofs, pins, properties and controls
tests/oracle/gen.sh --check                             # the committed oracle is Lean's
kernels/gen.sh --check                                  # the committed kernels are Lean's
c/build.sh                                              # C port against the oracle
tests/kernels/build.sh                                  # C++ emission against the oracle
GODOT=<godot 4> tests/kernels/gpu.sh                    # SPIR-V on a RenderingDevice, in a window
```

`kernels/gen.sh` needs `slangc` 2026.13. The committed C++ emits include `slang-cpp-prelude.h`; `tests/kernels/build.sh` finds it in the `include/` folder of that slang install, or in `SLANG_INCLUDE`. After a deliberate change to a kernel, run `elixir kernels/pin.exs --clear`, then `kernels/gen.sh`, then `elixir kernels/pin.exs`. Every test also runs its controls: a perturbed star, a twisting completion, a wrong axis, a moved oracle entry. The test fails if a control passes.

## Licence

MIT. See [LICENSE](LICENSE). `CITATION.cff` names the sources the solver is consolidated from.
