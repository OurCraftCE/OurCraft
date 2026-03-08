#pragma once

#include "../Level/Biome.h"

class PlainsBiome : public Biome
{
	friend class Biome;
protected:
	PlainsBiome(int id);
};