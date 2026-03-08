// CStorageManager — C++20 reimplementation using std::filesystem, no proprietary 4J code

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>

#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "../../World/x64headers/extraX64.h"
typedef unsigned __int64 __uint64;

#include "StorageManager.h"
#include "../Windows64/Win64SavePaths.h"

namespace fs = std::filesystem;

CStorageManager StorageManager;

static bool              g_saveDisabled = false;
static bool              g_deviceSelected[XUSER_MAX_COUNT]{};
static SAVE_DETAILS      g_details{};
static std::vector<SAVE_INFO> g_infoList;
static std::vector<uint8_t>   g_saveBuffer;
static uint8_t*          g_saveData    = nullptr;
static uint32_t          g_saveSize    = 0;
static uint32_t          g_saveVersion = 0;
static std::wstring      g_defaultName;
static std::wstring      g_title;
static std::string       g_packName;
static std::wstring      g_filename;
static uint8_t*          g_thumbData   = nullptr;
static uint32_t          g_thumbBytes  = 0;

static fs::path SavesRoot() {
    return fs::path(Win64SavePaths::GetWorldSavesDir());
}

static fs::path SaveFolder(const char* name) {
    return SavesRoot() / name;
}

static std::wstring Utf8ToWide(const char* s) {
    std::wstring out;
    while (*s) out += static_cast<wchar_t>(*s++);
    return out;
}

static bool WriteAtomic(const fs::path& dst, const void* data, size_t size) {
    auto tmp = dst;
    tmp += L".tmp";
    {
        std::ofstream f(tmp, std::ios::binary);
        if (!f) return false;
        f.write(static_cast<const char*>(data), size);
        if (!f) { std::error_code ec; fs::remove(tmp, ec); return false; }
    }
    std::error_code ec;
    fs::rename(tmp, dst, ec);
    if (ec) {
        fs::copy_file(tmp, dst, fs::copy_options::overwrite_existing, ec);
        fs::remove(tmp, ec);
    }
    return true;
}

static time_t FileTimeToUnix(const fs::file_time_type& ft) {
    auto sctp = std::chrono::clock_cast<std::chrono::system_clock>(ft);
    return std::chrono::system_clock::to_time_t(sctp);
}

CStorageManager::CStorageManager() : m_pStringTable(nullptr) {}

void CStorageManager::Tick() {}

CStorageManager::EMessageResult CStorageManager::RequestMessageBox(
    UINT, UINT, UINT*, UINT, DWORD,
    int(*Func)(LPVOID, int, const EMessageResult), LPVOID lpParam,
    C4JStringTable*, WCHAR*, DWORD)
{
    return EMessage_ResultAccept;
}

CStorageManager::EMessageResult CStorageManager::GetMessageBoxResult() {
    return EMessage_ResultAccept;
}

bool CStorageManager::SetSaveDevice(int(*Func)(LPVOID, const bool), LPVOID p, bool) {
    if (Func) Func(p, true);
    return true;
}

void CStorageManager::Init(unsigned int ver, LPCWSTR name, char* pack,
                      int, int(*)(LPVOID, const ESavingMessage, int), LPVOID, LPCSTR)
{
    g_saveVersion = ver;
    if (name) g_defaultName = name;
    if (pack) g_packName = pack;
}

void CStorageManager::ResetSaveData() {
    g_filename.clear();
    delete[] g_saveData;
    g_saveData = nullptr;
    g_saveSize = 0;
}

void CStorageManager::SetDefaultSaveNameForKeyboardDisplay(LPCWSTR n) {
    if (n) g_defaultName = n;
}

void CStorageManager::SetSaveTitle(LPCWSTR t) {
    if (t) g_title = t;
}

bool CStorageManager::GetSaveUniqueNumber(INT* out) {
    int count = 0;
    std::error_code ec;
    for (auto& e : fs::directory_iterator(SavesRoot(), ec))
        if (e.is_directory()) ++count;
    *out = count + 1;
    return true;
}

bool CStorageManager::GetSaveUniqueFilename(char* buf) {
    INT n = 0;
    GetSaveUniqueNumber(&n);
    sprintf_s(buf, MAX_SAVEFILENAME_LENGTH, "save_%d", n);
    return true;
}

void CStorageManager::SetSaveUniqueFilename(char* name) {
    if (name) g_filename = Utf8ToWide(name);
}

void CStorageManager::SetState(ESaveGameControlState, int(*Func)(LPVOID, const bool), LPVOID p) {
    if (Func) Func(p, true);
}

void CStorageManager::SetSaveDisabled(bool b)   { g_saveDisabled = b; }
bool CStorageManager::GetSaveDisabled()         { return g_saveDisabled; }
unsigned int CStorageManager::GetSaveSize()     { return g_saveSize; }

void CStorageManager::GetSaveData(void* dst, unsigned int* sz) {
    if (g_saveData && g_saveSize) {
        memcpy(dst, g_saveData, g_saveSize);
        *sz = g_saveSize;
    } else {
        *sz = 0;
    }
}

PVOID CStorageManager::AllocateSaveData(unsigned int bytes) {
    delete[] g_saveData;
    g_saveData = new uint8_t[bytes]();
    g_saveSize = bytes;
    return g_saveData;
}

void CStorageManager::SetSaveImages(PBYTE thumb, DWORD thumbSz, PBYTE, DWORD, PBYTE, DWORD) {
    g_thumbData  = thumb;
    g_thumbBytes = thumbSz;
}

CStorageManager::ESaveGameState CStorageManager::SaveSaveData(int(*Func)(LPVOID, const bool), LPVOID p) {
    bool ok = false;
    if (g_saveData && g_saveSize && !g_filename.empty()) {
        auto dir = SavesRoot() / g_filename;
        std::error_code ec;
        fs::create_directories(dir, ec);

        ok = WriteAtomic(dir / L"savedata.bin", g_saveData, g_saveSize);

        if (g_thumbData && g_thumbBytes)
            WriteAtomic(dir / L"thumbnail.png", g_thumbData, g_thumbBytes);

        if (!g_title.empty())
            WriteAtomic(dir / L"worldname.txt", g_title.c_str(), g_title.size() * sizeof(wchar_t));
    }
    if (Func) Func(p, ok);
    return ESaveGame_Idle;
}

void CStorageManager::CopySaveDataToNewSave(PBYTE, DWORD, WCHAR*, int(*Func)(LPVOID, bool), LPVOID p) {
    if (Func) Func(p, true);
}

void CStorageManager::SetSaveDeviceSelected(unsigned int pad, bool sel) {
    if (pad < XUSER_MAX_COUNT) g_deviceSelected[pad] = sel;
}

bool CStorageManager::GetSaveDeviceSelected(unsigned int pad) {
    return (pad < XUSER_MAX_COUNT) ? g_deviceSelected[pad] : true;
}

CStorageManager::ESaveGameState CStorageManager::DoesSaveExist(bool* exists) {
    if (!g_filename.empty()) {
        auto p = SavesRoot() / g_filename / L"savedata.bin";
        *exists = fs::exists(p);
    } else {
        *exists = false;
    }
    return ESaveGame_Idle;
}

bool CStorageManager::EnoughSpaceForAMinSaveGame() {
    std::error_code ec;
    auto si = fs::space(SavesRoot(), ec);
    if (ec) return true;
    return si.available > 10 * 1024 * 1024;
}

void CStorageManager::SetSaveMessageVPosition(float) {}

CStorageManager::ESaveGameState CStorageManager::GetSavesInfo(
    int, int(*Func)(LPVOID, SAVE_DETAILS*, const bool), LPVOID p, char*)
{
    g_infoList.clear();
    std::error_code ec;

    for (auto& entry : fs::directory_iterator(SavesRoot(), ec)) {
        if (!entry.is_directory()) continue;

        auto savebin = entry.path() / L"savedata.bin";
        auto leveldat = entry.path() / L"level.dat";
        fs::path dataFile;
        if (fs::exists(savebin))       dataFile = savebin;
        else if (fs::exists(leveldat)) dataFile = leveldat;
        else continue;

        SAVE_INFO info{};

        auto folderName = entry.path().filename().string();
        strncpy_s(info.UTF8SaveFilename, folderName.c_str(), MAX_SAVEFILENAME_LENGTH - 1);

        std::wstring displayName;
        auto metaPath = entry.path() / L"worldname.txt";
        if (fs::exists(metaPath)) {
            std::ifstream mf(metaPath, std::ios::binary);
            std::vector<char> buf(fs::file_size(metaPath));
            if (mf.read(buf.data(), buf.size())) {
                displayName.assign(reinterpret_cast<const wchar_t*>(buf.data()),
                                   buf.size() / sizeof(wchar_t));
            }
        }
        if (displayName.empty())
            displayName = entry.path().filename().wstring();

        WideCharToMultiByte(CP_UTF8, 0, displayName.c_str(), -1,
                            info.UTF8SaveTitle, MAX_DISPLAYNAME_LENGTH, nullptr, nullptr);

        info.metaData.modifiedTime = FileTimeToUnix(fs::last_write_time(entry, ec));
        info.metaData.dataSize = static_cast<unsigned int>(fs::file_size(dataFile, ec));
        info.metaData.thumbnailSize = 0;
        info.thumbnailData = nullptr;
        info.needsSync = false;

        g_infoList.push_back(info);
    }

    g_details.iSaveC    = static_cast<int>(g_infoList.size());
    g_details.SaveInfoA = g_infoList.empty() ? nullptr : g_infoList.data();

    if (Func) Func(p, &g_details, true);
    return ESaveGame_GetSavesInfo;
}

PSAVE_DETAILS CStorageManager::ReturnSavesInfo() { return &g_details; }

void CStorageManager::ClearSavesInfo() {
    g_infoList.clear();
    g_details = {};
}

CStorageManager::ESaveGameState CStorageManager::LoadSaveDataThumbnail(
    PSAVE_INFO, int(*Func)(LPVOID, PBYTE, DWORD), LPVOID p, bool)
{
    if (Func) Func(p, nullptr, 0);
    return ESaveGame_GetSaveThumbnail;
}

CStorageManager::ESaveGameState CStorageManager::RenameSaveData(
    int, uint16_t*, int(*Func)(LPVOID, bool), LPVOID p)
{
    if (Func) Func(p, true);
    return ESaveGame_Rename;
}

CStorageManager::ESaveGameState CStorageManager::GetSaveState() { return ESaveGame_Idle; }
void CStorageManager::ReadFromProfile(int) {}
void CStorageManager::GetSaveCacheFileInfo(DWORD, XCONTENT_DATA&) {}
void CStorageManager::GetSaveCacheFileInfo(DWORD, PBYTE* pp, DWORD* ps) {
    if (pp) *pp = nullptr;
    if (ps) *ps = 0;
}

CStorageManager::ESaveGameState CStorageManager::LoadSaveData(
    PSAVE_INFO si, int(*Func)(LPVOID, const bool, const bool), LPVOID p)
{
    bool ok = false;
    if (si) {
        auto path = SaveFolder(si->UTF8SaveFilename) / L"savedata.bin";
        std::error_code ec;
        auto sz = fs::file_size(path, ec);
        if (!ec && sz > 0) {
            AllocateSaveData(static_cast<unsigned int>(sz));
            std::ifstream f(path, std::ios::binary);
            if (f.read(reinterpret_cast<char*>(g_saveData), sz))
                ok = true;
        }
    }
    if (Func) Func(p, ok, false);
    return ESaveGame_Load;
}

CStorageManager::ESaveGameState CStorageManager::DeleteSaveData(
    PSAVE_INFO si, int(*Func)(LPVOID, const bool), LPVOID p)
{
    bool ok = false;
    if (si) {
        auto dir = SaveFolder(si->UTF8SaveFilename);
        std::error_code ec;
        ok = fs::remove_all(dir, ec) > 0;
    }
    if (Func) Func(p, ok);
    return ESaveGame_Delete;
}

// DLC stubs
void CStorageManager::RegisterMarketplaceCountsCallback(int(*)(LPVOID, DLC_TMS_DETAILS*, int), LPVOID) {}
void CStorageManager::SetDLCPackageRoot(char*) {}
CStorageManager::EDLCStatus CStorageManager::GetDLCOffers(int, int(*)(LPVOID, int, DWORD, int), LPVOID, DWORD) { return EDLC_NoOffers; }
DWORD CStorageManager::CancelGetDLCOffers() { return 0; }
void  CStorageManager::ClearDLCOffers() {}
XMARKETPLACE_CONTENTOFFER_INFO& CStorageManager::GetOffer(DWORD) { static XMARKETPLACE_CONTENTOFFER_INFO d{}; return d; }
int   CStorageManager::GetOfferCount() { return 0; }
DWORD CStorageManager::InstallOffer(int, __uint64*, int(*)(LPVOID, int, int), LPVOID, bool) { return 0; }
DWORD CStorageManager::InstallOffer(int, LPCWSTR, int(*)(LPVOID, int, int), LPVOID, bool)  { return 0; }
DWORD CStorageManager::GetAvailableDLCCount(int) { return 0; }
CStorageManager::EDLCStatus CStorageManager::GetInstalledDLC(int, int(*)(LPVOID, int, int), LPVOID) { return EDLC_NoInstalledDLC; }
XCONTENT_DATA& CStorageManager::GetDLC(DWORD) { static XCONTENT_DATA d{}; return d; }
DWORD CStorageManager::MountInstalledDLC(int, DWORD, int(*)(LPVOID, int, DWORD, DWORD), LPVOID, LPCSTR) { return 0; }
DWORD CStorageManager::UnmountInstalledDLC(LPCSTR)  { return 0; }
DWORD CStorageManager::UnmountInstalledDLC(LPCWSTR) { return 0; }
void  CStorageManager::GetMountedDLCFileList(const char*, std::vector<std::string>&) {}
std::string  CStorageManager::GetMountedPath(std::string  m) { return m; }
std::wstring CStorageManager::GetMountedPath(std::wstring m) { return m; }
XCONTENT_DATA* CStorageManager::GetInstalledDLC(WCHAR*) { return nullptr; }

// TMS stubs
CStorageManager::ETMSStatus CStorageManager::ReadTMSFile(int, eGlobalStorage, eTMS_FileType,
    WCHAR*, BYTE**, DWORD*, int(*)(LPVOID, WCHAR*, int, bool, int), LPVOID, int)
{ return ETMSStatus_Idle; }
bool CStorageManager::WriteTMSFile(int, eGlobalStorage, WCHAR*, BYTE*, DWORD) { return false; }
bool CStorageManager::DeleteTMSFile(int, eGlobalStorage, WCHAR*) { return false; }
void CStorageManager::StoreTMSPathName(WCHAR*) {}

// TMS++ stubs
CStorageManager::ETMSStatus CStorageManager::TMSPP_WriteFile(int, eGlobalStorage, eTMS_FILETYPEVAL, LPCWSTR, PBYTE, DWORD, int(*)(LPVOID, int, int), LPVOID, int) { return ETMSStatus_Idle; }
CStorageManager::ETMSStatus CStorageManager::TMSPP_WriteFile(int, eGlobalStorage, eTMS_FILETYPEVAL, eTMS_UGCTYPE, LPCSTR, PCHAR, DWORD, int(*)(LPVOID, int, int), LPVOID, int) { return ETMSStatus_Idle; }
CStorageManager::ETMSStatus CStorageManager::TMSPP_WriteFileWithProgress(int, eGlobalStorage, eTMS_FILETYPEVAL, eTMS_UGCTYPE, LPCSTR, PCHAR, DWORD, int(*)(LPVOID, int, int), LPVOID, int, int(*)(LPVOID, float), LPVOID) { return ETMSStatus_Idle; }
CStorageManager::ETMSStatus CStorageManager::TMSPP_ReadFile(int, eGlobalStorage, eTMS_FILETYPEVAL, LPCSTR, int(*)(LPVOID, int, int, PTMSPP_FILEDATA, LPCSTR), LPVOID, int) { return ETMSStatus_Idle; }
CStorageManager::ETMSStatus CStorageManager::TMSPP_ReadFile(int, eGlobalStorage, eTMS_FILETYPEVAL, LPCWSTR, int(*)(LPVOID, int, int, LPVOID, WCHAR*), LPVOID, int) { return ETMSStatus_Idle; }
CStorageManager::ETMSStatus CStorageManager::TMSPP_ReadFileList(int, eGlobalStorage, int(*)(LPVOID, int, int, LPVOID, WCHAR*), LPVOID, int) { return ETMSStatus_Idle; }
CStorageManager::ETMSStatus CStorageManager::TMSPP_DeleteFile(int, LPCSTR, eTMS_FILETYPEVAL, int(*)(LPVOID, int, int), LPVOID, int) { return ETMSStatus_Idle; }
CStorageManager::ETMSStatus CStorageManager::TMSPP_DeleteFile(int, eGlobalStorage, eTMS_FILETYPEVAL, LPCWSTR, int(*)(LPVOID, int, int), LPVOID, int) { return ETMSStatus_Idle; }
bool CStorageManager::TMSPP_InFileList(eGlobalStorage, int, const wstring&) { return false; }

// save transfer stubs
CStorageManager::eSaveTransferState CStorageManager::SaveTransferGetDetails(int, eGlobalStorage, WCHAR*, int(*)(LPVOID, SAVETRANSFER_FILE_DETAILS*), LPVOID) { return eSaveTransfer_Idle; }
CStorageManager::eSaveTransferState CStorageManager::SaveTransferGetData(int, eGlobalStorage, WCHAR*, int(*)(LPVOID, SAVETRANSFER_FILE_DETAILS*), int(*)(LPVOID, unsigned long), LPVOID, LPVOID) { return eSaveTransfer_Idle; }
CStorageManager::eSaveTransferState CStorageManager::SaveTransferClearState() { return eSaveTransfer_Idle; }
void CStorageManager::CancelSaveTransfer(int(*Func)(LPVOID), LPVOID p) { if (Func) Func(p); }

// CRC32 (standard table-driven, reflected polynomial 0xEDB88320)
unsigned int CStorageManager::CRC(unsigned char* buf, int len) {
    uint32_t crc = 0xFFFFFFFF;
    for (int i = 0; i < len; ++i) {
        crc ^= buf[i];
        for (int j = 0; j < 8; ++j)
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
    }
    return crc ^ 0xFFFFFFFF;
}

void CStorageManager::ContinueIncompleteOperation() {}
