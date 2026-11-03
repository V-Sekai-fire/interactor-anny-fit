# solve — sinew-mocap

[![CI](https://github.com/sinew-mocap/solve/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/sinew-mocap/solve/actions/workflows/ci.yml)

A hexagon cluster (`core/` + `ports/` + `adapters/`) of the Sinew mocap stack:
the body solve — FK + linear-blend skinning of the ANNY body (Lean->Slang kernels).

## Build

```
cmake -B build && cmake --build build
# tests: cmake -B build -DSINEW_TESTS=ON && cmake --build build && ctest --test-dir build
```

## Dependencies

See `ports/sibling-repos.txt` — clone the listed `sinew-mocap` repos side-by-side in `~/`.


## Vendored solvers

All three come from archived upstreams and are owned here as git subtrees (squashed). This repo is their source of truth; refresh a path only deliberately.

| Path | Source | Ref taken | What it is |
|---|---|---|---|
| `core/rotation-star/` | `V-Sekai-archive/contract-rotation-star` | `main` @ `0c3830c` | Rotation solver: Lean spec and proofs, Slang kernels (SPIR-V, C++), C port |
| `core/lbfgsb/` | `V-Sekai-archive/contract-lbfgsb` | `main` @ `449bb7d` | L-BFGS-B in Lean emitted through Slang to CPU and SPIR-V, with the guest driver; the bounded solver for mesh-surface fits on the GPU |
| `core/cloth-fit/` | `V-Sekai-archive/cloth-fit` | `main` @ `d2dbbbd` | Intersection-free garment retargeting (SIGGRAPH 2025, PolyFEM-based C++): fits a garment onto a new body |

Probe a link with `git subtree pull --prefix=<path> <source url> main --squash`; it answers `Subtree is already at commit <sha>`.

Check each package: `cd core/rotation-star/lean && lake build && lake exe rotation_star_test`, `cd core/lbfgsb/lean && lake build`, and `core/cloth-fit` with its own `pixi.toml` and CMake build (see its README).
