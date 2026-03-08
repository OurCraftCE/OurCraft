#pragma once
#include "../Level/Biome.h"

class TaigaBiome : public Biome
{
public:
	TaigaBiome(int id);

	virtual Feature *getTreeFeature(Random *random);
};