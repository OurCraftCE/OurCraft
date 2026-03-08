#pragma once
#ifdef __EMSCRIPTEN__

namespace EmscriptenStorage
{
    void Init();   // Call once at startup — mounts IDBFS at /saves
    void Sync();   // Call after saving — flushes MEMFS to IndexedDB
}

#endif // __EMSCRIPTEN__
