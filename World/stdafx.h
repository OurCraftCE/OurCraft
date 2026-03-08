// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#pragma once

#define AUTO_VAR(_var, _val) auto _var = _val

typedef unsigned __int64 __uint64;

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <malloc.h>
#include <tchar.h>
// TODO: reference additional headers your program requires here
#include <d3d11.h>


#include <unordered_map>
#include <unordered_set>
#include <sal.h>
#include <vector>

#include <memory>

#include <list>
#include <map>
#include <set>
#include <queue>
#include <deque>
#include <algorithm>
#include <math.h>
#include <limits>
#include <string>
#include <sstream>
#include <iostream>
#include <exception>

#include <assert.h>

#include "extraX64.h"

#include "Math/Definitions.h"
#include "Math/Class.h"
#include "IO/Exceptions.h"
#include "Math/Mth.h"
#include "Math/StringHelpers.h"
#include "Math/ArrayWithLength.h"
#include "Math/Random.h"
#include "Math/TilePos.h"
#include "Level/ChunkPos.h"
#include "IO/compression.h"
#include "Math/PerformanceTimer.h"


#ifdef _FINAL_BUILD
#define printf BREAKTHECOMPILE
#define wprintf BREAKTHECOMPILE
#undef OutputDebugString
#define OutputDebugString BREAKTHECOMPILE
#define OutputDebugStringA BREAKTHECOMPILE
#define OutputDebugStringW BREAKTHECOMPILE
#endif


void MemSect(int sect);

#include "..\Client\Platform\Profile.h"
#include "..\Client\Platform\BgfxRenderer.h"
#include "..\Client\Platform\StorageManager.h"
#include "..\Client\Platform\SDLInput.h"

#include "..\Client\Common\Network\GameNetworkManager.h"

// #ifdef _XBOX
#include "..\Client\Common\UI\UIEnums.h"
#include "..\Client\Common\App_defines.h"
#include "..\Client\Common\App_enums.h"
#include "..\Client\Common\Tutorial\TutorialEnum.h"
#include "..\Client\Common\App_structs.h"
//#endif

#include "..\Client\Common\Consoles_App.h"
#include "..\Client\Common\Minecraft_Macros.h"
#include "..\Client\Common\Colours\ColourTable.h"

#include "..\Client\Common\BuildVer.h"

#include "..\Client\Windows64\Windows64_App.h"
#include "..\Client\Windows64Media\strings.h"
#include "..\Client\Windows64\Sentient\SentientTelemetryCommon.h"
#include "..\Client\Windows64\Sentient\MinecraftTelemetry.h"

#include "..\Client\Common\DLC\DLCSkinFile.h"
#include "..\Client\Common\Console_Awards_enum.h"
#include "..\Client\Common\Potion_Macros.h"
#include "..\Client\Common\Console_Debug_enum.h"
#include "..\Client\Common\GameRules\ConsoleGameRulesConstants.h"
#include "..\Client\Common\GameRules\ConsoleGameRules.h"
#include "..\Client\Common\Telemetry\TelemetryManager.h"
