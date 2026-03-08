// no-op stubs for client symbols in Core.lib/World.lib (renderer, UI, audio, etc.)

#include "..\Client\stdafx.h"
#include "..\Client\Game\Settings.h"
#include "..\Client\Input\ConsoleInput.h"
#include "..\Client\Render\LevelRenderer.h"
#include "..\Client\Render\GameRenderer.h"
#include "..\Client\Game\StatsCounter.h"
#include "..\Client\Textures\TexturePackRepository.h"

class GameCommandPacket; // forward-declare to avoid pulling GameCommandPacket
#include "..\Client\Game\TeleportCommand.h"

unsigned int g_serverDifficulty = 2;  // 0=Peaceful,1=Easy,2=Normal,3=Hard
unsigned int g_serverGameType = 0;    // 0=Survival,1=Creative,2=Adventure

// global singletons referenced as extern by World.lib / Core.lib
#pragma warning(push)
#pragma warning(disable: 4273) // inconsistent dll linkage (some headers may have dllimport)
CConsoleMinecraftApp app;
CGameNetworkManager g_NetworkManager;
CStorageManager StorageManager;
C_4JProfile ProfileManager;
static CTelemetryManager s_telemetryStub;
CTelemetryManager* TelemetryManager = &s_telemetryStub;
#pragma warning(pop)

void MemSect(int sect) {}

DLCManager::DLCManager() { memset(this, 0, sizeof(*this)); }
DLCManager::~DLCManager() {}
DLCSkinFile* DLCManager::getSkinFile(const wstring& name) { return nullptr; }

DLCCatalog::DLCCatalog() { memset(this, 0, sizeof(*this)); }
DLCCatalog::~DLCCatalog() {}

DLCInstaller::DLCInstaller() { memset(this, 0, sizeof(*this)); }
DLCInstaller::~DLCInstaller() {}

GameRuleManager::GameRuleManager() { memset(this, 0, sizeof(*this)); }
LevelGenerators::LevelGenerators() { memset(this, 0, sizeof(*this)); }
LevelRules::LevelRules() { memset(this, 0, sizeof(*this)); }

CGameNetworkManager::CGameNetworkManager() { memset(this, 0, sizeof(*this)); }
CStorageManager::CStorageManager() { memset(this, 0, sizeof(*this)); }

// PIX debug markers — no-op on server
void PIXBeginNamedEvent(int, char*, ...) {}
void PIXEndNamedEvent(void) {}
void PIXAddNamedCounter(int, char*, ...) {}

HRESULT XMemDecompress(void* ctx, void* pDest, SIZE_T* pDestSize, const void* pSrc, SIZE_T srcSize) {
    if (pDestSize) *pDestSize = 0;
    return E_NOTIMPL;
}

CMinecraftApp::CMinecraftApp() {}
CConsoleMinecraftApp::CConsoleMinecraftApp() {}
unsigned char CMinecraftApp::GetGameSettings(eGameSetting eVal) { return 0; }
bool CMinecraftApp::isXuidNotch(PlayerUID xuid) { return false; }
bool CMinecraftApp::DefaultCapeExists() { return false; }
MOJANG_DATA* CMinecraftApp::GetMojangDataForXuid(PlayerUID xuid) { return nullptr; }
vector<ModelPart*>* CMinecraftApp::SetAdditionalSkinBoxes(DWORD dwSkinID, vector<SKIN_BOX*>* pvSkinBoxA) { return nullptr; }
vector<ModelPart*>* CMinecraftApp::GetAdditionalModelParts(DWORD dwSkinID) { return nullptr; }
wstring CMinecraftApp::getSkinPathFromId(DWORD skinId) { return L""; }
DWORD CMinecraftApp::getSkinIdFromPath(const wstring& skin) { return 0; }
LPCWSTR CMinecraftApp::GetString(int iID) { return L""; }
int CMinecraftApp::GetHTMLColour(eMinecraftColour colour) { return 0xFFFFFF; }
void CMinecraftApp::SetUniqueMapName(char* pszUniqueMapName) {}
void CMinecraftApp::processSchematics(LevelChunk* levelChunk) {}
void CMinecraftApp::processSchematicsLighting(LevelChunk* levelChunk) {}
void CMinecraftApp::AddTerrainFeaturePosition(_eTerrainFeatureType type, int x, int z) {}
bool CMinecraftApp::GetTerrainFeaturePosition(_eTerrainFeatureType eType, int* pX, int* pZ) { return false; }
unsigned int CMinecraftApp::GetGameHostOption(eGameHostOption eVal) {
    if (eVal == eGameHostOption_Difficulty) return g_serverDifficulty;
    if (eVal == eGameHostOption_GameType) return g_serverGameType;
    return 0;
}
unsigned int CMinecraftApp::GetGameHostOption(unsigned int uiHostSettings, eGameHostOption eVal) {
    return GetGameHostOption(eVal);
}
void CMinecraftApp::SetGameHostOption(eGameHostOption eVal, unsigned int uiVal) {
    if (eVal == eGameHostOption_Difficulty) g_serverDifficulty = uiVal;
    else if (eVal == eGameHostOption_GameType) g_serverGameType = uiVal;
}
unsigned int CMinecraftApp::GetGameSettingsDebugMask(int iPad, bool bOverridePlayer) { return 0; }
void CMinecraftApp::SetGameSettingsDebugMask(int iPad, unsigned int uiVal) {}
unsigned int CMinecraftApp::CreateImageTextData(PBYTE bTextMetadata, __int64 seed, bool hasSeed, unsigned int uiHostOptions, unsigned int uiTexturePackId) { return 0; }
void CMinecraftApp::AddMemoryTextureFile(const wstring& wName, PBYTE pbData, DWORD dwBytes) {}
void CMinecraftApp::GetMemFileDetails(const wstring& wName, PBYTE* ppbData, DWORD* pdwBytes) { if (ppbData) *ppbData = nullptr; if (pdwBytes) *pdwBytes = 0; }
bool CMinecraftApp::IsFileInMemoryTextures(const wstring& wName) { return false; }
void CMinecraftApp::DebugPrintf(const char* szFormat, ...) {}
void CMinecraftApp::EnterSaveNotificationSection() {}
void CMinecraftApp::LeaveSaveNotificationSection() {}
void CMinecraftApp::SetAdditionalSkinBoxes(DWORD dwSkinID, SKIN_BOX* SkinBoxA, DWORD dwSkinBoxC) {}
void CMinecraftApp::SetAnimOverrideBitmask(DWORD dwSkinID, unsigned int uiAnimOverrideBitmask) {}
unsigned int CMinecraftApp::GetAnimOverrideBitmask(DWORD dwSkinID) { return 0; }
vector<SKIN_BOX*>* CMinecraftApp::GetAdditionalSkinBoxes(DWORD dwSkinID) { return nullptr; }

void CMinecraftApp::FatalLoadError() { printf("[Server] FatalLoadError\n"); }
void CMinecraftApp::StoreLaunchData() {}
void CMinecraftApp::ExitGame() {}

void CConsoleMinecraftApp::FatalLoadError() {
    printf("[Server] FATAL: FatalLoadError called\n");
}
void CConsoleMinecraftApp::GetSaveThumbnail(PBYTE* ppbData, DWORD* pdwSize) {
    if (ppbData) *ppbData = nullptr;
    if (pdwSize) *pdwSize = 0;
}
void CConsoleMinecraftApp::SetRichPresenceContext(int iPad, int contextId) {}
void CConsoleMinecraftApp::StoreLaunchData() {}
void CConsoleMinecraftApp::ExitGame() {}
void CConsoleMinecraftApp::CaptureSaveThumbnail() {}
void CConsoleMinecraftApp::ReleaseSaveThumbnail() {}
void CConsoleMinecraftApp::GetScreenshot(int iPad, PBYTE* pbData, DWORD* pdwSize) { if (pbData) *pbData = nullptr; if (pdwSize) *pdwSize = 0; }
int CConsoleMinecraftApp::LoadLocalTMSFile(WCHAR* wchTMSFile) { return 0; }
int CConsoleMinecraftApp::LoadLocalTMSFile(WCHAR* wchTMSFile, eFileExtensionType eExt) { return 0; }
void CConsoleMinecraftApp::FreeLocalTMSFiles(eTMSFileType eType) {}
int CConsoleMinecraftApp::GetLocalTMSFileIndex(WCHAR* wchTMSFile, bool bFilenameIncludesExtension, eFileExtensionType eEXT) { return -1; }
void CConsoleMinecraftApp::TemporaryCreateGameStart() {}

bool CGameNetworkManager::IsHost() { return true; }  // server is always host
bool CGameNetworkManager::IsInSession() { return true; }
bool CGameNetworkManager::IsLeavingGame() { return false; }
bool CGameNetworkManager::SystemFlagGet(INetworkPlayer* player, int flag) { return false; }
void CGameNetworkManager::SystemFlagSet(INetworkPlayer* player, int flag) {}
INetworkPlayer* CGameNetworkManager::GetHostPlayer() { return nullptr; }
INetworkPlayer* CGameNetworkManager::GetPlayerByIndex(int idx) { return nullptr; }
INetworkPlayer* CGameNetworkManager::GetPlayerBySmallId(unsigned char id) { return nullptr; }
int CGameNetworkManager::GetPlayerCount() { return 0; }
void CGameNetworkManager::ServerReady() { printf("[Server] World loaded, entering tick loop.\n"); }
void CGameNetworkManager::ServerStopped() {}
void CGameNetworkManager::UpdateAndSetGameSessionData(INetworkPlayer* player) {}

unsigned int ColourTable::getColour(eMinecraftColour colour) { return 0xFFFFFF; }

#include "..\Client\Render\ProgressRenderer.h"
#include "..\Client\Game\Options.h"

class ServerProgressRenderer : public ProgressRenderer {
public:
    ServerProgressRenderer() : ProgressRenderer(nullptr) {}
    void progressStart(int title) override {}
    void progressStartNoAbort(int string) override {}
    void progressStage(int status) override {}
    void progressStage(wstring& text) override {}
    void progressStagePercentage(int i) override {
        static int lastPrinted = -1;
        int rounded = (i / 25) * 25;
        if (rounded != lastPrinted) {
            printf("[Server] Generating... %d%%\n", i);
            lastPrinted = rounded;
        }
    }
};

CRITICAL_SECTION ProgressRenderer::s_progress;

static ServerProgressRenderer s_serverProgress;
static char s_gameRendererMem[sizeof(GameRenderer)] = {};
static Options s_serverOptions;
static char s_skinsRepoMem[sizeof(TexturePackRepository)] = {};

static char s_minecraftMem[sizeof(Minecraft)] = {};
static bool s_minecraftInitialized = false;

static void ensureMinecraftSingleton() {
    if (!s_minecraftInitialized) {
        s_minecraftInitialized = true;
        memset(&s_serverOptions, 0, sizeof(s_serverOptions));
        s_serverOptions.difficulty = 2;
        InitializeCriticalSection(&ProgressRenderer::s_progress);
        Minecraft* mc = reinterpret_cast<Minecraft*>(s_minecraftMem);
        mc->progressRenderer = &s_serverProgress;
        mc->gameRenderer = reinterpret_cast<GameRenderer*>(s_gameRendererMem);
        mc->options = &s_serverOptions;
        mc->skins = reinterpret_cast<TexturePackRepository*>(s_skinsRepoMem);
    }
}

Minecraft* Minecraft::GetInstance() {
    ensureMinecraftSingleton();
    return reinterpret_cast<Minecraft*>(s_minecraftMem);
}
ColourTable* Minecraft::getColourTable() { return nullptr; }
unsigned int Minecraft::getCurrentTexturePackId() { return 0; }
bool Minecraft::isTutorial() { return false; }
MultiPlayerLevel* Minecraft::getLevel(int dimension) { return nullptr; }

bool CStorageManager::GetSaveUniqueNumber(int* pNum) { if (pNum) *pNum = 0; return true; }
bool CStorageManager::GetSaveUniqueFilename(char* buf) { if (buf) buf[0] = '\0'; return true; }
unsigned int CStorageManager::GetSaveSize() { return 0; }
void CStorageManager::GetSaveData(void* pData, unsigned int* pSize) { if (pSize) *pSize = 0; }
void* CStorageManager::AllocateSaveData(unsigned int size) { return malloc(size); }
void CStorageManager::SetSaveImages(unsigned char* a, unsigned long b, unsigned char* c, unsigned long d, unsigned char* e, unsigned long f) {}
CStorageManager::ESaveGameState CStorageManager::SaveSaveData(int(__cdecl* callback)(void*, bool), void* param) { return (CStorageManager::ESaveGameState)0; }
bool CStorageManager::GetSaveDisabled() { return false; }
string CStorageManager::GetMountedPath(string path) { return path; }

bool C_4JProfile::IsFullVersion() { return true; }
bool C_4JProfile::IsSignedIn(int iQuadrant) { return true; }
int C_4JProfile::AreXUIDSEqual(PlayerUID xuid1, PlayerUID xuid2) { return xuid1 == xuid2 ? 1 : 0; }
int C_4JProfile::GetPrimaryPad() { return 0; }

bool LevelGenerationOptions::checkIntersects(int x1, int y1, int z1, int x2, int y2, int z2) { return false; }
bool LevelGenerationOptions::isFeatureChunk(int x, int z, StructureFeature::EFeatureTypes type) { return false; }
bool LevelGenerationOptions::requiresBaseSave() { return false; }
bool LevelGenerationOptions::isFromDLC() { return false; }
bool LevelGenerationOptions::getuseFlatWorld() { return false; }
bool LevelGenerationOptions::hasLoadedData() { return false; }
bool LevelGenerationOptions::ready() { return true; }
unsigned char* LevelGenerationOptions::getBaseSaveData(unsigned long& size) { size = 0; return nullptr; }
void LevelGenerationOptions::deleteBaseSaveData() {}
void LevelGenerationOptions::getBiomeOverride(int biomeId, unsigned char& a, unsigned char& b) {}
Pos* LevelGenerationOptions::getSpawnPos() { return nullptr; }

void StatsCounter::setupStatBoards() {}
unsigned int StatsCounter::getTotalValue(Stat* stat) { return 0; }

Settings::Settings(File* file) {}
wstring Settings::getString(const wstring& key, const wstring& defaultValue) { return defaultValue; }
int Settings::getInt(const wstring& key, int defaultValue) { return defaultValue; }
bool Settings::getBoolean(const wstring& key, bool defaultValue) { return defaultValue; }

ConsoleInput::ConsoleInput(const wstring& msg, ConsoleInputSource* source) : msg(msg), source(source) {}

int LevelRenderer::getGlobalIndexForChunk(int x, int y, int z, int dimensionId) { return 0; }
void LevelRenderer::DestroyedTileManager::addAABBs(Level* level, AABB* box, vector<AABB*>* boxes) {}
void LevelRenderer::DestroyedTileManager::destroyingTileAt(Level* level, int x, int y, int z) {}

Options::Options() { memset(this, 0, sizeof(*this)); difficulty = 2; }

ProgressRenderer::ProgressRenderer(Minecraft* minecraft) : minecraft(minecraft), lastPercent(0), status(0), title(0), lastTime(0), noAbort(false), m_eType(eProgressStringType_ID) {}
int ProgressRenderer::getCurrentPercent() { return lastPercent; }
int ProgressRenderer::getCurrentTitle() { return title; }
int ProgressRenderer::getCurrentStatus() { return status; }
wstring& ProgressRenderer::getProgressString() { return m_wstrText; }
ProgressRenderer::eProgressStringType ProgressRenderer::getType() { return m_eType; }
void ProgressRenderer::setType(eProgressStringType eType) { m_eType = eType; }
void ProgressRenderer::progressStart(int title) { this->title = title; }
void ProgressRenderer::progressStartNoAbort(int string) { this->title = string; noAbort = true; }
void ProgressRenderer::_progressStart(int title) { this->title = title; }
void ProgressRenderer::progressStage(int status) { this->status = status; }
void ProgressRenderer::progressStage(wstring& wstrText) { m_wstrText = wstrText; }
void ProgressRenderer::progressStagePercentage(int i) { lastPercent = i; }

void GameRenderer::DisableUpdateThread() {}

TexturePack* TexturePackRepository::DEFAULT_TEXTURE_PACK = nullptr;
TexturePack* TexturePackRepository::getSelected() { return DEFAULT_TEXTURE_PACK; }

LevelGenerationOptions* GameRuleManager::loadGameRules(byte* dIn, UINT dSize) { return nullptr; }
void GameRuleManager::saveGameRules(byte** dOut, UINT* dSize) { if (dOut) *dOut = nullptr; if (dSize) *dSize = 0; }
void GameRuleManager::unloadCurrentGameRules() {}

void GameRule::onCollectItem(shared_ptr<ItemInstance> item) {}
void GameRule::onUseTile(int tileId, int x, int y, int z) {}
void GameRule::write(DataOutputStream* dos) {}
void GameRule::read(DataInputStream* dis) {}

GameRulesInstance* GameRuleDefinition::generateNewGameRulesInstance(GameRulesInstance::EGameRulesInstanceType type, LevelRuleset* rules, Connection* connection) { return nullptr; }

void ConsoleSchematicFile::generateSchematicFile(DataOutputStream* dos, Level* level, int xStart, int yStart, int zStart, int xEnd, int yEnd, int zEnd, bool bSaveMobs, Compression::ECompressionTypes compressionType) {}

vector<SKIN_BOX*>* DLCSkinFile::getAdditionalBoxes() { return nullptr; }
int DLCSkinFile::getAdditionalBoxesCount() { return 0; }

bool DLCPack::hasPurchasedFile(DLCManager::EDLCType type, const wstring& path) { return false; }

HRESULT CTelemetryManager::Init() { return S_OK; }
HRESULT CTelemetryManager::Tick() { return S_OK; }
HRESULT CTelemetryManager::Flush() { return S_OK; }
bool CTelemetryManager::RecordPlayerSessionStart(int iPad) { return true; }
bool CTelemetryManager::RecordPlayerSessionExit(int iPad, int exitStatus) { return true; }
bool CTelemetryManager::RecordHeartBeat(int iPad) { return true; }
bool CTelemetryManager::RecordLevelStart(int iPad, ESen_FriendOrMatch, ESen_CompeteOrCoop, int, int, int) { return true; }
bool CTelemetryManager::RecordLevelExit(int iPad, ESen_LevelExitStatus) { return true; }
bool CTelemetryManager::RecordLevelSaveOrCheckpoint(int iPad, int, int) { return true; }
bool CTelemetryManager::RecordLevelResume(int iPad, ESen_FriendOrMatch, ESen_CompeteOrCoop, int, int, int, int) { return true; }
bool CTelemetryManager::RecordPauseOrInactive(int iPad) { return true; }
bool CTelemetryManager::RecordUnpauseOrActive(int iPad) { return true; }
bool CTelemetryManager::RecordMenuShown(int iPad, EUIScene, int) { return true; }
bool CTelemetryManager::RecordAchievementUnlocked(int iPad, int, int) { return true; }
bool CTelemetryManager::RecordMediaShareUpload(int iPad, ESen_MediaDestination, ESen_MediaType) { return true; }
bool CTelemetryManager::RecordUpsellPresented(int iPad, ESen_UpsellID, int) { return true; }
bool CTelemetryManager::RecordUpsellResponded(int iPad, ESen_UpsellID, int, ESen_UpsellOutcome) { return true; }
bool CTelemetryManager::RecordPlayerDiedOrFailed(int iPad, int, int, int, int, int, int, ETelemetryChallenges) { return true; }
bool CTelemetryManager::RecordEnemyKilledOrOvercome(int iPad, int, int, int, int, int, int, ETelemetryChallenges) { return true; }
bool CTelemetryManager::RecordTexturePackLoaded(int iPad, int, bool) { return true; }
bool CTelemetryManager::RecordSkinChanged(int iPad, int) { return true; }
bool CTelemetryManager::RecordBanLevel(int iPad) { return true; }
bool CTelemetryManager::RecordUnBanLevel(int iPad) { return true; }
int CTelemetryManager::GetMultiplayerInstanceID() { return 0; }
int CTelemetryManager::GenerateMultiplayerInstanceId() { return 0; }
void CTelemetryManager::SetMultiplayerInstanceId(int) {}

EGameCommand TeleportCommand::getId() { return (EGameCommand)0; }
void TeleportCommand::execute(shared_ptr<CommandSender> source, byteArray commandData) {}
