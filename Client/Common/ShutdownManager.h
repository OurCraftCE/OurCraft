#pragma once
#include <Windows.h>

// Stub ShutdownManager - originally PS3, now provides thread lifecycle stubs
class ShutdownManager
{
public:
	enum eThreadType
	{
		eEventQueueThreads = 0,
		eConnectionReadThreads,
		eConnectionWriteThreads,
		eRenderChunkUpdateThread,
		ePostProcessThread,
		eServerThread,
		eRunUpdateThread,
		eThreadType_MAX
	};

	static void HasStarted(eThreadType type, HANDLE event = NULL) {}
	static void HasFinished(eThreadType type) {}
	static bool ShouldRun(eThreadType type) { return true; }
};
