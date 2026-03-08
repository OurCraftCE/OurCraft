#ifdef __EMSCRIPTEN__

#include "WasmApp.h"

// Define the global app instance for WASM builds.
// This replaces Windows64/Windows64_App.cpp or Durango/Durango_App.cpp.
CConsoleMinecraftApp app;

#endif // __EMSCRIPTEN__
