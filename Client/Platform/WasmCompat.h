#pragma once
#ifdef __EMSCRIPTEN__

// WASM uses the shared platform compat layer
#include "PlatformCompat.h"

// Emscripten-specific extras
#include <emscripten/emscripten.h>

// WASM has no process/file path introspection — override GetModuleFileNameW
#undef GetModuleFileNameW  // remove the linux readlink version from PlatformCompat
inline DWORD GetModuleFileNameW(HMODULE, LPWSTR buf, DWORD size) {
    if (buf && size > 0) buf[0] = 0;
    return 0;
}

#endif // __EMSCRIPTEN__
