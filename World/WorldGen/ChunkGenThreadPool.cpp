#include "stdafx.h"
#include "ChunkGenThreadPool.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.biome.h"
#include "net.minecraft.world.level.levelgen.h"
#include "net.minecraft.world.level.levelgen.feature.h"
#include "net.minecraft.world.level.levelgen.structure.h"
#include "net.minecraft.world.level.levelgen.synth.h"
#include "net.minecraft.world.level.tile.h"
#include "net.minecraft.world.level.storage.h"
#include "RandomLevelSource.h"
#include "HellRandomLevelSource.h"
#include "../Level/TheEndLevelRandomLevelSource.h"

#define CPU_CORE_CHUNKGEN_BASE 2

ChunkGenThreadPool::ChunkGenThreadPool()
	: m_shutdown(false)
	, m_initialized(false)
	, m_level(NULL)
	, m_seed(0)
	, m_dimension(0)
	, m_playerChunkX(0)
	, m_playerChunkZ(0)
	, m_activeCount(0)
	, m_workAvailable(NULL)
{
	InitializeCriticalSectionAndSpinCount(&m_queueCS, 4000);
	InitializeCriticalSectionAndSpinCount(&m_completedCS, 4000);
}

ChunkGenThreadPool::~ChunkGenThreadPool()
{
	Shutdown();
	DeleteCriticalSection(&m_queueCS);
	DeleteCriticalSection(&m_completedCS);
}

void ChunkGenThreadPool::Initialize(int numThreads)
{
	if (m_initialized) return;

	if (numThreads <= 0)
	{
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		numThreads = (int)sysInfo.dwNumberOfProcessors - 2;
		if (numThreads < 1) numThreads = 1;
		if (numThreads > 4) numThreads = 4; // Cap to avoid oversubscription
	}

	m_shutdown = false;
	m_workAvailable = new C4JThread::Event(C4JThread::Event::e_modeManualClear);

	m_workers.resize(numThreads);
	for (int i = 0; i < numThreads; i++)
	{
		WorkerThreadParam* param = new WorkerThreadParam();
		param->pool = this;
		param->workerIndex = i;

		char threadName[64];
		sprintf_s(threadName, 64, "ChunkGen%d", i);

		m_workers[i].index = i;
		m_workers[i].running = true;
		m_workers[i].thread = new C4JThread(WorkerThread, param, threadName);
		m_workers[i].thread->SetProcessor(CPU_CORE_CHUNKGEN_BASE + (i % 4));
		m_workers[i].thread->Run();
	}

	m_initialized = true;
}

void ChunkGenThreadPool::Shutdown()
{
	if (!m_initialized) return;

	m_shutdown = true;

	// Wake all workers so they can exit
	if (m_workAvailable)
		m_workAvailable->Set();

	for (size_t i = 0; i < m_workers.size(); i++)
	{
		if (m_workers[i].thread)
		{
			m_workers[i].thread->WaitForCompletion(5000);
			delete m_workers[i].thread;
			m_workers[i].thread = NULL;
		}
	}
	m_workers.clear();

	// Clean up completed chunks that were never polled
	EnterCriticalSection(&m_completedCS);
	for (size_t i = 0; i < m_completedChunks.size(); i++)
	{
		delete m_completedChunks[i];
	}
	m_completedChunks.clear();
	LeaveCriticalSection(&m_completedCS);

	EnterCriticalSection(&m_queueCS);
	while (!m_pendingQueue.empty()) m_pendingQueue.pop();
	m_pendingSet.clear();
	LeaveCriticalSection(&m_queueCS);

	if (m_workAvailable)
	{
		delete m_workAvailable;
		m_workAvailable = NULL;
	}

	m_initialized = false;
}

void ChunkGenThreadPool::SetLevel(Level* level, __int64 seed, int dimension)
{
	m_level = level;
	m_seed = seed;
	m_dimension = dimension;
}

int64_t ChunkGenThreadPool::MakeChunkKey(int x, int z, int dim)
{
	// Combine x, z, and dim into a single key
	// Use lower 28 bits for x and z, upper 8 for dim
	return ((int64_t)(dim & 0xFF) << 56) | ((int64_t)(x & 0x0FFFFFFF) << 28) | ((int64_t)(z & 0x0FFFFFFF));
}

void ChunkGenThreadPool::QueueChunk(int chunkX, int chunkZ, int dimension, int priority)
{
	EnterCriticalSection(&m_queueCS);

	int64_t key = MakeChunkKey(chunkX, chunkZ, dimension);
	if (m_pendingSet.count(key) > 0)
	{
		LeaveCriticalSection(&m_queueCS);
		return; // Already queued
	}

	ChunkGenTask task;
	task.chunkX = chunkX;
	task.chunkZ = chunkZ;
	task.priority = priority;
	task.dimension = dimension;

	m_pendingQueue.push(task);
	m_pendingSet.insert(key);

	LeaveCriticalSection(&m_queueCS);

	// Signal workers
	m_workAvailable->Set();
}

void ChunkGenThreadPool::CancelChunk(int chunkX, int chunkZ, int dimension)
{
	EnterCriticalSection(&m_queueCS);
	int64_t key = MakeChunkKey(chunkX, chunkZ, dimension);
	m_pendingSet.erase(key);
	// Note: The task stays in the priority queue but will be skipped when dequeued
	// because it won't be in m_pendingSet anymore
	LeaveCriticalSection(&m_queueCS);
}

void ChunkGenThreadPool::UpdatePlayerPosition(int playerChunkX, int playerChunkZ)
{
	EnterCriticalSection(&m_queueCS);

	m_playerChunkX = playerChunkX;
	m_playerChunkZ = playerChunkZ;

	// Rebuild the priority queue with updated distances
	std::priority_queue<ChunkGenTask, std::vector<ChunkGenTask>, std::greater<ChunkGenTask>> newQueue;
	while (!m_pendingQueue.empty())
	{
		ChunkGenTask task = m_pendingQueue.top();
		m_pendingQueue.pop();

		int64_t key = MakeChunkKey(task.chunkX, task.chunkZ, task.dimension);
		if (m_pendingSet.count(key) > 0)
		{
			int dx = task.chunkX - playerChunkX;
			int dz = task.chunkZ - playerChunkZ;
			task.priority = dx * dx + dz * dz;
			newQueue.push(task);
		}
	}
	m_pendingQueue = newQueue;

	LeaveCriticalSection(&m_queueCS);
}

LevelChunk* ChunkGenThreadPool::PollCompleted()
{
	LevelChunk* result = NULL;

	EnterCriticalSection(&m_completedCS);
	if (!m_completedChunks.empty())
	{
		result = m_completedChunks.back();
		m_completedChunks.pop_back();
	}
	LeaveCriticalSection(&m_completedCS);

	return result;
}

int ChunkGenThreadPool::GetCompletedCount() const
{
	return (int)m_completedChunks.size();
}

int ChunkGenThreadPool::GetPendingCount() const
{
	return (int)m_pendingQueue.size();
}

int ChunkGenThreadPool::GetActiveCount() const
{
	return (int)m_activeCount;
}

bool ChunkGenThreadPool::IsChunkPending(int chunkX, int chunkZ, int dimension) const
{
	int64_t key = MakeChunkKey(chunkX, chunkZ, dimension);
	return m_pendingSet.count(key) > 0;
}

int ChunkGenThreadPool::CalculateChunkPriority(int chunkX, int chunkZ, int playerChunkX, int playerChunkZ)
{
	int dx = chunkX - playerChunkX;
	int dz = chunkZ - playerChunkZ;
	return dx * dx + dz * dz; // Squared distance = natural priority
}

void ChunkGenThreadPool::RequestChunksAroundPlayer(int cx, int cz, int viewDistance, int dimension)
{
	// Generate in spiral order from center outward
	// This ensures closer chunks are queued first with lower priority values
	for (int r = 0; r <= viewDistance; r++)
	{
		for (int x = -r; x <= r; x++)
		{
			for (int z = -r; z <= r; z++)
			{
				if (abs(x) == r || abs(z) == r) // Only the ring at distance r
				{
					int priority = CalculateChunkPriority(cx + x, cz + z, cx, cz);
					QueueChunk(cx + x, cz + z, dimension, priority);
				}
			}
		}
	}
}

void ChunkGenThreadPool::CancelChunksOutOfRange(int playerChunkX, int playerChunkZ, int maxDistance, int dimension)
{
	int maxDistSq = maxDistance * maxDistance;

	EnterCriticalSection(&m_queueCS);

	// Rebuild queue, dropping entries that are too far
	std::priority_queue<ChunkGenTask, std::vector<ChunkGenTask>, std::greater<ChunkGenTask>> newQueue;
	while (!m_pendingQueue.empty())
	{
		ChunkGenTask task = m_pendingQueue.top();
		m_pendingQueue.pop();

		int64_t key = MakeChunkKey(task.chunkX, task.chunkZ, task.dimension);
		if (m_pendingSet.count(key) == 0)
			continue; // Already cancelled

		int dx = task.chunkX - playerChunkX;
		int dz = task.chunkZ - playerChunkZ;
		int distSq = dx * dx + dz * dz;

		if (distSq > maxDistSq)
		{
			m_pendingSet.erase(key); // Cancel it
		}
		else
		{
			newQueue.push(task);
		}
	}
	m_pendingQueue = newQueue;

	LeaveCriticalSection(&m_queueCS);
}

int ChunkGenThreadPool::WorkerThread(void* param)
{
	WorkerThreadParam* wtp = (WorkerThreadParam*)param;
	wtp->pool->WorkerLoop(wtp->workerIndex);
	delete wtp;
	return 0;
}

LevelChunk* ChunkGenThreadPool::GenerateChunk(const ChunkGenTask& task)
{
	if (m_level == NULL) return NULL;

	// Each call creates a thread-local LevelSource with the same seed
	// This ensures complete thread safety - no shared mutable state
	ChunkSource* localSource = NULL;
	if (task.dimension == 0)
	{
		localSource = new RandomLevelSource(m_level, m_seed, true);
	}
	else if (task.dimension == -1)
	{
		localSource = new HellRandomLevelSource(m_level, m_seed);
	}
	else if (task.dimension == 1)
	{
		localSource = new TheEndLevelRandomLevelSource(m_level, m_seed);
	}

	if (localSource == NULL) return NULL;

	LevelChunk* chunk = localSource->getChunk(task.chunkX, task.chunkZ);

	delete localSource;
	return chunk;
}

void ChunkGenThreadPool::WorkerLoop(int workerIndex)
{
	while (!m_shutdown)
	{
		// Wait for work (with timeout so we can check shutdown)
		m_workAvailable->WaitForSignal(100);

		if (m_shutdown) break;

		ChunkGenTask task;
		bool hasTask = false;

		EnterCriticalSection(&m_queueCS);
		while (!m_pendingQueue.empty())
		{
			task = m_pendingQueue.top();
			m_pendingQueue.pop();

			// Check if this task was cancelled
			int64_t key = MakeChunkKey(task.chunkX, task.chunkZ, task.dimension);
			if (m_pendingSet.count(key) > 0)
			{
				m_pendingSet.erase(key);
				hasTask = true;
				break;
			}
			// Otherwise skip cancelled task and try next
		}

		// If queue is now empty, clear the event
		if (m_pendingQueue.empty())
			m_workAvailable->Clear();

		LeaveCriticalSection(&m_queueCS);

		if (hasTask)
		{
			InterlockedIncrement(&m_activeCount);

			LevelChunk* chunk = GenerateChunk(task);

			if (chunk != NULL)
			{
				EnterCriticalSection(&m_completedCS);
				m_completedChunks.push_back(chunk);
				LeaveCriticalSection(&m_completedCS);
			}

			InterlockedDecrement(&m_activeCount);
		}
	}
}
