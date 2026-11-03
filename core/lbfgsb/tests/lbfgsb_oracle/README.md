# L-BFGS-B oracle generator (host-native)

`gen.cpp` drives unmodified LBFGSpp 0.3.0 (the solver cloth-dynamics'
`BackwardTaskSolver::optimizeLBFGS` uses) and writes text fixtures that the
Lean-emitted L-BFGS-B in drape.elf is checked against in Gate 5:

- `components/comp_00..19.txt`: one state pushed through `BFGSMat`,
  `Cauchy::get_cauchy_point` and `SubspaceMin::subspace_minimize` (G1).
- `problems/*.txt` and `traces/*.txt`: whole bounded problems and every
  iterate of the solver on them, with the diffcloth parameters (delta 1e-3,
  max_linesearch 20) and a tight variant (G2).

The format, coverage and results are documented next to the data, in
`gates/5-drape/oracle/README.md`.

## Run

```sh
tests/lbfgsb_oracle/build.sh                 # writes gates/5-drape/oracle/
OUT=/some/dir tests/lbfgsb_oracle/build.sh   # writes elsewhere, e.g. to diff -r
```

It clones `V-Sekai-fire/interactor-aria-lbfgspp` into `.deps/` (gitignored)
when it is missing, checks out `10086b6b`, and takes LBFGSpp
(`thirdparty/LBFGSpp/include`) and Eigen (`thirdparty/eigen`) from it; `EIGEN`
overrides the Eigen root. `core.longpaths` is set in that clone's own config
because Eigen's `doc/` exceeds MAX_PATH under a deep checkout. The compiler
defaults to the newest `~/llvm-mingw/llvm-mingw-*-ucrt-x86_64` clang++; `CXX`
overrides it. `gen.exe` lands in `.deps/`.

## Scope

LBFGSpp (MIT) and Eigen (MPL-2.0) are used only here, by a host-native test
tool. They are not linked into any guest ELF: the drape's L-BFGS-B is written
in Lean and emitted through Slang (AGENTS.md rules 2 and 3). `gen.cpp` includes
`LBFGSB.h` with `#define private public` so the headers stay byte-identical to
the ones cloth-dynamics ships.
