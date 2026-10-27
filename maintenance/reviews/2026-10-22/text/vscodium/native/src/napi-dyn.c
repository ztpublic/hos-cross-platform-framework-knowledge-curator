// See napi-dyn.h for the design. This file defines the forwarding pointers
// and resolves them from libelectron.so at load time.
//
// Compiled as C with NAPI_DYN_IMPL so the redir macros stay off: here the
// names refer to the real declarations from node_api.h.

#define NAPI_DYN_IMPL
#include "napi-dyn.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#define X(name) __typeof__(name)* napi_dyn_##name;
#include "napi-dyn-symbols.h"
#undef X

/* Priority 101: run before any unprioritized constructors in this module. */
static void __attribute__((constructor(101))) napi_dyn_init(void) {
  void* h = dlopen("libelectron.so", RTLD_NOW | RTLD_NOLOAD);
  if (!h) {
    h = dlopen("libelectron.so", RTLD_NOW);
  }
  if (!h) {
    fprintf(stderr, "napi-dyn: cannot get handle for libelectron.so: %s\n",
            dlerror());
    abort();
  }
#define X(name)                                                          \
  do {                                                                   \
    napi_dyn_##name = (__typeof__(napi_dyn_##name))dlsym(h, #name);      \
    if (!napi_dyn_##name) {                                              \
      fprintf(stderr, "napi-dyn: symbol not found: %s\n", #name);        \
      abort();                                                           \
    }                                                                    \
  } while (0);
#include "napi-dyn-symbols.h"
#undef X
}
