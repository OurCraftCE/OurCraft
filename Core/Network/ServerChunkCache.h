#pragma once
#include "net.minecraft.world.level.h"
#include "File.h"
#include "net.minecraft.world.level.storage.h"
#include "JavaIntHash.h"
#include "RandomLevelSource.h"
#include "C4JThread.h"
#include "ChunkGenThreadPool.h"
#include <unordered_map>
#include <unordered_set>
using namespace std;

static inline int64_t chunkKey(int x, int z) {
    return ((int64_t)x << 32) | ((int64_t)z & 0xFFFFFFFF);
}

class ServerLevel;

class ServerChunkCache : public ChunkSource
{

private:
//	unordered_set<int,IntKeyHash, IntKeyEq> toDrop;
private:
	LevelChunk *emptyChunk;
    ChunkSource *source;
    ChunkStorage *storage;
public:
	bool autoCreate;
private:
	std::unordered_map<int64_t, LevelChunk*> m_chunkMap;
	std::unordered_map<int64_t, LevelChunk*> m_unloadedCache;
	std::deque<LevelChunk*> m_toDrop;
    vector<LevelChunk *> m_loadedChunkList;
    ServerLevel *level;

	// 4J - added for multithreaded support
	CRITICAL_SECTION m_csLoadCreate;

public:
	ServerChunkCache(ServerLevel *level, ChunkStorage *storage, ChunkSource *source);
	virtual ~ServerChunkCache();
    virtual bool hasChunk(int x, int z);
	vector<LevelChunk *> *getLoadedChunkList();
    void drop(int x, int z);
	void dropAll();
    virtual LevelChunk *create(int x, int z);
	LevelChunk *create(int x, int z, bool asyncPostProcess );	// 4J added
    virtual LevelChunk *getChunk(int x, int z);
	LevelChunk *getChunkLoadedOrUnloaded(int x, int z);		// 4J added
	const std::unordered_map<int64_t, LevelChunk*>& getChunkMap() const { return m_chunkMap; }

	// 4J-JEV Added; Remove chunk from the toDrop queue.
	void dontDrop(int x, int z);

private:
	 LevelChunk *load(int x, int z);
    void saveEntities(LevelChunk *levelChunk);
    void save(LevelChunk *levelChunk);

	void updatePostProcessFlag(short flag, int x, int z, int xo, int zo, LevelChunk *lc);	// 4J added
	void updatePostProcessFlags(int x, int z);								// 4J added
	void flagPostProcessComplete(short flag, int x, int z);					// 4J added
public:
	virtual void postProcess(ChunkSource *parent, int x, int z);


private:
static const int MAX_SAVES = 20;
public:
	virtual bool saveAllEntities();
	virtual bool save(bool force, ProgressListener *progressListener);
    virtual bool tick();
    virtual bool shouldSave();
    virtual wstring gatherStats();

	virtual vector<Biome::MobSpawnerData *> *getMobsAt(MobCategory *mobCategory, int x, int y, int z);
	virtual TilePos *findNearestMapFeature(Level *level, const wstring &featureName, int x, int y, int z);

	// Async chunk generation support
	ChunkGenThreadPool* m_genThreadPool;
	std::unordered_set<int64_t> m_generatingSet;  // Chunks currently being generated async
	int m_dimension;

	void integrateCompletedChunks(int maxPerTick);

public:
	// Initialize async generation (call after constructor)
	void initAsyncGeneration(int dimension, __int64 seed, int numThreads = 0);
	void shutdownAsyncGeneration();

	// Queue a chunk for async generation
	void queueAsyncGeneration(int x, int z, int priority);

	// Update player position for priority re-ordering
	void updatePlayerPosition(int chunkX, int chunkZ, int viewDistance);

	// Check if a chunk is being generated
	bool isGenerating(int x, int z) const;

	// Get async generation stats
	int getAsyncPendingCount() const { return m_genThreadPool ? m_genThreadPool->GetPendingCount() : 0; }
	int getAsyncActiveCount() const { return m_genThreadPool ? m_genThreadPool->GetActiveCount() : 0; }

private:
	typedef struct _SaveThreadData
	{
		ServerChunkCache *cache;
		LevelChunk *chunkToSave;
		bool saveEntities;
		bool useSharedThreadStorage;
		C4JThread::Event *notificationEvent;
		C4JThread::Event *wakeEvent; // This is a handle to the one fired by the producer thread
	} SaveThreadData;

public:
	static int runSaveThreadProc(LPVOID lpParam);
};
