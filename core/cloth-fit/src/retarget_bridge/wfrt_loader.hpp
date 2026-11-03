#pragma once

// Consumer-side loader for the weftfit_retarget bridge. A consumer (the NIF/CLI)
// links this + the bridge's import lib (delay-loaded on Windows) and calls
// load_from_env() once before any wf_retarget_* call. The bridge is self-contained
// (PolyFEM + garment core + TBB inside), so no external deps to resolve — the
// loader just LoadLibraryEx's / dlopens the bridge sitting next to the consumer.
namespace wfrt_loader
{
    bool loaded();
    // dlopen / LoadLibraryEx libweftfit_retarget next to the consumer module (or
    // $WFRT_BRIDGE). Idempotent; returns true once the bridge is loaded.
    bool load_from_env();
}
