#pragma once

#include <string>
#include <vector>
#include "..\..\ModAPI\ModAPI.h"

// Cross-platform shared library abstraction
#ifdef _WIN32
    #include <windows.h>
    typedef HMODULE SharedLibHandle;
#else
    typedef void* SharedLibHandle;
#endif

SharedLibHandle LoadSharedLib(const char* path);
void*           GetSymbol(SharedLibHandle handle, const char* name);
void            UnloadSharedLib(SharedLibHandle handle);

// A single loaded mod
struct LoadedMod {
    SharedLibHandle handle;
    ModInfo*        info;
    std::string     path;
    bool            initialized;
};

class ModLoader {
public:
    ModLoader();
    ~ModLoader();

    // Set the mods directory and create it if it doesn't exist
    void Initialize(const std::string& modsDir);

    // Scan the mods directory and load all .dll / .so files
    void LoadAllMods();

    // Load a single mod from a shared library file
    bool LoadMod(const std::string& path);

    // Call onInitialize on all loaded mods
    void InitializeAllMods();

    // Call onTick on all loaded mods
    void TickAllMods(float deltaTime);

    // Call onShutdown on all loaded mods, then unload libraries
    void ShutdownAllMods();

    // Notify mods of world load/unload
    void NotifyWorldLoad(void* levelPtr);
    void NotifyWorldUnload(void* levelPtr);

    // Get count of loaded mods
    int GetModCount() const;

    // Singleton access
    static ModLoader& Get();

private:
    std::string              m_modsDir;
    std::vector<LoadedMod>   m_mods;
    bool                     m_initialized;

    static ModLoader         s_instance;
};
