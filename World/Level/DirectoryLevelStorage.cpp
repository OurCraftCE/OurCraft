#include "stdafx.h"
#include "../IO/System.h"
#include "net.minecraft.world.entity.player.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.chunk.storage.h"
#include "net.minecraft.world.level.dimension.h"
#include "../IO/com.mojang.nbt.h"
#include "../IO/File.h"
#include "../IO/DataInputStream.h"
#include "../IO/FileInputStream.h"
#include "LevelData.h"
#include "DirectoryLevelStorage.h"
#include "../IO/ConsoleSaveFileIO.h"

const wstring DirectoryLevelStorage::sc_szPlayerDir(L"players/");

_MapDataMappings::_MapDataMappings()
{
	ZeroMemory(xuids,sizeof(PlayerUID)*MAXIMUM_MAP_SAVE_DATA);
	ZeroMemory(dimensions,sizeof(byte)*(MAXIMUM_MAP_SAVE_DATA/4));
}

int _MapDataMappings::getDimension(int id)
{
	int offset = (2*(id%4));
	int val = (dimensions[id>>2] & (3 << offset))>>offset;

	int returnVal=0;

	switch(val)
	{
	case 0:
		returnVal = 0; // Overworld
		break;
	case 1:
		returnVal = -1; // Nether
		break;
	case 2:
		returnVal = 1; // End
		break;
	default:
#ifndef _CONTENT_PACKAGE
		printf("Read invalid dimension from MapDataMapping\n");
		__debugbreak();
#endif
		break;
	}
	return returnVal;
}

void _MapDataMappings::setMapping(int id, PlayerUID xuid, int dimension)
{
	xuids[id] = xuid;

	int offset = (2*(id%4));

	// Reset it first
	dimensions[id>>2] &= ~( 2 << offset );
	switch(dimension)
	{
	case 0: // Overworld
		//dimensions[id>>2] &= ~( 2 << offset );
		break;
	case -1: // Nether
		dimensions[id>>2] |= ( 1 << offset );
		break;
	case 1: // End
		dimensions[id>>2] |= ( 2 << offset );
		break;
	default:
#ifndef _CONTENT_PACKAGE
		printf("Trinyg to set a MapDataMapping for an invalid dimension.\n");
		__debugbreak();
#endif
		break;
	}
}

// Old version the only used 1 bit for dimension indexing
_MapDataMappings_old::_MapDataMappings_old()
{
	ZeroMemory(xuids,sizeof(PlayerUID)*MAXIMUM_MAP_SAVE_DATA);
	ZeroMemory(dimensions,sizeof(byte)*(MAXIMUM_MAP_SAVE_DATA/8));
}

int _MapDataMappings_old::getDimension(int id)
{
	return dimensions[id>>3] & (128 >> (id%8) ) ? -1 : 0;
}

void _MapDataMappings_old::setMapping(int id, PlayerUID xuid, int dimension)
{
	xuids[id] = xuid;
	if( dimension == 0 )
	{
		dimensions[id>>3] &= ~( 128 >> (id%8) );
	}
	else
	{
		dimensions[id>>3] |= ( 128 >> (id%8) );
	}
}

void DirectoryLevelStorage::PlayerMappings::addMapping(int id, int centreX, int centreZ, int dimension, int scale)
{
	__int64 index = ( ((__int64)(centreZ & 0x1FFFFFFF)) << 34) | ( ((__int64)(centreX & 0x1FFFFFFF)) << 5) | ( (scale & 0x7) << 2) | (dimension & 0x3);
	m_mappings[index] = id;
	//app.DebugPrintf("Adding mapping: %d - (%d,%d)/%d/%d [%I64d - 0x%016llx]\n", id, centreX, centreZ, dimension, scale, index, index);
}

bool DirectoryLevelStorage::PlayerMappings::getMapping(int &id, int centreX, int centreZ, int dimension, int scale)
{
	//__int64 zMasked = centreZ & 0x1FFFFFFF;
	//__int64 xMasked = centreX & 0x1FFFFFFF;
	//__int64 zShifted = zMasked << 34;
	//__int64 xShifted = xMasked << 5;
	//app.DebugPrintf("xShifted = %d (0x%016x), zShifted = %I64d (0x%016llx)\n", xShifted, xShifted, zShifted, zShifted);
	__int64 index = ( ((__int64)(centreZ & 0x1FFFFFFF)) << 34) | ( ((__int64)(centreX & 0x1FFFFFFF)) << 5) | ( (scale & 0x7) << 2) | (dimension & 0x3);
	AUTO_VAR(it,m_mappings.find(index));
	if(it != m_mappings.end())
	{
		id = it->second;
		//app.DebugPrintf("Found mapping: %d - (%d,%d)/%d/%d [%I64d - 0x%016llx]\n", id, centreX, centreZ, dimension, scale, index, index);
		return true;
	}
	else
	{
		//app.DebugPrintf("Failed to find mapping: (%d,%d)/%d/%d [%I64d - 0x%016llx]\n", centreX, centreZ, dimension, scale, index, index);
		return false;
	}
}

void DirectoryLevelStorage::PlayerMappings::writeMappings(DataOutputStream *dos)
{
	dos->writeInt(m_mappings.size());
	for(AUTO_VAR(it, m_mappings.begin()); it != m_mappings.end(); ++it)
	{
		app.DebugPrintf("    -- %lld (0x%016llx) = %d\n", it->first, it->first, it->second);
		dos->writeLong(it->first);
		dos->writeInt(it->second);
	}
}

void DirectoryLevelStorage::PlayerMappings::readMappings(DataInputStream *dis)
{
	int count = dis->readInt();
	for(unsigned int i = 0; i < count; ++i)
	{
		__int64 index = dis->readLong();
		int id = dis->readInt();
		m_mappings[index] = id;
		app.DebugPrintf("    -- %lld (0x%016llx) = %d\n", index, index, id);
	}
}

DirectoryLevelStorage::DirectoryLevelStorage(ConsoleSaveFile *saveFile, const File dir, const wstring& levelId, bool createPlayerDir) : sessionId( System::currentTimeMillis() ),
	dir( L"" ), playerDir( sc_szPlayerDir ), dataDir( wstring(L"data/") ), levelId(levelId)
{
	m_saveFile = saveFile;
	m_bHasLoadedMapDataMappings = false;

	m_usedMappings = byteArray(MAXIMUM_MAP_SAVE_DATA/8);
}

DirectoryLevelStorage::~DirectoryLevelStorage()
{
	delete m_saveFile;

	for(AUTO_VAR(it,m_cachedSaveData.begin()); it != m_cachedSaveData.end(); ++it)
	{
		delete it->second;
	}

	delete m_usedMappings.data;
}

void DirectoryLevelStorage::initiateSession()
{
	// 4J Jev, removed try/catch.

	File dataFile = File( dir, wstring(L"session.lock") );
	FileOutputStream fos = FileOutputStream(dataFile);
	DataOutputStream dos = DataOutputStream(&fos);
	dos.writeLong(sessionId);
	dos.close();

}

File DirectoryLevelStorage::getFolder()
{
	return dir;
}

void DirectoryLevelStorage::checkSession()
{
	// 4J-PB - Not in the Xbox game

	/*
	File dataFile = File( dir, wstring(L"session.lock"));
	FileInputStream fis = FileInputStream(dataFile);
	DataInputStream dis = DataInputStream(&fis);
	dis.close();
	*/
}

ChunkStorage *DirectoryLevelStorage::createChunkStorage(Dimension *dimension)
{
	// 4J Jev, removed try/catch.

	if (dynamic_cast<HellDimension *>(dimension) != NULL)
	{
		File dir2 = File(dir, LevelStorage::NETHER_FOLDER);
		//dir2.mkdirs(); // 4J Removed
		return new OldChunkStorage(dir2, true);
	}
	if (dynamic_cast<TheEndDimension *>(dimension) != NULL)
	{
		File dir2 = File(dir, LevelStorage::ENDER_FOLDER);
		//dir2.mkdirs(); // 4J Removed
		return new OldChunkStorage(dir2, true);
	}

	return new OldChunkStorage(dir, true);
}

LevelData *DirectoryLevelStorage::prepareLevel() 
{
	// 4J Stu Added
	ConsoleSavePath mapFile = getDataFile(L"largeMapDataMappings");
	if (!m_bHasLoadedMapDataMappings && !mapFile.getName().empty() && getSaveFile()->doesFileExist( mapFile ))
	{
		DWORD NumberOfBytesRead;
		FileEntry *fileEntry = getSaveFile()->createFile(mapFile);

		{
			getSaveFile()->setFilePointer(fileEntry,0,NULL, FILE_BEGIN);

			byteArray data(fileEntry->getFileSize());
			getSaveFile()->readFile( fileEntry, data.data, fileEntry->getFileSize(), &NumberOfBytesRead);
			assert( NumberOfBytesRead == fileEntry->getFileSize() );

			ByteArrayInputStream bais(data);
			DataInputStream dis(&bais);
			int count = dis.readInt();
			app.DebugPrintf("Loading %d mappings\n", count);
			for(unsigned int i = 0; i < count; ++i)
			{
				PlayerUID playerUid = dis.readPlayerUID();
				app.DebugPrintf("  -- %d\n", playerUid);
				m_playerMappings[playerUid].readMappings(&dis);
			}
			dis.readFully(m_usedMappings);


			// Write out our changes now
			if(getSaveFile()->getSaveVersion() < END_DIMENSION_MAP_MAPPINGS_SAVE_VERSION) saveMapIdLookup();
		}

		m_bHasLoadedMapDataMappings = true;
    }

	// 4J Jev, removed try/catch

	ConsoleSavePath dataFile = ConsoleSavePath( wstring( L"level.dat" ) );

	if ( m_saveFile->doesFileExist( dataFile ) ) 
	{
		ConsoleSaveFileInputStream fis = ConsoleSaveFileInputStream(m_saveFile, dataFile);
		CompoundTag *root = NbtIo::readCompressed(&fis);
		CompoundTag *tag = root->getCompound(L"Data");
		LevelData *ret = new LevelData(tag);
		delete root;
		return ret;
	}

    return NULL;
}

void DirectoryLevelStorage::saveLevelData(LevelData *levelData, vector<shared_ptr<Player> > *players)
{
	// 4J Jev, removed try/catch

	CompoundTag *dataTag = levelData->createTag(players);

	CompoundTag *root = new CompoundTag();
	root->put(L"Data", dataTag);

	ConsoleSavePath currentFile = ConsoleSavePath( wstring( L"level.dat" ) );

	ConsoleSaveFileOutputStream fos = ConsoleSaveFileOutputStream( m_saveFile, currentFile );
	NbtIo::writeCompressed(root, &fos);

	delete root;
}

void DirectoryLevelStorage::saveLevelData(LevelData *levelData)
{
	// 4J Jev, removed try/catch

	CompoundTag *dataTag = levelData->createTag();

	CompoundTag *root = new CompoundTag();
	root->put(L"Data", dataTag);

	ConsoleSavePath currentFile = ConsoleSavePath( wstring( L"level.dat" ) );

	ConsoleSaveFileOutputStream fos = ConsoleSaveFileOutputStream( m_saveFile, currentFile );
	NbtIo::writeCompressed(root, &fos);

	delete root;
}

void DirectoryLevelStorage::save(shared_ptr<Player> player)
{
	// 4J Jev, removed try/catch.
	PlayerUID playerXuid = player->getXuid();
	if( playerXuid != INVALID_XUID && !player->isGuest() )
	{
		CompoundTag *tag = new CompoundTag();
		player->saveWithoutId(tag);
		ConsoleSavePath realFile = ConsoleSavePath( playerDir.getName() + std::to_wstring(player->getXuid()) + L".dat" );
		// If saves are disabled (e.g. because we are writing the save buffer to disk) then cache this player data
		if(StorageManager.GetSaveDisabled())
		{
			ByteArrayOutputStream *bos = new ByteArrayOutputStream();
			NbtIo::writeCompressed(tag,bos);

			AUTO_VAR(it, m_cachedSaveData.find(realFile.getName()));
			if(it != m_cachedSaveData.end() )
			{
				delete it->second;
			}
			m_cachedSaveData[realFile.getName()] = bos;
			app.DebugPrintf("Cached saving of file %ls due to saves being disabled\n", realFile.getName().c_str() );
		}
		else
		{
			ConsoleSaveFileOutputStream fos = ConsoleSaveFileOutputStream( m_saveFile, realFile );
			NbtIo::writeCompressed(tag, &fos);
		}
		delete tag;
	}
	else if( playerXuid != INVALID_XUID )
	{
		app.DebugPrintf("Not saving player as their XUID is a guest\n");
		dontSaveMapMappingForPlayer(playerXuid);
	}
}

 // 4J Changed return val to bool to check if new player or loaded player
bool DirectoryLevelStorage::load(shared_ptr<Player> player) 
{
	bool newPlayer = true;
	CompoundTag *tag = loadPlayerDataTag( player->getXuid() );
	if (tag != NULL)
	{
		newPlayer = false;
		player->load(tag);
		delete tag;
	}
	return newPlayer;
}

CompoundTag *DirectoryLevelStorage::loadPlayerDataTag(PlayerUID xuid)
{
	// 4J Jev, removed try/catch.
	ConsoleSavePath realFile = ConsoleSavePath( playerDir.getName() + std::to_wstring(xuid) + L".dat" );
	AUTO_VAR(it, m_cachedSaveData.find(realFile.getName()));
	if(it != m_cachedSaveData.end() )
	{
		ByteArrayOutputStream *bos = it->second;
		ByteArrayInputStream bis(bos->buf, 0, bos->size());
		CompoundTag *tag = NbtIo::readCompressed(&bis);
		bis.reset();
		app.DebugPrintf("Loaded player data from cached file %ls\n", realFile.getName().c_str() );
		return tag;
	}
	else if ( m_saveFile->doesFileExist( realFile ) )
	{
		ConsoleSaveFileInputStream fis = ConsoleSaveFileInputStream(m_saveFile, realFile);
		return NbtIo::readCompressed(&fis);
	}
	return NULL;
}

// 4J Added function
void DirectoryLevelStorage::clearOldPlayerFiles()
{
	if(StorageManager.GetSaveDisabled() ) return;

	vector<FileEntry *> *playerFiles = m_saveFile->getFilesWithPrefix( playerDir.getName() );

	if( playerFiles != NULL )
	{
#ifndef _FINAL_BUILD
		if(app.DebugSettingsOn() && app.GetGameSettingsDebugMask(ProfileManager.GetPrimaryPad())&(1L<<eDebugSetting_DistributableSave))
		{
			for(unsigned int i = 0; i < playerFiles->size(); ++i )
			{
				FileEntry *file = playerFiles->at(i);
				wstring xuidStr = replaceAll( replaceAll(file->data.filename,playerDir.getName(),L""),L".dat",L"");
				PlayerUID xuid = 0; try { xuid = std::stoull(xuidStr); } catch (...) {}
				deleteMapFilesForPlayer(xuid);
				m_saveFile->deleteFile( playerFiles->at(i) );
			}
		}
		else 
#endif
			if( playerFiles->size() > MAX_PLAYER_DATA_SAVES )
			{
				sort(playerFiles->begin(), playerFiles->end(), FileEntry::newestFirst );

				for(unsigned int i = MAX_PLAYER_DATA_SAVES; i < playerFiles->size(); ++i )
			{
					FileEntry *file = playerFiles->at(i);
					wstring xuidStr = replaceAll( replaceAll(file->data.filename,playerDir.getName(),L""),L".dat",L"");
				PlayerUID xuid = 0; try { xuid = std::stoull(xuidStr); } catch (...) {}
				deleteMapFilesForPlayer(xuid);
				m_saveFile->deleteFile( playerFiles->at(i) );
			}
			}

		delete playerFiles;
	}
}

PlayerIO *DirectoryLevelStorage::getPlayerIO() 
{
	return this;
}

void DirectoryLevelStorage::closeAll() 
{
}

ConsoleSavePath DirectoryLevelStorage::getDataFile(const wstring& id)
{
	return ConsoleSavePath( dataDir.getName() + id + L".dat" );
}

wstring DirectoryLevelStorage::getLevelId()
{
	return levelId;
}

void DirectoryLevelStorage::flushSaveFile(bool autosave)
{
#ifndef _CONTENT_PACKAGE
	if(app.DebugSettingsOn() && app.GetGameSettingsDebugMask(ProfileManager.GetPrimaryPad())&(1L<<eDebugSetting_DistributableSave))
	{
		// Delete gamerules files if it exists
		ConsoleSavePath gameRulesFiles(GAME_RULE_SAVENAME);
		if(m_saveFile->doesFileExist(gameRulesFiles))
		{
			FileEntry *fe = m_saveFile->createFile(gameRulesFiles);
			m_saveFile->deleteFile( fe );
		}
	}
#endif
	m_saveFile->Flush(autosave);
}

// 4J Added
void DirectoryLevelStorage::resetNetherPlayerPositions()
{
	if(app.GetResetNether())
	{
		vector<FileEntry *> *playerFiles = m_saveFile->getFilesWithPrefix( playerDir.getName() );

		if( playerFiles != NULL )
		{
			for( AUTO_VAR(it, playerFiles->begin()); it != playerFiles->end(); ++it)
			{
				FileEntry * realFile = *it;
				ConsoleSaveFileInputStream fis = ConsoleSaveFileInputStream(m_saveFile, realFile);
				CompoundTag *tag = NbtIo::readCompressed(&fis);
				if (tag != NULL)
				{
					// If the player is in the nether, set their y position above the top of the nether
					// This will force the player to be spawned in a valid position in the overworld when they are loaded
					if(tag->contains(L"Dimension") && tag->getInt(L"Dimension") == LevelData::DIMENSION_NETHER && tag->contains(L"Pos"))
					{						
						ListTag<DoubleTag> *pos = (ListTag<DoubleTag> *) tag->getList(L"Pos");
						pos->get(1)->data = DBL_MAX;

						ConsoleSaveFileOutputStream fos = ConsoleSaveFileOutputStream( m_saveFile, realFile );
						NbtIo::writeCompressed(tag, &fos);
					}
					delete tag;
				}
			}
			delete playerFiles;
		}
	}
}

int DirectoryLevelStorage::getAuxValueForMap(PlayerUID xuid, int dimension, int centreXC, int centreZC, int scale)
{
	int mapId = -1;
	bool foundMapping = false;

	AUTO_VAR(it, m_playerMappings.find(xuid) );
	if(it != m_playerMappings.end())
	{
		foundMapping = it->second.getMapping(mapId, centreXC, centreZC, dimension, scale);
	}

	if(!foundMapping)
	{
		for(unsigned int i = 0; i < m_usedMappings.length; ++i)
		{
			if(m_usedMappings[i] < 0xFF)
			{
				unsigned int offset = 0;
				for(; offset < 8; ++offset)
				{
					if( !(m_usedMappings[i] & (1<<offset)) )
					{
						break;
					}
				}
				mapId = (i*8) + offset;
				m_playerMappings[xuid].addMapping(mapId, centreXC, centreZC, dimension, scale);
				m_usedMappings[i] |= (1<<offset);
				break;
			}
		}
	}
	return mapId;
}

void DirectoryLevelStorage::saveMapIdLookup()
{
	if(StorageManager.GetSaveDisabled() ) return;

	ConsoleSavePath file = getDataFile(L"largeMapDataMappings");

	if (!file.getName().empty())
	{
		DWORD NumberOfBytesWritten;
		FileEntry *fileEntry = m_saveFile->createFile(file);
		m_saveFile->setFilePointer(fileEntry,0,NULL, FILE_BEGIN);

		ByteArrayOutputStream baos;
		DataOutputStream dos(&baos);
		dos.writeInt(m_playerMappings.size());
		app.DebugPrintf("Saving %d mappings\n", m_playerMappings.size());
		for(AUTO_VAR(it,m_playerMappings.begin()); it != m_playerMappings.end(); ++it)
		{
			app.DebugPrintf("  -- %d\n", it->first);
			dos.writePlayerUID(it->first);
			it->second.writeMappings(&dos);
		}
		dos.write(m_usedMappings);
		m_saveFile->writeFile(	fileEntry,
			baos.buf.data, // data buffer
			baos.size(), // number of bytes to write
			&NumberOfBytesWritten // number of bytes written
			);
	}
}

void DirectoryLevelStorage::dontSaveMapMappingForPlayer(PlayerUID xuid)
{
	AUTO_VAR(it, m_playerMappings.find(xuid) );
	if(it != m_playerMappings.end())
	{
		for(AUTO_VAR(itMap, it->second.m_mappings.begin()); itMap != it->second.m_mappings.end(); ++itMap)
		{
			int index = itMap->second / 8;
			int offset = itMap->second % 8;
			m_usedMappings[index] &= ~(1<<offset);
		}
		m_playerMappings.erase(it);
	}
}

void DirectoryLevelStorage::deleteMapFilesForPlayer(shared_ptr<Player> player)
{
	PlayerUID playerXuid = player->getXuid();
	if(playerXuid != INVALID_XUID) deleteMapFilesForPlayer(playerXuid);
}

void DirectoryLevelStorage::deleteMapFilesForPlayer(PlayerUID xuid)
{
	AUTO_VAR(it, m_playerMappings.find(xuid) );
	if(it != m_playerMappings.end())
	{
		for(AUTO_VAR(itMap, it->second.m_mappings.begin()); itMap != it->second.m_mappings.end(); ++itMap)
		{
			std::wstring id = wstring( L"map_" ) + _toString(itMap->second);
			ConsoleSavePath file = getDataFile(id);

			if(m_saveFile->doesFileExist(file) )
			{
				// If we can't actually delete this file, store the name so we can delete it later
				if(StorageManager.GetSaveDisabled()) m_mapFilesToDelete.push_back(itMap->second);
				else m_saveFile->deleteFile( m_saveFile->createFile(file) );
			}

			int index = itMap->second / 8;
			int offset = itMap->second % 8;
			m_usedMappings[index] &= ~(1<<offset);
		}
		m_playerMappings.erase(it);
	}
}

void DirectoryLevelStorage::saveAllCachedData()
{
	if(StorageManager.GetSaveDisabled() ) return;

	// Save any files that were saved while saving was disabled
	for(AUTO_VAR(it, m_cachedSaveData.begin()); it != m_cachedSaveData.end(); ++it)
	{
		ByteArrayOutputStream *bos = it->second;

		ConsoleSavePath realFile = ConsoleSavePath( it->first );
		ConsoleSaveFileOutputStream fos = ConsoleSaveFileOutputStream( m_saveFile, realFile );

		app.DebugPrintf("Actually writing cached file %ls\n",it->first.c_str() );
		fos.write(bos->buf, 0, bos->size() );
		delete bos;
	}
	m_cachedSaveData.clear();

	for(AUTO_VAR(it, m_mapFilesToDelete.begin()); it != m_mapFilesToDelete.end(); ++it)
	{
		std::wstring id = wstring( L"map_" ) + _toString(*it);
		ConsoleSavePath file = getDataFile(id);
		if(m_saveFile->doesFileExist(file) )
		{
			m_saveFile->deleteFile( m_saveFile->createFile(file) );
		}
	}
	m_mapFilesToDelete.clear();
}
