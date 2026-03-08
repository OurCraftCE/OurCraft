#ifdef __EMSCRIPTEN__

// EmscriptenStorage — CStorageManager overrides for WASM.
//
// uses Emscripten's IDBFS (IndexedDB) for persistent saves.
// mount point: /saves  →  IndexedDB store "minecraft-saves"
//
// call EmscriptenStorage::Init() once at startup (before any file I/O).
// call EmscriptenStorage::Sync() after write operations to persist to IDB.

#include <emscripten/emscripten.h>

namespace EmscriptenStorage
{
    static bool s_syncing = false;

    void Init()
    {
        EM_ASM({
            FS.mkdir('/saves');
            FS.mount(IDBFS, {}, '/saves');
            // async populate from IndexedDB — game must wait for callback
            FS.syncfs(true, function(err) {
                if (err) console.error('IDBFS populate error:', err);
                else     console.log('Saves loaded from IndexedDB');
            });
        });
    }

    // flush MEMFS → IndexedDB after saving a world
    void Sync()
    {
        if (s_syncing) return;
        s_syncing = true;
        EM_ASM({
            FS.syncfs(false, function(err) {
                if (err) console.error('IDBFS sync error:', err);
                else     console.log('Saves written to IndexedDB');
                Module._EmscriptenStorage_SyncDone();
            });
        });
    }

    extern "C" EMSCRIPTEN_KEEPALIVE void EmscriptenStorage_SyncDone()
    {
        s_syncing = false;
    }
}

#endif // __EMSCRIPTEN__
