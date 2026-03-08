#pragma once
#include "../Math/C4JThread.h"
#include <queue>
#include <functional>
#include <vector>
#include <unordered_set>

class LevelChunk;
class Level;
class ChunkSource;

struct ChunkGenTask {
	int chunkX, chunkZ;
	int priority;       // Lower = higher priority (distance from player)
	int dimension;

	bool operator>(const ChunkGenTask& other) const {
		return priority > other.priority;
	}
};

class ChunkGenThreadPool
{
public:
	ChunkGenThreadPool();
	~ChunkGenThreadPool();

	void Initialize(int numThreads = 0); // 0 = auto (cores - 2, min 1)
	void Shutdown();

	// Set the level and source used for generation (must be set before queuing)
	void SetLevel(Level* level, __int64 seed, int dimension);

	// Queue a chunk for generation
	void QueueChunk(int chunkX, int chunkZ, int dimension, int priority);

	// Cancel pending generation (e.g., player moved away)
	void CancelChunk(int chunkX, int chunkZ, int dimension);

	// Update priorities when player moves to a new chunk
	void UpdatePlayerPosition(int playerChunkX, int playerChunkZ);

	// Poll for completed chunks (call from main thread)
	LevelChunk* PollCompleted();
	int GetCompletedCount() const;

	// Stats
	int GetPendingCount() const;
	int GetActiveCount() const;

	// Generate chunks in spiral order around player
	void RequestChunksAroundPlayer(int cx, int cz, int viewDistance, int dimension);

	// Check if a chunk is already queued or being generated
	bool IsChunkPending(int chunkX, int chunkZ, int dimension) const;

private:
	static int WorkerThread(void* param);
	void WorkerLoop(int workerIndex);

	struct WorkerState {
		C4JThread* thread;
		bool running;
		int index;
	};

	std::vector<WorkerState> m_workers;
	std::priority_queue<ChunkGenTask, std::vector<ChunkGenTask>, std::greater<ChunkGenTask>> m_pendingQueue;
	std::vector<LevelChunk*> m_completedChunks;

	// Track which chunks are pending or in-progress
	std::unordered_set<int64_t> m_pendingSet;

	CRITICAL_SECTION m_queueCS;
	CRITICAL_SECTION m_completedCS;
	C4JThread::Event* m_workAvailable;
	bool m_shutdown;
	bool m_initialized;

	Level* m_level;
	__int64 m_seed;
	int m_dimension;

	int m_playerChunkX;
	int m_playerChunkZ;

	volatile long m_activeCount;

	static int64_t MakeChunkKey(int x, int z, int dim);
	LevelChunk* GenerateChunk(const ChunkGenTask& task);

	struct WorkerThreadParam {
		ChunkGenThreadPool* pool;
		int workerIndex;
	};

public:
	// Calculate priority from squared distance to player
	static int CalculateChunkPriority(int chunkX, int chunkZ, int playerChunkX, int playerChunkZ);

	// Cancel all chunks beyond a given distance from player
	void CancelChunksOutOfRange(int playerChunkX, int playerChunkZ, int maxDistance, int dimension);
};
