#include "stdafx.h"
#include "MultiPlayerChunkCache.h"
#include "ServerChunkCache.h"
#include "../../World/FwdHeaders/net.minecraft.world.level.chunk.h"
#include "../../World/FwdHeaders/net.minecraft.world.level.dimension.h"
#include "../../World/Math\Arrays.h"
#include "../../World/Math\StringHelpers.h"
#include "../MinecraftServer.h"
#include "../Game/ServerLevel.h"
#include "../../World/Tiles\Tile.h"
#include "../../World/Level\WaterLevelChunk.h"

MultiPlayerChunkCache::MultiPlayerChunkCache(Level *level)
{
	m_XZSize = level->dimension->getXZSize();
	m_chunkMap.reserve(4096);
	m_hasData.reserve(4096);

	emptyChunk = new EmptyLevelChunk(level, byteArray(16 * 16 * Level::maxBuildHeight), 0, 0);

	// For normal world dimension, create a chunk that can be used to create the illusion of infinite water at the edge of the world
	if( level->dimension->id == 0 )
	{
		byteArray bytes = byteArray(16 * 16 * 128);

		// Superflat.... make grass, not water...
		if(level->getLevelData()->getGenerator() == LevelType::lvl_flat)
		{
			for( int x = 0; x < 16; x++ )
				for( int y = 0; y < 128; y++ )
					for( int z = 0; z < 16; z++ )
					{
						unsigned char tileId = 0;
						if( y == 3 ) tileId = Tile::grass_Id;
						else if( y <= 2 ) tileId = Tile::dirt_Id;

						bytes[x << 11 | z << 7 | y] = tileId;
					}
		}
		else
		{
			for( int x = 0; x < 16; x++ )
				for( int y = 0; y < 128; y++ )
					for( int z = 0; z < 16; z++ )
					{
						unsigned char tileId = 0;
						if( y <= ( level->getSeaLevel() - 10 ) ) tileId = Tile::rock_Id;
						else if( y < level->getSeaLevel() ) tileId = Tile::calmWater_Id;

						bytes[x << 11 | z << 7 | y] = tileId;
					}
		}

		waterChunk = new WaterLevelChunk(level, bytes, 0, 0);

		delete[] bytes.data;

		if(level->getLevelData()->getGenerator() == LevelType::lvl_flat)
		{
			for( int x = 0; x < 16; x++ )
				for( int y = 0; y < 128; y++ )
					for( int z = 0; z < 16; z++ )
					{
						if( y >= 3 )
						{
							((WaterLevelChunk *)waterChunk)->setLevelChunkBrightness(LightLayer::Sky,x,y,z,15);
						}
					}
		}
		else
		{
			for( int x = 0; x < 16; x++ )
				for( int y = 0; y < 128; y++ )
					for( int z = 0; z < 16; z++ )
					{
						if( y >= ( level->getSeaLevel() - 1 ) )
						{
							((WaterLevelChunk *)waterChunk)->setLevelChunkBrightness(LightLayer::Sky,x,y,z,15);
						}
						else
						{
							((WaterLevelChunk *)waterChunk)->setLevelChunkBrightness(LightLayer::Sky,x,y,z,2);
						}
					}
		}
	}
	else
	{
		waterChunk = NULL;
	}

	this->level = level;

	InitializeCriticalSectionAndSpinCount(&m_csLoadCreate,4000);
}

MultiPlayerChunkCache::~MultiPlayerChunkCache()
{
	delete emptyChunk;
	delete waterChunk;

	AUTO_VAR(itEnd, loadedChunkList.end());
	for (AUTO_VAR(it, loadedChunkList.begin()); it != itEnd; it++)
		delete *it;

	m_chunkMap.clear();
	m_hasData.clear();

	DeleteCriticalSection(&m_csLoadCreate);
}


bool MultiPlayerChunkCache::hasChunk(int x, int z)
{
	// This cache always claims to have chunks, although it might actually just return empty data if it doesn't have anything
	return true;
}

// 4J  added - find out if we actually really do have a chunk in our cache
bool MultiPlayerChunkCache::reallyHasChunk(int x, int z)
{
	int64_t key = chunkKey(x, z);
	auto it = m_chunkMap.find(key);
	if( it == m_chunkMap.end() || it->second == NULL )
	{
		return false;
	}
	return m_hasData.count(key) > 0;
}

void MultiPlayerChunkCache::drop(int x, int z)
{
	// 4J Stu - We do want to drop any entities in the chunks, especially for the case when a player is dead as they will
	// not get the RemoveEntity packet if an entity is removed.
    LevelChunk *chunk = getChunk(x, z);
    if (!chunk->isEmpty())
	{
		// Added parameter here specifies that we don't want to delete tile entities, as they won't get recreated unless they've got update packets
		// The tile entities are in general only created on the client by virtue of the chunk rebuild
        chunk->unload(false);

		// 4J - We just want to clear out the entities in the chunk, but everything else should be valid
		chunk->loaded = true;
    }
}

LevelChunk *MultiPlayerChunkCache::create(int x, int z)
{
	int64_t key = chunkKey(x, z);
	auto it = m_chunkMap.find(key);
	LevelChunk *chunk = (it != m_chunkMap.end()) ? it->second : NULL;

	if( chunk == NULL )
	{
		EnterCriticalSection(&m_csLoadCreate);

		if( g_NetworkManager.IsHost() )		// force here to disable sharing of data
		{
			// 4J-JEV: We are about to use shared data, abort if the server is stopped and the data is deleted.
			if (MinecraftServer::getInstance()->serverHalted()) return NULL;

			// If we're the host, then don't create the chunk, share data from the server's copy
			LevelChunk *serverChunk = MinecraftServer::getInstance()->getLevel(level->dimension->id)->cache->getChunkLoadedOrUnloaded(x,z);
			chunk = new LevelChunk(level, x, z, serverChunk);
			// Let renderer know that this chunk has been created - it might have made render data from the EmptyChunk if it got to a chunk before the server sent it
			level->setTilesDirty( x * 16 , 0 , z * 16 , x * 16 + 15, 127, z * 16 + 15);
			m_hasData.insert(key);
		}
		else
		{
			// Passing an empty array into the LevelChunk ctor, which it now detects and sets up the chunk as compressed & empty
			byteArray bytes;

			chunk = new LevelChunk(level, bytes, x, z);

			// 4J - changed to use new methods for lighting
			chunk->setSkyLightDataAllBright();
//			Arrays::fill(chunk->skyLight->data, (byte) 255);
		}

		chunk->loaded = true;

		LeaveCriticalSection(&m_csLoadCreate);

		// Insert into the HashMap
		m_chunkMap[key] = chunk;

		// If we're sharing with the server, we'll need to calculate our heightmap now, which isn't shared. If we aren't sharing with the server,
		// then this will be calculated when the chunk data arrives.
		if( g_NetworkManager.IsHost() )
		{
			chunk->recalcHeightmapOnly();
		}

		EnterCriticalSection(&m_csLoadCreate);
		loadedChunkList.push_back(chunk);
		LeaveCriticalSection(&m_csLoadCreate);
	}
	else
	{
		chunk->load();
	}

	return chunk;
}

LevelChunk *MultiPlayerChunkCache::getChunk(int x, int z)
{
	int64_t key = chunkKey(x, z);
	auto it = m_chunkMap.find(key);

	if( it == m_chunkMap.end() || it->second == NULL )
	{
		return emptyChunk;
	}
	else
	{
		return it->second;
	}
}

bool MultiPlayerChunkCache::save(bool force, ProgressListener *progressListener)
{
	return true;
}

bool MultiPlayerChunkCache::tick()
{
	return false;
}

bool MultiPlayerChunkCache::shouldSave()
{
	return false;
}

void MultiPlayerChunkCache::postProcess(ChunkSource *parent, int x, int z)
{
}

vector<Biome::MobSpawnerData *> *MultiPlayerChunkCache::getMobsAt(MobCategory *mobCategory, int x, int y, int z)
{
	return NULL;
}

TilePos *MultiPlayerChunkCache::findNearestMapFeature(Level *level, const wstring &featureName, int x, int y, int z)
{
	return NULL;
}

wstring MultiPlayerChunkCache::gatherStats()
{
	EnterCriticalSection(&m_csLoadCreate);
	int size = (int)loadedChunkList.size();
	LeaveCriticalSection(&m_csLoadCreate);
	return L"MultiplayerChunkCache: " + _toString<int>(size);

}

void MultiPlayerChunkCache::dataReceived(int x, int z)
{
	int64_t key = chunkKey(x, z);
	m_hasData.insert(key);
}
