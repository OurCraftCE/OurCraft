#pragma once
using namespace std;

#include "LevelStorage.h"
#include "../Entities/PlayerIO.h"

#include "net.minecraft.world.entity.player.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.chunk.storage.h"
#include "net.minecraft.world.level.dimension.h"
#include "../IO/com.mojang.nbt.h"

#include "../IO/ConsoleSaveFile.h"

class MemoryLevelStorage : public LevelStorage, public PlayerIO 
{
public:
    MemoryLevelStorage();
    virtual LevelData *prepareLevel();
    virtual void checkSession();
    virtual ChunkStorage *createChunkStorage(Dimension *dimension);
    virtual void saveLevelData(LevelData *levelData, vector<shared_ptr<Player> > *players);
    virtual void saveLevelData(LevelData *levelData);
    virtual PlayerIO *getPlayerIO();
    virtual void closeAll();
    virtual void save(shared_ptr<Player> player);
    virtual bool load(shared_ptr<Player> player);
    virtual CompoundTag *loadPlayerDataTag(const wstring& playerName);
    virtual ConsoleSavePath getDataFile(const wstring& id);
};