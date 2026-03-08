#include "stdafx.h"
#include "ChunkMemoryManager.h"

void (*ChunkMemoryManager::addForDeleteSLS)(SparseLightStorage*) = nullptr;
void (*ChunkMemoryManager::addForDeleteCTS)(CompressedTileStorage*) = nullptr;
void (*ChunkMemoryManager::addForDeleteSDS)(SparseDataStorage*) = nullptr;
void (*ChunkMemoryManager::finishedReassigning)() = nullptr;
