// stubgen_compat.h — inserted (via --macro-include) after the system includes in
// the generate_stubs.py-emitted cloth_fit_usd_stubs.cc. Makes the C-ABI types
// (cfusd_mesh_t) visible to the generated dispatch table, and provides the empty
// DISABLE_CFI_ICALL macro the generator emits before each dispatched call.

#pragma once

#include "cloth_fit_usd.h"

#ifndef DISABLE_CFI_ICALL
#define DISABLE_CFI_ICALL
#endif
