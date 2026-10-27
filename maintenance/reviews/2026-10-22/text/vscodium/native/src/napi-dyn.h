// napi-dyn: dlsym-forwarding layer for napi symbols.
//
// The OHOS Electron main process loads libelectron.so with RTLD_LOCAL, so
// its napi_* exports are NOT in the global scope; the global scope only has
// a partial napi subset forwarded by the fork's ArkTS VM. OHcode's working
// prebuilt modules solve this by dlopen("libelectron.so", RTLD_NOLOAD) +
// dlsym at runtime. This header implements the same trick with zero call
// site changes:
//
//   - For every napi_* function we declare a typed function-pointer
//     variable napi_dyn_<name> and then #define <name> napi_dyn_<name>.
//     All later call sites (napi.h/node-addon-api, our code) transparently
//     call through the pointers.
//   - napi-dyn.c defines the pointers and fills them in a prioritized
//     constructor (runs before any plain constructor) via
//     dlopen("libelectron.so", RTLD_NOW|RTLD_NOLOAD), falling back to a
//     plain dlopen. A missing symbol aborts with a diagnostic line.
//
// Usage: compile all module TUs with -include napi-dyn.h (must precede any
// node_api.h / napi.h include) and link napi-dyn.o. -Wl,-Bsymbolic keeps
// pointer references module-local.
#ifndef NAPI_DYN_H
#define NAPI_DYN_H

#include <node_api.h>

#ifdef __cplusplus
extern "C" {
#endif

#define X(name) extern __typeof__(name)* napi_dyn_##name;
#include "napi-dyn-symbols.h"
#undef X

#ifdef __cplusplus
}
#endif

#ifndef NAPI_DYN_IMPL
/* redirect every napi_* call site to its forwarding pointer */
#include "napi-dyn-redir.h"
#endif

#endif /* NAPI_DYN_H */
