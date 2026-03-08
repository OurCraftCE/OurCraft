#pragma once
#include "../../World/FwdHeaders/net.minecraft.world.level.h"
#include "../../World/FwdHeaders/net.minecraft.world.level.chunk.h"
#include "../../World/WorldGen\RandomLevelSource.h"
#include <unordered_map>
#include <unordered_set>

using namespace std;
class ServerChunkCache;

// 4J - various alterations here to make this thread safe, and operate as a fixed sized cache
class MultiPlayerChunkCache : public ChunkSource
{
	friend class LevelRenderer;
private:
	LevelChunk *emptyChunk;
	LevelChunk *waterChunk;

	vector<LevelChunk *> loadedChunkList;

	std::unordered_map<int64_t, LevelChunk*> m_chunkMap;
	// 4J - added for multithreaded support
	CRITICAL_SECTION m_csLoadCreate;
	std::unordered_set<int64_t> m_hasData;

    Level *level;

public:
	MultiPlayerChunkCache(Level *level);
	~MultiPlayerChunkCache();
    virtual bool hasChunk(int x, int z);
	virtual bool reallyHasChunk(int x, int z);
    virtual void drop(int x, int z);
    virtual LevelChunk *create(int x, int z);
    virtual LevelChunk *getChunk(int x, int z);
    virtual bool save(bool force, ProgressListener *progressListener);
    virtual bool tick();
    virtual bool shouldSave();
    virtual void postProcess(ChunkSource *parent, int x, int z);
    virtual wstring gatherStats();
	virtual vector<Biome::MobSpawnerData *> *getMobsAt(MobCategory *mobCategory, int x, int y, int z);
	virtual TilePos *findNearestMapFeature(Level *level, const wstring &featureName, int x, int y, int z);
	virtual void dataReceived(int x, int z);	// 4J added

	const std::unordered_map<int64_t, LevelChunk*>& getChunkMap() const { return m_chunkMap; }
};
