// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__)") : 4J Warning Msg: "

// use  - #pragma message(__LOC__"Need to do something here")

#define AUTO_VAR(_var, _val) auto _var = _val
#include <unordered_map>
#include <unordered_set>
#include <vector>
typedef unsigned __int64 __uint64;

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <malloc.h>
#include <tchar.h>
// TODO: reference additional headers your program requires here
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;

#define HRESULT_SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)

#ifdef _DURANGO
#include <xdk.h>
#include <wrl.h>
#include <d3d11_x.h>
#include <DirectXMath.h>
#include <ppltasks.h>
#include <collection.h>
using namespace DirectX;
#include <pix.h>
#endif

#include "extraX64.h"
#elif defined(__EMSCRIPTEN__)
// WASM — Win32 type stubs + Emscripten extras
#include "Platform/WasmCompat.h"
#include "extraX64.h"
#elif defined(__linux__) || defined(__APPLE__)
// Linux / macOS — Win32 type stubs
#include "Platform/PlatformCompat.h"
#include "extraX64.h"
#endif // _WIN32 / __EMSCRIPTEN__ / __linux__ / __APPLE__

// C RunTime Header Files
#include <stdlib.h>

#include <memory>

#include <list>
#include <map>
#include <set>
#include <deque>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include <exception>

#include <assert.h>

#include "..\World\Math\Definitions.h"
#include "..\World\Math\Class.h"
#include "..\World\Math\ArrayWithLength.h"
#include "..\World\Math\SharedConstants.h"
#include "..\World\Math\Random.h"
#include "..\World\IO\compression.h"
#include "..\World\Math\PerformanceTimer.h"
#include "..\World\Math\Mth.h"
#include "..\World\Math\StringHelpers.h"

#ifdef _DURANGO
	#include "Durango\4JLibs\inc\4J_Input.h"
	#include "Durango\4JLibs\inc\4J_Render.h"
#else
	#include "Platform\SDLInput.h"
	#include "Platform\BgfxRenderer.h"
	#include "Platform\Profile.h"
	#include "Platform\StorageManager.h"
#endif

#include "Render\Textures.h"
#include "Textures\Font.h"
#include "Network\ClientConstants.h"
#include "UI\Gui.h"
#include "Screens\Screen.h"
#include "UI\ScreenSizeCalculator.h"
#include "OurCraft.h"
#include "Game\MemoryTracker.h"
#include "stubs.h"
#include "Textures\BufferedImage.h"

#include "Common\Network\GameNetworkManager.h"

#include "Common\UI\UIEnums.h"
#include "Common\UI\UIStructs.h"
#include "Common\App_defines.h"
#include "Common\App_enums.h"
#include "Common\Tutorial\TutorialEnum.h"
#include "Common\App_structs.h"

#include "Common\Consoles_App.h"
#include "Common\Minecraft_Macros.h"
#include "Common\BuildVer.h"
#include "Common\Potion_Macros.h"

#ifdef _DURANGO
	#include "Durango\Sentient\MinecraftTelemetry.h"
	#include "DurangoMedia\strings.h"
	#include "Durango\Durango_App.h"
	#include "Durango\Sentient\DynamicConfigurations.h"
	#include "Durango\Sentient\TelemetryEnum.h"
	#include "Durango\Sentient\SentientTelemetryCommon.h"
	#include "DurangoMedia\4J_strings.h"
	#include "Durango\XML\ATGXmlParser.h"
	#include "Common\Audio\SoundEngine.h"
	#include "Durango\Iggy\include\iggy.h"
	#include "Durango\Iggy\gdraw\gdraw_d3d11.h"
	#include "Durango\Durango_UIController.h"
#elif defined(__EMSCRIPTEN__)
	// WASM: minimal platform, no Xbox/Win32-specific app framework
	#include "Platform/WasmApp.h"
	#include "Common/Audio/SoundEngine.h"
#elif defined(__linux__) || defined(__APPLE__)
	// Linux / macOS: shared Unix app stub
	#include "Platform/UnixApp.h"
	#include "Common/Audio/SoundEngine.h"
#else
	#include "Windows64\Sentient\MinecraftTelemetry.h"
	#include "Windows64Media\strings.h"
	#include "Windows64\Windows64_App.h"
	#include "Windows64\Sentient\DynamicConfigurations.h"
	#include "Windows64\Sentient\SentientTelemetryCommon.h"
	#include "Windows64\GameConfig\Minecraft.spa.h"
	#include "Windows64\XML\ATGXmlParser.h"
	#include "Windows64\Social\SocialManager.h"
	#include "Common\Audio\SoundEngine.h"
	#include "Windows64\Iggy\include\iggy.h"
	#include "Windows64\Iggy\gdraw\gdraw_d3d11.h"
	#include "Windows64\Windows64_UIController.h"
#endif

#include "Common\ConsoleGameMode.h"
#include "Common\Console_Debug_enum.h"
#include "Common\Console_Awards_enum.h"
#include "Common\Tutorial\TutorialMode.h"
#include "Common\Tutorial\Tutorial.h"
#include "Common\Tutorial\FullTutorialMode.h"
#include "Common\Trial\TrialMode.h"
#include "Common\GameRules\ConsoleGameRules.h"
#include "Common\GameRules\ConsoleSchematicFile.h"
#include "Common\Colours\ColourTable.h"
#include "Common\DLC\DLCSkinFile.h"
#include "Common\DLC\DLCManager.h"
#include "Common\DLC\DLCPack.h"
#include "Common\Telemetry\TelemetryManager.h"

#include "Common\ShutdownManager.h"
#include "extraX64client.h"



#ifdef _FINAL_BUILD
#define printf BREAKTHECOMPILE
#define wprintf BREAKTHECOMPILE
#undef OutputDebugString
#define OutputDebugString BREAKTHECOMPILE
#define OutputDebugStringA BREAKTHECOMPILE
#define OutputDebugStringW BREAKTHECOMPILE
#endif

void MemSect(int sect);
