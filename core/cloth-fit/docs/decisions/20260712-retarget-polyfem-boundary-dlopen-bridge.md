---
status: accepted
date: 2026-07-12
deciders: K. S. Ernest (iFire) Lee
consulted: —
informed: V-Sekai-fire/cloth-fit#2
---

# Decouple the retarget↔PolyFEM boundary with a `.sigs` / generate_stubs dlopen bridge

## Context and Problem Statement

cloth-fit ships as a self-contained Elixir Burrito CLI whose garment-retargeting
solve is PolyFEM. PolyFEM drags in a heavy static dependency stack (polysolve,
ipc-toolkit, tight-inclusion, scalable-ccd, simple_bvh, finitediff, jse,
predicates, spdlog, filib, OpenVDB, Abseil) plus a pinned **static** oneTBB. The
same tree also links OpenUSD (`usd_ms`), which carries its **own** TBB.

Every consumer that wants the solve — the NIF today, the planned `weftfit/cli`
and `obj`/`stage` adapters, a viewer, other language bindings — was forced to
link that entire stack. That is slow, couples each consumer to PolyFEM's build,
and puts two TBB instances on one link line (PolyFEM's static oneTBB + USD's
`usd_ms` TBB), which aborts at load. As part of decomposing cloth-fit
hexagonally into the `weftfit` cluster (see the manuals decision
`20260610-hexagonal-core-ports-adapters`), we needed a boundary where the
garment core is the *only* unit that links PolyFEM and consumers depend on a
narrow, stable surface instead.

How should the retarget↔PolyFEM boundary be structured so consumers get the
solve without linking PolyFEM/TBB/USD?

## Decision Drivers

* Consumers must link **zero** PolyFEM/TBB/USD — narrow ABI only.
* Keep USD's TBB isolated from PolyFEM's static oneTBB (no double-instance abort).
* Relocatable: the runtime artifact must load from Burrito's `priv/` with no
  system install and no `*_ROOT` handoff.
* One implementation of the solve, shared by all consumers (no divergence).
* Buildable on all three desktop toolchains: gcc (Linux), clang (macOS),
  llvm-mingw (Windows, `x86_64-windows-gnu`) — the last has **no `dlfcn.h`**.
* Reuse the mechanism already proven for USD I/O (the `cloth_fit_usd` bridge).

## Considered Options

* **A — Conventional shared library.** Build the garment core as a normal shared
  lib; consumers link its import lib directly (load-time) and `#include` its
  headers. No generated dispatch layer.
* **B — FetchContent the core into each consumer.** Each consumer's CMake
  `FetchContent`s cloth-fit with `POLYFEM_WITH_GARMENT=OFF` and rebuilds the
  garment sources as a local `weftfit_core` target (the `WEFTFIT_BUILD_CORE`
  path).
* **C — `.sigs` / generate_stubs dlopen C-ABI bridge.** A single SHARED library
  (`weftfit_retarget`) is the only unit that links PolyFEM + the garment core; it
  exports only an `extern "C"` surface declared in a `.sigs` contract
  (`wf_retarget_run` / `wf_retarget_last_error` / `wf_retarget_last_result` /
  `wf_retarget_version`), everything else hidden. `generate_stubs.py` turns the
  `.sigs` into a `dlsym` dispatch table (POSIX) / delay-loaded import lib
  (Windows); consumers link only a tiny loader stub + `dlopen` the bridge at
  runtime. Mirrors the existing `cloth_fit_usd` bridge one level up.

## Decision Outcome

Chosen option: **C**, ranked **C ≫ A > B**.

C is the only option that satisfies every driver: the consumer link line carries
zero PolyFEM/TBB/USD, TBB isolation falls out for free (the bridge `.dll/.so` is
the sole TBB carrier and, under USD, delay-loads `cloth_fit_usd` internally so
USD's TBB stays in *its* DLL), and it reuses the exact machinery already trusted
for USD. B was **abandoned** after the nested-FetchContent chain
(retarget → cloth-fit → igl → predicates) failed to build on CMake 4.4 + MinGW
(`No rule to make target …gitinfo.txt`); cloth-fit's own top-level build fetches
the same deps fine, so the failure is specific to nesting. A is simpler than C
but keeps the heavy `.so` on the link line, needs it resolvable at load time (no
clean `priv/` relocatability), and does not isolate TBB as cleanly.

### Implementation (landed)

* Bridge: `src/retarget_bridge/weftfit_retarget.{sigs,h,cpp}` — SHARED, hidden
  visibility, links `polyfem` privately, delay-loads `cloth_fit_usd` on Windows.
* Consumer stub: `weftfit_retarget_stub` = the loader
  (`loader.cpp` + `wfrt_loader.hpp`) plus, on POSIX, the generate_stubs table
  (`stubgen_compat.h` feeds `generate_stubs.py`). Windows binds via the
  delay-loaded import lib; POSIX binds via `wfrt::InitializeStubs`.
* NIF (`cloth_fit_cli/.../polyfem.cpp`): `simulate()` `dlopen`s the bridge and
  calls the C ABI; the heavy PolyFEM/igl/ipc/polysolve includes are gone
  (validate/info need only the header-only coordinate convention +
  nlohmann/json + the OBJ reader).
* Build (`cloth_fit.build_native`): builds the bridge + stub explicitly,
  `gen_link` emits only the stub/import-lib (+ `-delayload`) instead of the full
  static set, and bundles the bridge (+ `cloth_fit_usd`/`usd_ms`/tbb/plugins
  under USD) into `priv/`.

PRs: [#11](https://github.com/V-Sekai-fire/cloth-fit/pull/11) (`.sigs` contract),
[#12](https://github.com/V-Sekai-fire/cloth-fit/pull/12) (`run_retarget`
extraction + SHARED bridge),
[#13](https://github.com/V-Sekai-fire/cloth-fit/pull/13) (NIF `dlopen` rewire),
[#14](https://github.com/V-Sekai-fire/cloth-fit/pull/14) (POSIX
`InitializeStubs` fix).

### Consequences

* **Good** — The NIF drops from the full static PolyFEM link to **1.5 MB** and
  imports no `usd_ms`/PolyFEM symbols. One solve implementation, shared by all
  consumers. New consumers (`cli`, adapters, viewer) link the same tiny stub.
* **Good** — TBB isolation is structural, not a link-order gamble.
* **Good** — Fully relocatable from `priv/`; no `*_ROOT` at runtime.
* **Bad / cost** — Two-level `dlopen` at runtime (NIF → `weftfit_retarget` →
  `cloth_fit_usd`) and a per-OS binding mechanism to maintain (delay-load vs
  `dlsym` stubs). The `.sigs` is a hand-kept contract that must stay in sync with
  the header.
* **Bad / risk realized** — The two binding mechanisms are easy to get subtly
  wrong: the POSIX loader initially `dlopen`'d the bridge but never called
  `InitializeStubs`, leaving the weak forwarder pointers null → segfault on the
  first POSIX fit. Caught pre-CI by running `generate_stubs.py` locally and
  diffing against `cloth_fit_usd`; fixed in #14. Lesson: any new bridge must
  mirror **both** branches of `cfusd_loader`, not just the Windows one.

### Confirmation

Validated end-to-end on Windows (llvm-mingw): the `foxgirl_skirt` fit runs
entirely through the bridge and writes 236 valid `.usda` (`upAxis="Y"`) via the
bridge's internal `cloth_fit_usd` `dlopen`; the result JSON round-trips through
`wf_retarget_last_result()`. The POSIX build/run path is confirmed by CI
(`nif.yml`) on Linux and macOS, the authoritative cross-platform check.

## Pros and Cons of the Options

### A — Conventional shared library

* Good — Simple; no code generation; standard CMake `target_link_libraries`.
* Neutral — One shared solve implementation (same as C).
* Bad — Heavy `.so`/import lib stays on the consumer link line.
* Bad — Requires the lib resolvable at load; no clean `priv/` relocatability and
  no delay-load story for llvm-mingw's missing `dlfcn`.
* Bad — TBB isolation depends on link/load order rather than being structural.

### B — FetchContent the core into each consumer

* Good — No prebuilt artifact to ship; each consumer builds from source.
* Bad — **Does not build**: nested FetchContent
  (retarget → cloth-fit → igl → predicates) fails on CMake 4.4 + MinGW.
* Bad — Every consumer rebuilds the heavy stack; slow and still links PolyFEM.
* Bad — Defeats the goal — consumers are *more* coupled to PolyFEM, not less.

### C — `.sigs` / generate_stubs dlopen C-ABI bridge (chosen)

* Good — Consumer link line carries zero PolyFEM/TBB/USD.
* Good — Structural TBB isolation; fully relocatable from `priv/`.
* Good — Reuses the proven `cloth_fit_usd` mechanism; works on all three
  toolchains incl. llvm-mingw (delay-load) and POSIX (`dlsym` stubs).
* Bad — Two-level `dlopen`; per-OS binding code; hand-kept `.sigs` contract.

## More Information

* Reuses `thirdparty/generate_stubs/generate_stubs.py` and the `cloth_fit_usd`
  bridge pattern (`src/usd_bridge/`), which established this design for the
  §3 solver USD read/write boundary.
* Future (v2): extend the `.sigs` with `wf_mesh_source` / `wf_mesh_sink` vtables
  so I/O is driven through the weftfit ports (the `obj`/`stage` adapters) instead
  of file paths — see the `weftfit/retarget` `ports/mesh_{source,sink}.h` sketch.
* Related: manuals `20260610-hexagonal-core-ports-adapters` (the cluster
  decomposition this boundary serves); issue V-Sekai-fire/cloth-fit#2 §2/§3.
