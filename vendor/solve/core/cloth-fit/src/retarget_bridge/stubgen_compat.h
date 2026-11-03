// stubgen_compat.h — inserted (via --macro-include) after the system includes in
// the generate_stubs.py-emitted weftfit_retarget_stubs.cc. Makes the C-ABI decls
// (wf_retarget_*) visible to the generated dispatch table and provides the empty
// DISABLE_CFI_ICALL macro the generator emits before each dispatched call. The
// weftfit_retarget ABI uses only plain scalar/pointer types (no opaque handles),
// so this just pulls in the header.

#pragma once

#include "weftfit_retarget.h"

#ifndef DISABLE_CFI_ICALL
#define DISABLE_CFI_ICALL
#endif
