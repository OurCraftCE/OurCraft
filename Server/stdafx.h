// Minimal precompiled header for Server
// Provides the same types as Client stdafx but without renderer/UI/audio deps
#pragma once

#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__)") : Warning: "
#define AUTO_VAR(_var, _val) auto _var = _val

#include <unordered_map>
#include <unordered_set>
#include <vector>
typedef unsigned __int64 __uint64;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <malloc.h>
#include <tchar.h>
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;

#define HRESULT_SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)

#include "extraX64.h"

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

#include "Definitions.h"
#include "Class.h"
#include "ArrayWithLength.h"
#include "SharedConstants.h"
#include "Random.h"
#include "compression.h"
#include "PerformanceTimer.h"

void MemSect(int sect);
