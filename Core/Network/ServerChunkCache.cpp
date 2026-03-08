#include "stdafx.h"
#include "ServerChunkCache.h"
#include "ServerLevel.h"
#include "MinecraftServer.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.dimension.h"
#include "net.minecraft.world.level.storage.h"
#include "net.minecraft.world.level.chunk.h"
#include "Pos.h"
#include "ProgressListener.h"
#include "ThreadName.h"
#include "compression.h"
#include "OldChunkStorage.h"
#include <sstream>

ServerChunkCache::ServerChunkCache(ServerLevel *level, ChunkStorage *storage, ChunkSource *source)
{
	autoCreate = false;
	emptyChunk = new EmptyLevelChunk(level, byteArray(Level::CHUNK_TILE_COUNT), 0, 0);
	this->level = level;
	this->storage = storage;
	this->source = source;
	this->m_XZSize = source->m_XZSize;
	this->m_genThreadPool = NULL;
	this->m_dimension = 0;

	m_chunkMap.reserve(4096);
	m_unloadedCache.reserve(1024);

	InitializeCriticalSectionAndSpinCount(&m_csLoadCreate, 4000);
}

ServerChunkCache::~ServerChunkCache()
{
	shutdownAsyncGeneration();

	for (auto& pair : m_unloadedCache)
	{
		delete pair.second;
	}
	m_unloadedCache.clear();

	for (auto it = m_loadedChunkList.begin(); it != m_loadedChunkList.end(); ++it)
	{
		delete *it;
	}
	m_loadedChunkList.clear();
	m_chunkMap.clear();

	delete emptyChunk; emptyChunk = 0;
	delete source; source = 0;

	DeleteCriticalSection(&m_csLoadCreate);
}

bool ServerChunkCache::hasChunk(int x, int z)
{
	int64_t key = chunkKey(x, z);
	return m_chunkMap.count(key) > 0;
}

vector<LevelChunk *> *ServerChunkCache::getLoadedChunkList()
{
	return &m_loadedChunkList;
}

void ServerChunkCache::drop(int x, int z)
{
	int64_t key = chunkKey(x, z);
	auto it = m_chunkMap.find(key);
	if (it != m_chunkMap.end() && it->second != nullptr)
	{
		m_toDrop.push_back(it->second);
	}
}

void ServerChunkCache::dropAll()
{
	for (auto it = m_loadedChunkList.begin(); it != m_loadedChunkList.end(); ++it)
	{
		LevelChunk* chunk = *it;
		drop(chunk->x, chunk->z);
	}
}

// 4J - this is the original (and virtual) interface to create
LevelChunk *ServerChunkCache::create(int x, int z)
{
	return create(x, z, false);
}

LevelChunk *ServerChunkCache::create(int x, int z, bool asyncPostProcess)	// 4J - added extra parameter
{
	int64_t key = chunkKey(x, z);

	// World border check
	int blockX = x * 16;
	int blockZ = z * 16;
	if (blockX < -Level::MAX_LEVEL_SIZE || blockX >= Level::MAX_LEVEL_SIZE ||
		blockZ < -Level::MAX_LEVEL_SIZE || blockZ >= Level::MAX_LEVEL_SIZE)
		return emptyChunk;

	auto it = m_chunkMap.find(key);
	if (it != m_chunkMap.end() && it->second != nullptr)
		return it->second;

	EnterCriticalSection(&m_csLoadCreate);

	// Double-check after lock
	it = m_chunkMap.find(key);
	if (it != m_chunkMap.end() && it->second != nullptr)
	{
		LeaveCriticalSection(&m_csLoadCreate);
		return it->second;
	}

	LevelChunk *chunk = load(x, z);
	if (chunk == NULL)
	{
		if (source == NULL)
			chunk = emptyChunk;
		else
			chunk = source->getChunk(x, z);
	}
	if (chunk != NULL)
	{
		chunk->load();
	}

	m_chunkMap[key] = chunk;

	// 4J - added - this will run a recalcHeightmap if source is a randomlevelsource, which has been split out from source::getChunk so that
	// we are doing it after the chunk has been added to the cache - otherwise a lot of the lighting fails as lights aren't added if the chunk
	// they are in fail ServerChunkCache::hasChunk.
	source->lightChunk(chunk);

	updatePostProcessFlags(x, z);

	m_loadedChunkList.push_back(chunk);

	// 4J - If post-processing is to be async, then let the server know about requests rather than processing directly here. Note that
	// these hasChunk() checks appear to be incorrect - the chunks checked by these map out as:
	//
	// 1.		2.		3.		4.
	// oxx		xxo		ooo		ooo
	// oPx		Poo		oox		xoo
	// ooo		ooo		oPx		Pxo
	//
	// where P marks the chunk that is being considered for postprocessing, and x marks chunks that needs to be loaded. It would seem that the
	// chunks which need to be loaded should stay the same relative to the chunk to be processed, but the hasChunk checks in 3 cases check again
	// the chunk which is to be processed itself rather than (what I presume to be) the correct position.
	// Don't think we should change in case it alters level creation.

	if (asyncPostProcess)
	{
		// 4J Stu - TODO This should also be calling the same code as chunk->checkPostProcess, but then we cannot guarantee we are in the server add the post-process request
		if (((chunk->terrainPopulated & LevelChunk::sTerrainPopulatedFromHere) == 0) && hasChunk(x + 1, z + 1) && hasChunk(x, z + 1) && hasChunk(x + 1, z)) MinecraftServer::getInstance()->addPostProcessRequest(this, x, z);
		if (hasChunk(x - 1, z) && ((getChunk(x - 1, z)->terrainPopulated & LevelChunk::sTerrainPopulatedFromHere) == 0) && hasChunk(x - 1, z + 1) && hasChunk(x, z + 1) && hasChunk(x - 1, z)) MinecraftServer::getInstance()->addPostProcessRequest(this, x - 1, z);
		if (hasChunk(x, z - 1) && ((getChunk(x, z - 1)->terrainPopulated & LevelChunk::sTerrainPopulatedFromHere) == 0) && hasChunk(x + 1, z - 1) && hasChunk(x, z - 1) && hasChunk(x + 1, z)) MinecraftServer::getInstance()->addPostProcessRequest(this, x, z - 1);
		if (hasChunk(x - 1, z - 1) && ((getChunk(x - 1, z - 1)->terrainPopulated & LevelChunk::sTerrainPopulatedFromHere) == 0) && hasChunk(x - 1, z - 1) && hasChunk(x, z - 1) && hasChunk(x - 1, z)) MinecraftServer::getInstance()->addPostProcessRequest(this, x - 1, z - 1);
	}
	else
	{
		chunk->checkPostProcess(this, this, x, z);
	}

	// 4J - Now try and fix up any chests that were saved pre-1.8.2. We don't want to do this to this particular chunk as we don't know if all its neighbours are loaded yet, and we
	// need the neighbours to be able to work out the facing direction for the chests. Therefore process any neighbouring chunk that loading this chunk would be the last neighbour for.
	// 5 cases illustrated below, where P is the chunk to be processed, T is this chunk, and x are other chunks that need to be checked for being present

	// 1.		2.		3.		4.		5.
	// ooooo	ooxoo	ooooo	ooooo	ooooo
	// oxooo	oxPxo	oooxo	ooooo	ooxoo
	// xPToo	ooToo	ooTPx	ooToo	oxPxo	(in 5th case P and T are same)
	// oxooo	ooooo	oooxo	oxPxo	ooxoo
	// ooooo	ooooo	ooooo	ooxoo	ooooo

	if (hasChunk(x - 1, z) && hasChunk(x - 2, z) && hasChunk(x - 1, z + 1) && hasChunk(x - 1, z - 1)) chunk->checkChests(this, x - 1, z);
	if (hasChunk(x, z + 1) && hasChunk(x, z + 2) && hasChunk(x - 1, z + 1) && hasChunk(x + 1, z + 1)) chunk->checkChests(this, x, z + 1);
	if (hasChunk(x + 1, z) && hasChunk(x + 2, z) && hasChunk(x + 1, z + 1) && hasChunk(x + 1, z - 1)) chunk->checkChests(this, x + 1, z);
	if (hasChunk(x, z - 1) && hasChunk(x, z - 2) && hasChunk(x - 1, z - 1) && hasChunk(x + 1, z - 1)) chunk->checkChests(this, x, z - 1);
	if (hasChunk(x - 1, z) && hasChunk(x + 1, z) && hasChunk(x, z - 1) && hasChunk(x, z + 1)) chunk->checkChests(this, x, z);

	LeaveCriticalSection(&m_csLoadCreate);

	return chunk;
}

// 4J Stu - Split out this function so that we get a chunk without loading entities
// This is used when sharing server chunk data on the main thread
LevelChunk *ServerChunkCache::getChunk(int x, int z)
{
	int64_t key = chunkKey(x, z);
	auto it = m_chunkMap.find(key);
	if (it != m_chunkMap.end() && it->second != nullptr)
		return it->second;

	if (level->isFindingSpawn || autoCreate)
		return create(x, z);

	// If async generation is active and chunk is not yet being generated, queue it
	if (m_genThreadPool != NULL && !isGenerating(x, z))
	{
		int priority = ChunkGenThreadPool::CalculateChunkPriority(
			x, z, m_genThreadPool->GetPendingCount(), 0);
		queueAsyncGeneration(x, z, priority);
	}

	return emptyChunk;
}

// 4J added - this special variation on getChunk also checks the unloaded chunk cache. It is called on a host machine from the client-side level when:
// (1) Trying to determine whether the client blocks and data are the same as those on the server, so we can start sharing them
// (2) Trying to resync the lighting data from the server to the client
// As such it is really important that we don't return emptyChunk in these situations, when we actually still have the block/data/lighting in the unloaded cache
LevelChunk *ServerChunkCache::getChunkLoadedOrUnloaded(int x, int z)
{
	int64_t key = chunkKey(x, z);

	auto it = m_chunkMap.find(key);
	if (it != m_chunkMap.end() && it->second != nullptr)
		return it->second;

	auto uit = m_unloadedCache.find(key);
	if (uit != m_unloadedCache.end() && uit->second != nullptr)
		return uit->second;

	if (level->isFindingSpawn || autoCreate)
		return create(x, z);

	return emptyChunk;
}

// 4J Added //
void ServerChunkCache::dontDrop(int x, int z)
{
	LevelChunk *chunk = getChunk(x, z);
	m_toDrop.erase(std::remove(m_toDrop.begin(), m_toDrop.end(), chunk), m_toDrop.end());
}

LevelChunk *ServerChunkCache::load(int x, int z)
{
	if (storage == NULL) return NULL;

	LevelChunk *levelChunk = NULL;

	int64_t key = chunkKey(x, z);
	auto uit = m_unloadedCache.find(key);
	if (uit != m_unloadedCache.end())
	{
		levelChunk = uit->second;
		m_unloadedCache.erase(uit);
	}

	if (levelChunk == NULL)
	{
		levelChunk = storage->load(level, x, z);
	}
	if (levelChunk != NULL)
	{
		levelChunk->lastSaveTime = level->getTime();
	}
	return levelChunk;
}

void ServerChunkCache::saveEntities(LevelChunk *levelChunk)
{
    if (storage == NULL) return;

    storage->saveEntities(level, levelChunk);
}

void ServerChunkCache::save(LevelChunk *levelChunk)
{
	if (storage == NULL) return;

	levelChunk->lastSaveTime = level->getTime();
	storage->save(level, levelChunk);
}

// 4J added
void ServerChunkCache::updatePostProcessFlag(short flag, int x, int z, int xo, int zo, LevelChunk *lc)
{
	if( hasChunk( x + xo, z + zo ) )
	{
		LevelChunk *lc2 = getChunk(x + xo, z + zo);
		if( lc2 != emptyChunk )		// Will only be empty chunk of this is the edge (we've already checked hasChunk so won't just be a missing chunk)
		{
			if( lc2->terrainPopulated & LevelChunk::sTerrainPopulatedFromHere )
			{
				lc->terrainPopulated |= flag;
			}
		}
		else
		{
			// The edge - always consider as post-processed
			lc->terrainPopulated |= flag;
		}
	}
}

// 4J added - normally we try and set these flags when a chunk is post-processed. However, when setting in a north or easterly direction the
// affected chunks might not themselves exist, so we need to check the flags also when creating new chunks.
void ServerChunkCache::updatePostProcessFlags(int x, int z)
{
	LevelChunk *lc = getChunk(x, z);
	if( lc != emptyChunk )
	{
		// First check if any of our neighbours are post-processed, that should affect OUR flags
		updatePostProcessFlag( LevelChunk::sTerrainPopulatedFromS,  x, z,  0, -1, lc );
		updatePostProcessFlag( LevelChunk::sTerrainPopulatedFromSW, x, z, -1, -1, lc );
		updatePostProcessFlag( LevelChunk::sTerrainPopulatedFromW,  x, z, -1,  0, lc );
		updatePostProcessFlag( LevelChunk::sTerrainPopulatedFromNW, x, z, -1,  1, lc );
		updatePostProcessFlag( LevelChunk::sTerrainPopulatedFromN,  x, z,  0,  1, lc );
		updatePostProcessFlag( LevelChunk::sTerrainPopulatedFromNE, x, z,  1,  1, lc );
		updatePostProcessFlag( LevelChunk::sTerrainPopulatedFromE,  x, z,  1,  0, lc );
		updatePostProcessFlag( LevelChunk::sTerrainPopulatedFromSE, x, z,  1, -1, lc );

		// Then, if WE are post-processed, check that our neighbour's flags are also set
		if( lc->terrainPopulated & LevelChunk::sTerrainPopulatedFromHere )
		{
			flagPostProcessComplete(LevelChunk::sTerrainPopulatedFromW,  x + 1, z + 0 );
			flagPostProcessComplete(LevelChunk::sTerrainPopulatedFromSW, x + 1, z + 1 );
			flagPostProcessComplete(LevelChunk::sTerrainPopulatedFromS,  x + 0, z + 1 );
			flagPostProcessComplete(LevelChunk::sTerrainPopulatedFromSE, x - 1, z + 1 );
			flagPostProcessComplete(LevelChunk::sTerrainPopulatedFromE,  x - 1, z + 0 );
			flagPostProcessComplete(LevelChunk::sTerrainPopulatedFromNE, x - 1, z - 1 );
			flagPostProcessComplete(LevelChunk::sTerrainPopulatedFromN,  x + 0, z - 1 );
			flagPostProcessComplete(LevelChunk::sTerrainPopulatedFromNW, x + 1, z - 1 );
		}
	}

	flagPostProcessComplete(0, x, z);
}

// 4J added - add a flag to a chunk to say that one of its neighbours has completed post-processing. If this completes the set of
// chunks which can actually set tile tiles in this chunk (sTerrainPopulatedAllAffecting), then this is a good point to compress this chunk.
// If this completes the set of all 8 neighbouring chunks that have been fully post-processed, then this is a good time to fix up some
// lighting things that need all the tiles to be in place in the region into which they might propagate.
void ServerChunkCache::flagPostProcessComplete(short flag, int x, int z)
{
	// Set any extra flags for this chunk to indicate which neighbours have now had their post-processing done
	if( !hasChunk(x, z) ) return;

	LevelChunk *lc = level->getChunk( x, z );
	if( lc == emptyChunk ) return;

	lc->terrainPopulated |= flag;

	// Are all neighbouring chunks which could actually place tiles on this chunk complete? (This is ones to W, SW, S)
	if( ( lc->terrainPopulated & LevelChunk::sTerrainPopulatedAllAffecting ) == LevelChunk::sTerrainPopulatedAllAffecting )
	{
		// Do the compression of data & lighting at this point
		PIXBeginNamedEvent(0,"Compressing lighting/blocks");

		// Check, using lower blocks as a reference, if we've already compressed - no point doing this multiple times, which
		// otherwise we will do as we aren't checking for the flags transitioning in the if statement we're in here
		if( !lc->isLowerBlockStorageCompressed() )
			lc->compressBlocks();
		if( !lc->isLowerBlockLightStorageCompressed() )
			lc->compressLighting();
		if( !lc->isLowerDataStorageCompressed() )
			lc->compressData();
		
		PIXEndNamedEvent();
	}

	// Are all neighbouring chunks And this one now post-processed?
	if( lc->terrainPopulated == LevelChunk::sTerrainPopulatedAllNeighbours )
	{
		// Special lighting patching for schematics first
		app.processSchematicsLighting(lc);

		// This would be a good time to fix up any lighting for this chunk since all the geometry that could affect it should now be in place
		PIXBeginNamedEvent(0,"Recheck gaps");
		if( lc->level->dimension->id != 1 )
		{
			lc->recheckGaps(true);
		}
		PIXEndNamedEvent();

		// Do a checkLight on any tiles which are lava.
		PIXBeginNamedEvent(0,"Light lava (this)");
		lc->lightLava();
		PIXEndNamedEvent();

		// Flag as now having this post-post-processing stage completed
		lc->terrainPopulated |= LevelChunk::sTerrainPostPostProcessed;
	}
}

void ServerChunkCache::postProcess(ChunkSource *parent, int x, int z )
{
    LevelChunk *chunk = getChunk(x, z);
    if ( (chunk->terrainPopulated & LevelChunk::sTerrainPopulatedFromHere) == 0 )
	{	
		if (source != NULL)
		{
			PIXBeginNamedEvent(0,"Main post processing");
            source->postProcess(parent, x, z);
			PIXEndNamedEvent();

            chunk->markUnsaved();
        }

		// Flag not only this chunk as being post-processed, but also all the chunks that this post-processing might affect. We can guarantee that these
		// chunks exist as that's determined before post-processing can even run
		chunk->terrainPopulated |= LevelChunk::sTerrainPopulatedFromHere;

		// If we are an edge chunk, fill in missing flags from sides that will never post-process
		int chunkLimit = Level::MAX_LEVEL_SIZE / 16;
		if (x <= -chunkLimit)					// Furthest west
		{
			chunk->terrainPopulated |= LevelChunk::sTerrainPopulatedFromW;
			chunk->terrainPopulated |= LevelChunk::sTerrainPopulatedFromSW;
			chunk->terrainPopulated |= LevelChunk::sTerrainPopulatedFromNW;
		}
		if (x >= chunkLimit - 1)				// Furthest east
		{
			chunk->terrainPopulated |= LevelChunk::sTerrainPopulatedFromE;
			chunk->terrainPopulated |= LevelChunk::sTerrainPopulatedFromSE;
			chunk->terrainPopulated |= LevelChunk::sTerrainPopulatedFromNE;
		}
		if (z <= -chunkLimit)					// Furthest south
		{
			chunk->terrainPopulated |= LevelChunk::sTerrainPopulatedFromS;
			chunk->terrainPopulated |= LevelChunk::sTerrainPopulatedFromSW;
			chunk->terrainPopulated |= LevelChunk::sTerrainPopulatedFromSE;
		}
		if (z >= chunkLimit - 1)				// Furthest north
		{
			chunk->terrainPopulated |= LevelChunk::sTerrainPopulatedFromN;
			chunk->terrainPopulated |= LevelChunk::sTerrainPopulatedFromNW;
			chunk->terrainPopulated |= LevelChunk::sTerrainPopulatedFromNE;
		}

		// Set flags for post-processing being complete for neighbouring chunks. This also performs actions if this post-processing completes
		// a full set of post-processing flags for one of these neighbours.
		flagPostProcessComplete(0, x, z );
		flagPostProcessComplete(LevelChunk::sTerrainPopulatedFromW,  x + 1, z + 0 );
		flagPostProcessComplete(LevelChunk::sTerrainPopulatedFromSW, x + 1, z + 1 );
		flagPostProcessComplete(LevelChunk::sTerrainPopulatedFromS,  x + 0, z + 1 );
		flagPostProcessComplete(LevelChunk::sTerrainPopulatedFromSE, x - 1, z + 1 );
		flagPostProcessComplete(LevelChunk::sTerrainPopulatedFromE,  x - 1, z + 0 );
		flagPostProcessComplete(LevelChunk::sTerrainPopulatedFromNE, x - 1, z - 1 );
		flagPostProcessComplete(LevelChunk::sTerrainPopulatedFromN,  x + 0, z - 1 );
		flagPostProcessComplete(LevelChunk::sTerrainPopulatedFromNW, x + 1, z - 1 );
    }
}

// 4J Added for suspend
bool ServerChunkCache::saveAllEntities()
{
	PIXBeginNamedEvent(0, "Save all entities");

	PIXBeginNamedEvent(0, "saving to NBT");
	EnterCriticalSection(&m_csLoadCreate);
	for(AUTO_VAR(it,m_loadedChunkList.begin()); it != m_loadedChunkList.end(); ++it)
	{
		storage->saveEntities(level, *it);
	}
	LeaveCriticalSection(&m_csLoadCreate);
	PIXEndNamedEvent();

	PIXBeginNamedEvent(0,"Flushing");
	storage->flush();
	PIXEndNamedEvent();

	PIXEndNamedEvent();
	return true;
}

bool ServerChunkCache::save(bool force, ProgressListener *progressListener)
{
	EnterCriticalSection(&m_csLoadCreate);
    int saves = 0;

	// 4J - added this to support progressListner
	int count = 0;
    if (progressListener != NULL)
	{
        AUTO_VAR(itEnd, m_loadedChunkList.end());
		for (AUTO_VAR(it, m_loadedChunkList.begin()); it != itEnd; it++)
		{
			LevelChunk *chunk = *it;
            if (chunk->shouldSave(force))
			{
                count++;
            }
        }
    }
    int cc = 0;

	bool maxSavesReached = false;

	if(!force)
	{
		//app.DebugPrintf("Unsaved chunks = %d\n", level->getUnsavedChunkCount() );
		// Single threaded implementation for small saves
		for (unsigned int i = 0; i < m_loadedChunkList.size(); i++)
		{
			LevelChunk *chunk = m_loadedChunkList[i];
#ifndef SPLIT_SAVES
			if (force && !chunk->dontSave) saveEntities(chunk);
#endif
			if (chunk->shouldSave(force))
			{
				save(chunk);
				chunk->setUnsaved(false);
				if (++saves == MAX_SAVES && !force)
				{
					LeaveCriticalSection(&m_csLoadCreate);
					return false;
				}

				// 4J - added this to support progressListener
				if (progressListener != NULL)
				{
					if (++cc % 10 == 0)
					{
						progressListener->progressStagePercentage(cc * 100 / count);
					}
				}
			}
		}
	}
	else
	{
		// 4J Stu - We have multiple for threads for all saving as part of the storage, so use that rather than new threads here

		// Created a roughly sorted list to match the order that the files were created in 	McRegionChunkStorage::McRegionChunkStorage.
		// This is to minimise the amount of data that needs to be moved round when creating a new level.

		vector<LevelChunk *> sortedChunkList;

		for( int i = 0; i < m_loadedChunkList.size(); i++ )
		{
			if( ( m_loadedChunkList[i]->x < 0 ) && ( m_loadedChunkList[i]->z < 0 ) ) sortedChunkList.push_back(m_loadedChunkList[i]);
		}
		for( int i = 0; i < m_loadedChunkList.size(); i++ )
		{
			if( ( m_loadedChunkList[i]->x >= 0 ) && ( m_loadedChunkList[i]->z < 0 ) ) sortedChunkList.push_back(m_loadedChunkList[i]);
		}
		for( int i = 0; i < m_loadedChunkList.size(); i++ )
		{
			if( ( m_loadedChunkList[i]->x >= 0 ) && ( m_loadedChunkList[i]->z >= 0 ) ) sortedChunkList.push_back(m_loadedChunkList[i]);
		}
		for( int i = 0; i < m_loadedChunkList.size(); i++ )
		{
			if( ( m_loadedChunkList[i]->x < 0 ) && ( m_loadedChunkList[i]->z >= 0 ) ) sortedChunkList.push_back(m_loadedChunkList[i]);
		}

		// Push all the chunks to be saved to the compression threads
		for (unsigned int i = 0; i < sortedChunkList.size();++i)
		{
			LevelChunk *chunk = sortedChunkList[i];
			if (force && !chunk->dontSave) saveEntities(chunk);
			if (chunk->shouldSave(force))
			{
				save(chunk);
				chunk->setUnsaved(false);
				if (++saves == MAX_SAVES && !force)
				{
					LeaveCriticalSection(&m_csLoadCreate);
					return false;
				}

				// 4J - added this to support progressListener
				if (progressListener != NULL)
				{
					if (++cc % 10 == 0)
					{
						progressListener->progressStagePercentage(cc * 100 / count);
					}
				}
			}
			// Wait if we are building up too big a queue of chunks to be written - on PS3 this has been seen to cause so much data to be queued that we run out of
			// out of memory when saving after exploring a full map
			storage->WaitIfTooManyQueuedChunks();
		}

		// Wait for the storage threads to be complete
		storage->WaitForAll();
	}

    if (force)
	{
        if (storage == NULL)
		{
			LeaveCriticalSection(&m_csLoadCreate);
			return true;
		}
        storage->flush();
    }

	LeaveCriticalSection(&m_csLoadCreate);
    return !maxSavesReached;

}

bool ServerChunkCache::tick()
{
	// Integrate async-generated chunks (limit to avoid frame spikes)
	integrateCompletedChunks(4);

	if (!level->noSave)
	{
		for (int i = 0; i < 100; i++)
		{
			if (!m_toDrop.empty())
			{
				LevelChunk *chunk = m_toDrop.front();
				if (!chunk->isUnloaded())
				{
					save(chunk);
					saveEntities(chunk);
					chunk->unload(true);

					AUTO_VAR(it, std::find(m_loadedChunkList.begin(), m_loadedChunkList.end(), chunk));
					if (it != m_loadedChunkList.end()) m_loadedChunkList.erase(it);

					int64_t key = chunkKey(chunk->x, chunk->z);
					m_chunkMap.erase(key);
					m_unloadedCache[key] = chunk;
				}
				m_toDrop.pop_front();
			}
		}
		if (storage != NULL) storage->tick();
	}

	return source->tick();
}

bool ServerChunkCache::shouldSave()
{
	return !level->noSave;
}

wstring ServerChunkCache::gatherStats()
{
	wstringstream ss;
	ss << L"ServerChunkCache: " << m_chunkMap.size() << L" loaded, " << m_unloadedCache.size() << L" unloaded";
	if (m_genThreadPool != NULL)
	{
		ss << L", " << m_genThreadPool->GetPendingCount() << L" pending gen";
		ss << L", " << m_genThreadPool->GetActiveCount() << L" active gen";
		ss << L", " << m_generatingSet.size() << L" in-flight";
	}
	return ss.str();
}

vector<Biome::MobSpawnerData *> *ServerChunkCache::getMobsAt(MobCategory *mobCategory, int x, int y, int z)
{
	return source->getMobsAt(mobCategory, x, y, z);
}

TilePos *ServerChunkCache::findNearestMapFeature(Level *level, const wstring &featureName, int x, int y, int z)
{
	return source->findNearestMapFeature(level, featureName, x, y, z);
}

void ServerChunkCache::initAsyncGeneration(int dimension, __int64 seed, int numThreads)
{
	if (m_genThreadPool != NULL) return;

	m_dimension = dimension;
	m_genThreadPool = new ChunkGenThreadPool();
	m_genThreadPool->SetLevel((Level*)level, seed, dimension);
	m_genThreadPool->Initialize(numThreads);
}

void ServerChunkCache::shutdownAsyncGeneration()
{
	if (m_genThreadPool != NULL)
	{
		m_genThreadPool->Shutdown();
		delete m_genThreadPool;
		m_genThreadPool = NULL;
	}
	m_generatingSet.clear();
}

void ServerChunkCache::queueAsyncGeneration(int x, int z, int priority)
{
	if (m_genThreadPool == NULL) return;

	int64_t key = chunkKey(x, z);

	// Don't queue if already loaded or generating
	if (m_chunkMap.count(key) > 0) return;
	if (m_generatingSet.count(key) > 0) return;

	m_generatingSet.insert(key);
	m_genThreadPool->QueueChunk(x, z, m_dimension, priority);
}

void ServerChunkCache::updatePlayerPosition(int chunkX, int chunkZ, int viewDistance)
{
	if (m_genThreadPool == NULL) return;

	m_genThreadPool->UpdatePlayerPosition(chunkX, chunkZ);
	m_genThreadPool->CancelChunksOutOfRange(chunkX, chunkZ, viewDistance + 2, m_dimension);
}

bool ServerChunkCache::isGenerating(int x, int z) const
{
	int64_t key = chunkKey(x, z);
	return m_generatingSet.count(key) > 0;
}

void ServerChunkCache::integrateCompletedChunks(int maxPerTick)
{
	if (m_genThreadPool == NULL) return;

	LevelChunk* completed;
	while (maxPerTick-- > 0 && (completed = m_genThreadPool->PollCompleted()) != NULL)
	{
		int64_t key = chunkKey(completed->x, completed->z);

		// Check not already loaded (e.g., sync load happened while async was in flight)
		auto it = m_chunkMap.find(key);
		if (it != m_chunkMap.end() && it->second != NULL)
		{
			delete completed;
			m_generatingSet.erase(key);
			continue;
		}

		completed->load();
		m_chunkMap[key] = completed;
		m_generatingSet.erase(key);

		// Run lighting
		source->lightChunk(completed);

		updatePostProcessFlags(completed->x, completed->z);
		m_loadedChunkList.push_back(completed);

		// Check post-processing for this and neighbors
		completed->checkPostProcess(this, this, completed->x, completed->z);
	}
}

int ServerChunkCache::runSaveThreadProc(LPVOID lpParam)
{
	SaveThreadData *params = (SaveThreadData *)lpParam;

	if(params->useSharedThreadStorage)
	{
		Compression::UseDefaultThreadStorage();
		OldChunkStorage::UseDefaultThreadStorage();
	}
	else
	{
		Compression::CreateNewThreadStorage();
		OldChunkStorage::CreateNewThreadStorage();
	}

	// Wait for the producer thread to tell us to start
	params->wakeEvent->WaitForSignal(INFINITE); //WaitForSingleObject(params->wakeEvent,INFINITE);

	//app.DebugPrintf("Save thread has started\n");

	while(params->chunkToSave != NULL)
	{
		PIXBeginNamedEvent(0,"Saving entities");
		//app.DebugPrintf("Save thread has started processing a chunk\n");
		if (params->saveEntities) params->cache->saveEntities(params->chunkToSave);
		PIXEndNamedEvent();
		PIXBeginNamedEvent(0,"Saving chunk");

		params->cache->save(params->chunkToSave);
		params->chunkToSave->setUnsaved(false);

		PIXEndNamedEvent();
		PIXBeginNamedEvent(0,"Notifying and waiting for next chunk");

		// Inform the producer thread that we are done with this chunk
		params->notificationEvent->Set(); //SetEvent(params->notificationEvent);

		//app.DebugPrintf("Save thread has alerted producer that it is complete\n");

		// Wait for the producer thread to tell us to go again
		params->wakeEvent->WaitForSignal(INFINITE); //WaitForSingleObject(params->wakeEvent,INFINITE);
		PIXEndNamedEvent();
	}

	//app.DebugPrintf("Thread is exiting as it has no chunk to process\n");

	if(!params->useSharedThreadStorage)
	{
		Compression::ReleaseThreadStorage();
		OldChunkStorage::ReleaseThreadStorage();
	}

	return 0;
}
