#pragma once

class SparseLightStorage;
class CompressedTileStorage;
class SparseDataStorage;

// Deferred deletion callbacks for chunk storage objects.
// Client sets these to GameRenderer::AddForDelete (thread-safe deferred delete).
// Server uses defaults (immediate delete).
struct ChunkMemoryManager
{
	static void (*addForDeleteSLS)(SparseLightStorage*);
	static void (*addForDeleteCTS)(CompressedTileStorage*);
	static void (*addForDeleteSDS)(SparseDataStorage*);
	static void (*finishedReassigning)();

	static void AddForDelete(SparseLightStorage* p)  { if (addForDeleteSLS) addForDeleteSLS(p); else delete p; }
	static void AddForDelete(CompressedTileStorage* p) { if (addForDeleteCTS) addForDeleteCTS(p); else delete p; }
	static void AddForDelete(SparseDataStorage* p)   { if (addForDeleteSDS) addForDeleteSDS(p); else delete p; }
	static void FinishedReassigning()                 { if (finishedReassigning) finishedReassigning(); }
};
