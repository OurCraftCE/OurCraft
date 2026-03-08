#include "stdafx.h"
#include <iostream>
#include "../IO/InputOutputStream.h"
#include "net.minecraft.world.level.h"
#include "PacketListener.h"
#include "TileUpdatePacket.h"
#include "../Level/Dimension.h"



TileUpdatePacket::TileUpdatePacket() 
{
	shouldDelay = true;
}

TileUpdatePacket::TileUpdatePacket(int x, int y, int z, Level *level)
{
	shouldDelay = true;
	this->x = x;
	this->y = y;
	this->z = z;
	block = level->getTile(x, y, z);
	data = level->getData(x, y, z);
	levelIdx = ( ( level->dimension->id == 0 ) ? 0 : ( (level->dimension->id == -1) ? 1 : 2 ) );
}

void TileUpdatePacket::read(DataInputStream *dis) //throws IOException 
{
	x = dis->readInt();
	y = dis->read();
	z = dis->readInt();

	block = (int)dis->readShort() & 0xffff;

	BYTE dataLevel = dis->readByte();
	data = dataLevel & 0xf;
	levelIdx = (dataLevel>>4) & 0xf;
}

void TileUpdatePacket::write(DataOutputStream *dos) //throws IOException
{
	dos->writeInt(x);
	dos->write(y);
	dos->writeInt(z);
	dos->writeShort(block);

	BYTE dataLevel = ((levelIdx & 0xf ) << 4) | (data & 0xf);
	dos->writeByte(dataLevel);
}

void TileUpdatePacket::handle(PacketListener *listener) 
{
	listener->handleTileUpdate(shared_from_this());
}

int TileUpdatePacket::getEstimatedSize() 
{
	return 12;
}
