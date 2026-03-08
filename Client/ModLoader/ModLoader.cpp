#ifndef MINECRAFT_SERVER
#include "stdafx.h"
#endif
#include "ModLoader.h"
#include <cstdio>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
#else
    #include <dlfcn.h>
    #include <dirent.h>
    #include <sys/stat.h>
    #include <cstring>
#endif

// ============================================================
// Cross-platform shared library functions
// ============================================================

SharedLibHandle LoadSharedLib(const char* path)
{
#ifdef _WIN32
    return LoadLibraryA(path);
#else
    return dlopen(path, RTLD_NOW);
#endif
}

void* GetSymbol(SharedLibHandle handle, const char* name)
{
#ifdef _WIN32
    return (void*)GetProcAddress(handle, name);
#else
    return dlsym(handle, name);
#endif
}

void UnloadSharedLib(SharedLibHandle handle)
{
    if (!handle) return;
#ifdef _WIN32
    FreeLibrary(handle);
#else
    dlclose(handle);
#endif
}

// ============================================================
// ModLoader singleton
// ============================================================

ModLoader ModLoader::s_instance;

ModLoader& ModLoader::Get()
{
    return s_instance;
}

ModLoader::ModLoader()
    : m_initialized(false)
{
}

ModLoader::~ModLoader()
{
    ShutdownAllMods();
}

void ModLoader::Initialize(const std::string& modsDir)
{
    m_modsDir = modsDir;
    m_initialized = true;

    // Create mods directory if it doesn't exist
#ifdef _WIN32
    CreateDirectoryA(modsDir.c_str(), NULL);
#else
    mkdir(modsDir.c_str(), 0755);
#endif

    printf("[ModLoader] Initialized. Mods directory: %s\n", modsDir.c_str());
}

void ModLoader::LoadAllMods()
{
    if (!m_initialized) {
        printf("[ModLoader] Error: not initialized\n");
        return;
    }

    printf("[ModLoader] Scanning for mods in: %s\n", m_modsDir.c_str());

#ifdef _WIN32
    // Windows: use FindFirstFileA / FindNextFileA
    std::string searchPath = m_modsDir + "\\*.dll";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        printf("[ModLoader] No mod files found.\n");
        return;
    }

    do {
        std::string fullPath = m_modsDir + "\\" + findData.cFileName;
        LoadMod(fullPath);
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
#else
    // Linux/Unix: use opendir/readdir
    DIR* dir = opendir(m_modsDir.c_str());
    if (!dir) {
        printf("[ModLoader] No mod files found.\n");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string filename = entry->d_name;
        // Check for .so extension
        if (filename.length() > 3 && filename.substr(filename.length() - 3) == ".so") {
            std::string fullPath = m_modsDir + "/" + filename;
            LoadMod(fullPath);
        }
    }
    closedir(dir);
#endif

    printf("[ModLoader] Loaded %d mod(s).\n", (int)m_mods.size());
}

bool ModLoader::LoadMod(const std::string& path)
{
    printf("[ModLoader] Loading mod: %s\n", path.c_str());

    SharedLibHandle handle = LoadSharedLib(path.c_str());
    if (!handle) {
        printf("[ModLoader] Failed to load library: %s\n", path.c_str());
#ifdef _WIN32
        printf("[ModLoader] Win32 error code: %lu\n", GetLastError());
#else
        printf("[ModLoader] Error: %s\n", dlerror());
#endif
        return false;
    }

    // Look for the GetModInfo export
    GetModInfoFunc getModInfo = (GetModInfoFunc)GetSymbol(handle, "GetModInfo");
    if (!getModInfo) {
        printf("[ModLoader] No GetModInfo export found in: %s\n", path.c_str());
        UnloadSharedLib(handle);
        return false;
    }

    ModInfo* info = getModInfo();
    if (!info) {
        printf("[ModLoader] GetModInfo returned NULL in: %s\n", path.c_str());
        UnloadSharedLib(handle);
        return false;
    }

    LoadedMod mod;
    mod.handle = handle;
    mod.info = info;
    mod.path = path;
    mod.initialized = false;

    printf("[ModLoader] Found mod: %s v%s by %s\n",
        info->modName ? info->modName : "Unknown",
        info->modVersion ? info->modVersion : "0.0.0",
        info->modAuthor ? info->modAuthor : "Unknown");

    m_mods.push_back(mod);
    return true;
}

void ModLoader::InitializeAllMods()
{
    for (size_t i = 0; i < m_mods.size(); i++) {
        LoadedMod& mod = m_mods[i];
        if (!mod.initialized && mod.info && mod.info->onInitialize) {
            printf("[ModLoader] Initializing mod: %s\n",
                mod.info->modName ? mod.info->modName : "Unknown");
            mod.info->onInitialize();
            mod.initialized = true;
        }
    }
}

void ModLoader::TickAllMods(float deltaTime)
{
    for (size_t i = 0; i < m_mods.size(); i++) {
        LoadedMod& mod = m_mods[i];
        if (mod.initialized && mod.info && mod.info->onTick) {
            mod.info->onTick(deltaTime);
        }
    }
}

void ModLoader::ShutdownAllMods()
{
    for (size_t i = 0; i < m_mods.size(); i++) {
        LoadedMod& mod = m_mods[i];
        if (mod.initialized && mod.info && mod.info->onShutdown) {
            printf("[ModLoader] Shutting down mod: %s\n",
                mod.info->modName ? mod.info->modName : "Unknown");
            mod.info->onShutdown();
        }
        if (mod.handle) {
            UnloadSharedLib(mod.handle);
            mod.handle = NULL;
        }
        mod.initialized = false;
    }
    m_mods.clear();
}

void ModLoader::NotifyWorldLoad(void* levelPtr)
{
    ModWorld world = (ModWorld)levelPtr;
    for (size_t i = 0; i < m_mods.size(); i++) {
        LoadedMod& mod = m_mods[i];
        if (mod.initialized && mod.info && mod.info->onWorldLoad) {
            mod.info->onWorldLoad(world);
        }
    }
}

void ModLoader::NotifyWorldUnload(void* levelPtr)
{
    ModWorld world = (ModWorld)levelPtr;
    for (size_t i = 0; i < m_mods.size(); i++) {
        LoadedMod& mod = m_mods[i];
        if (mod.initialized && mod.info && mod.info->onWorldUnload) {
            mod.info->onWorldUnload(world);
        }
    }
}

int ModLoader::GetModCount() const
{
    return (int)m_mods.size();
}
