#include "stdafx.h"
#include "StatsCounter.h"
#include "../../World/Stats\Stat.h"
#include "../../World/Stats\Stats.h"
#include "../../World/Stats\Achievement.h"
#include "../../World/Stats\Achievements.h"
#include "LocalPlayer.h"

#include "../../World/FwdHeaders/net.minecraft.world.level.tile.h"
#include "../../World/FwdHeaders/net.minecraft.world.item.h"

#include "../../Client/Common\Leaderboards\LeaderboardManager.h"

Stat** StatsCounter::LARGE_STATS[] = {
	&Stats::walkOneM,
	&Stats::swimOneM,
	&Stats::fallOneM,
	&Stats::climbOneM,
	&Stats::minecartOneM,
	&Stats::boatOneM,
	&Stats::pigOneM,
	&Stats::timePlayed
};

unordered_map<Stat*, int> StatsCounter::statBoards;

StatsCounter::StatsCounter()
{
	requiresSave = false;
	saveCounter = 0;
	modifiedBoards = 0;
	flushCounter = 0;
}

void StatsCounter::award(Stat* stat, unsigned int difficulty, unsigned int count)
{
#ifndef _DURANGO
	if( stat->isAchievement() )
		difficulty = 0;

	StatsMap::iterator val = stats.find(stat);
	if( val == stats.end() )
	{
		StatContainer newVal;
		newVal.stats[difficulty] = count;
		stats.insert( make_pair(stat, newVal) );
	}
	else
	{
		val->second.stats[difficulty] += count;

		if (stat != GenericStats::timePlayed())
			app.DebugPrintf("");

		//If value has wrapped, cap it to UINT_MAX
		if( val->second.stats[difficulty] < (val->second.stats[difficulty]-count) )
			val->second.stats[difficulty] = UINT_MAX;

		//If value is larger than USHRT_MAX and is not designated as large, cap it to USHRT_MAX
		if( val->second.stats[difficulty] > USHRT_MAX && !isLargeStat(stat) )
			val->second.stats[difficulty] = USHRT_MAX;
	}

	requiresSave = true;

	//If this stat is on a leaderboard, mark that leaderboard as needing updated
	unordered_map<Stat*, int>::iterator leaderboardEntry = statBoards.find(stat);
	if( leaderboardEntry != statBoards.end() )
	{
		app.DebugPrintf("[StatsCounter] award(): %X\n", leaderboardEntry->second << difficulty);
		modifiedBoards |= (leaderboardEntry->second << difficulty);
		if( flushCounter == 0 )
			flushCounter = FLUSH_DELAY;
	}
#endif
}

bool StatsCounter::hasTaken(Achievement *ach)
{
	return stats.find(ach) != stats.end();
}

bool StatsCounter::canTake(Achievement *ach)
{
	if (ach->requires == NULL) return true;
	return hasTaken(ach->requires);
}

unsigned int StatsCounter::getValue(Stat *stat, unsigned int difficulty)
{
	StatsMap::iterator val = stats.find(stat);
	if( val != stats.end() )
		return val->second.stats[difficulty];
	return 0;
}

unsigned int StatsCounter::getTotalValue(Stat *stat)
{
	StatsMap::iterator val = stats.find(stat);
	if( val != stats.end() )
		return val->second.stats[0] + val->second.stats[1] + val->second.stats[2] + val->second.stats[3];
	return 0;
}

void StatsCounter::tick(int player)
{
	if( saveCounter > 0 )
		--saveCounter;

	if( requiresSave && saveCounter == 0 )
		save(player);

	// 4J-JEV, we don't want to write leaderboards in the middle of a game.
	// EDIT: Yes we do, people were not ending their games properly and not updating scores.
	if( flushCounter > 0 )
	{
		--flushCounter;
		if( flushCounter == 0 )
			flushLeaderboards();
	}
// #endif
}

void StatsCounter::clear()
{
	// clear out the stats when someone signs out
	stats.clear();
}

void StatsCounter::parse(void* data)
{
#ifndef _DURANGO
	// 4J-PB - If this is the trial game, let's just make sure all the stats are empty
	// 4J-PB - removing - someone can have the full game, and then remove it and go back to the trial
// 	if(!ProfileManager.IsFullVersion())
// 	{
// 		stats.clear();
// 		return;
// 	}
	//Check that we don't already have any stats
	assert( stats.size() == 0 );

	//Pointer to current position in stat array
	PBYTE pbData=(PBYTE)data;
	pbData+=sizeof(GAME_SETTINGS);
	unsigned short* statData = (unsigned short*)pbData;//data + (STAT_DATA_OFFSET/sizeof(unsigned short));

	//Value being read
	StatContainer newVal;

	//For each stat
	vector<Stat *>::iterator end = Stats::all->end();
	for( vector<Stat *>::iterator iter = Stats::all->begin() ; iter != end ; ++iter )
	{
		if( !(*iter)->isAchievement() )
		{
			if( !isLargeStat(*iter) )
			{
				if( statData[0] != 0 || statData[1] != 0 || statData[2] != 0 || statData[3] != 0 )
				{
					newVal.stats[0] = statData[0];
					newVal.stats[1] = statData[1];
					newVal.stats[2] = statData[2];
					newVal.stats[3] = statData[3];
					stats.insert( make_pair(*iter, newVal) );
				}
				statData += 4;
			}
			else
			{
				unsigned int* largeStatData = (unsigned int*)statData;
				if( largeStatData[0] != 0 || largeStatData[1] != 0 || largeStatData[2] != 0 || largeStatData[3] != 0 )
				{
					newVal.stats[0] = largeStatData[0];
					newVal.stats[1] = largeStatData[1];
					newVal.stats[2] = largeStatData[2];
					newVal.stats[3] = largeStatData[3];
					stats.insert( make_pair(*iter, newVal) );
				}
				largeStatData += 4;
				statData = (unsigned short*)largeStatData;
			}
		}
		else
		{
			if( statData[0] != 0 )
			{
				newVal.stats[0] = statData[0];
				newVal.stats[1] = 0;
				newVal.stats[2] = 0;
				newVal.stats[3] = 0;
				stats.insert( make_pair(*iter, newVal) );
			}
			++statData;
		}
	}

	dumpStatsToTTY();
#endif
}

void StatsCounter::save(int player, bool force)
{
#ifndef _DURANGO
	// 4J-PB - If this is the trial game, don't save any stats
	if(!ProfileManager.IsFullVersion())
	{
		return;
	}

	//Check we're going to have enough room to store all possible stats
	unsigned int uiTotalStatsSize = (Stats::all->size() * 4 * sizeof(unsigned short)) - (Achievements::achievements->size() * 3 * sizeof(unsigned short)) + (LARGE_STATS_COUNT*4*(sizeof(unsigned int)-sizeof(unsigned short)));
	assert( uiTotalStatsSize <= (CConsoleMinecraftApp::GAME_DEFINED_PROFILE_DATA_BYTES-sizeof(GAME_SETTINGS)) );

	//Retrieve the data pointer from the profile
#if defined _DURANGO
	PBYTE pbData = (PBYTE)StorageManager.GetGameDefinedProfileData(player);
#else
	PBYTE pbData = (PBYTE)ProfileManager.GetGameDefinedProfileData(player);
#endif
	pbData+=sizeof(GAME_SETTINGS);
	
	//Pointer to current position in stat array
	//unsigned short* statData = (unsigned short*)data + (STAT_DATA_OFFSET/sizeof(unsigned short));
	unsigned short* statData = (unsigned short*)pbData;

	//Reset all the data to 0 (we're going to replace it with the map data)
	memset(statData, 0, CConsoleMinecraftApp::GAME_DEFINED_PROFILE_DATA_BYTES-sizeof(GAME_SETTINGS));

	//For each stat
	StatsMap::iterator val;
	vector<Stat *>::iterator end = Stats::all->end();
	for( vector<Stat *>::iterator iter = Stats::all->begin() ; iter != end ; ++iter )
	{
		//If the stat is in the map write out it's value
		val = stats.find(*iter);
		if( !(*iter)->isAchievement() )
		{
			if( !isLargeStat(*iter) )
			{
				if( val != stats.end() )
				{
					statData[0] = val->second.stats[0];
					statData[1] = val->second.stats[1];
					statData[2] = val->second.stats[2];
					statData[3] = val->second.stats[3];
				}
				statData += 4;
			}
			else
			{
				unsigned int* largeStatData = (unsigned int*)statData;
				if( val != stats.end() )
				{
					largeStatData[0] = val->second.stats[0];
					largeStatData[1] = val->second.stats[1];
					largeStatData[2] = val->second.stats[2];
					largeStatData[3] = val->second.stats[3];
				}
				largeStatData += 4;
				statData = (unsigned short*)largeStatData;
			}
		}
		else
		{
			if( val != stats.end() )
			{
				statData[0] = val->second.stats[0];
			}
			++statData;
		}
	}

#if defined _DURANGO
	StorageManager.WriteToProfile(player, true, force);
#else
	ProfileManager.WriteToProfile(player, true, force);
#endif

	saveCounter = SAVE_DELAY;
#endif
}


void StatsCounter::flushLeaderboards()
{
#ifndef _DURANGO
	if( LeaderboardManager::Instance()->OpenSession() )
	{
		writeStats();
		LeaderboardManager::Instance()->FlushStats();
	}
	else
	{
		app.DebugPrintf("Failed to open a session in order to write to leaderboard\n");

		// 4J-JEV: If user was not signed in it would hit this.
		//assert(false);// && "Failed to open a session in order to write to leaderboard");
	}

	modifiedBoards = 0;
#endif
}

void StatsCounter::saveLeaderboards()
{
#ifndef _DURANGO
	// 4J-PB - If this is the trial game, no writing leaderboards
	if(!ProfileManager.IsFullVersion())
	{
		return;
	}

	if( LeaderboardManager::Instance()->OpenSession() )
	{
		writeStats();
		LeaderboardManager::Instance()->CloseSession();
	}
	else
	{
		app.DebugPrintf("Failed to open a session in order to write to leaderboard\n");

		// 4J-JEV: If user was not signed in it would hit this.
		//assert(false);// && "Failed to open a session in order to write to leaderboard");
	}

	modifiedBoards = 0;
#endif
}

void StatsCounter::writeStats()
{
#ifndef _DURANGO
	// 4J-PB - If this is the trial game, no writing
	if(!ProfileManager.IsFullVersion())
	{
		return;
	}
	//unsigned int locale = XGetLocale();

	int viewCount = 0;
	int iPad = ProfileManager.GetLockedProfile();

#endif // ndef _DURANGO
}

void StatsCounter::setupStatBoards()
{
#ifndef _DURANGO
	statBoards.insert( make_pair(Stats::killsZombie, LEADERBOARD_KILLS_PEACEFUL) );
	statBoards.insert( make_pair(Stats::killsSkeleton, LEADERBOARD_KILLS_PEACEFUL) );
	statBoards.insert( make_pair(Stats::killsCreeper, LEADERBOARD_KILLS_PEACEFUL) );
	statBoards.insert( make_pair(Stats::killsSpider, LEADERBOARD_KILLS_PEACEFUL) );
	statBoards.insert( make_pair(Stats::killsSpiderJockey, LEADERBOARD_KILLS_PEACEFUL) );
	statBoards.insert( make_pair(Stats::killsZombiePigman, LEADERBOARD_KILLS_PEACEFUL) );
	statBoards.insert( make_pair(Stats::killsNetherZombiePigman, LEADERBOARD_KILLS_PEACEFUL) );
	statBoards.insert( make_pair(Stats::killsSlime, LEADERBOARD_KILLS_PEACEFUL) );

	statBoards.insert( make_pair(Stats::blocksMined[Tile::dirt->id], LEADERBOARD_MININGBLOCKS_PEACEFUL) );
	statBoards.insert( make_pair(Stats::blocksMined[Tile::stoneBrick->id], LEADERBOARD_MININGBLOCKS_PEACEFUL) );
	statBoards.insert( make_pair(Stats::blocksMined[Tile::sand->id], LEADERBOARD_MININGBLOCKS_PEACEFUL) );
	statBoards.insert( make_pair(Stats::blocksMined[Tile::rock->id], LEADERBOARD_MININGBLOCKS_PEACEFUL) );
	statBoards.insert( make_pair(Stats::blocksMined[Tile::gravel->id], LEADERBOARD_MININGBLOCKS_PEACEFUL) );
	statBoards.insert( make_pair(Stats::blocksMined[Tile::clay->id], LEADERBOARD_MININGBLOCKS_PEACEFUL) );
	statBoards.insert( make_pair(Stats::blocksMined[Tile::obsidian->id], LEADERBOARD_MININGBLOCKS_PEACEFUL) );
 
	statBoards.insert( make_pair(Stats::itemsCollected[Item::egg->id], LEADERBOARD_FARMING_PEACEFUL) );
	statBoards.insert( make_pair(Stats::blocksMined[Tile::crops_Id], LEADERBOARD_FARMING_PEACEFUL) );
	statBoards.insert( make_pair(Stats::blocksMined[Tile::mushroom1_Id], LEADERBOARD_FARMING_PEACEFUL) );
	statBoards.insert( make_pair(Stats::blocksMined[Tile::reeds_Id], LEADERBOARD_FARMING_PEACEFUL) );
	statBoards.insert( make_pair(Stats::cowsMilked, LEADERBOARD_FARMING_PEACEFUL) );
	statBoards.insert( make_pair(Stats::itemsCollected[Tile::pumpkin->id], LEADERBOARD_FARMING_PEACEFUL) );

	statBoards.insert( make_pair(Stats::walkOneM, LEADERBOARD_TRAVELLING_PEACEFUL) );
	statBoards.insert( make_pair(Stats::fallOneM, LEADERBOARD_TRAVELLING_PEACEFUL) );
	statBoards.insert( make_pair(Stats::minecartOneM, LEADERBOARD_TRAVELLING_PEACEFUL) );
	statBoards.insert( make_pair(Stats::boatOneM, LEADERBOARD_TRAVELLING_PEACEFUL) );
#endif
}

bool StatsCounter::isLargeStat(Stat* stat)
{
#ifndef _DURANGO
	Stat*** end = &LARGE_STATS[LARGE_STATS_COUNT];
	for( Stat*** iter = LARGE_STATS ; iter != end ; ++iter )
		if( (*(*iter))->id == stat->id )
			return true;
#endif
	return false;
}

void StatsCounter::dumpStatsToTTY()
{
	vector<Stat*>::iterator statsEnd = Stats::all->end();
	for( vector<Stat*>::iterator statsIter = Stats::all->begin() ; statsIter!=statsEnd ; ++statsIter )
	{
		app.DebugPrintf("%ls\t\t%u\t%u\t%u\t%u\n",
			(*statsIter)->name.c_str(),
			getValue(*statsIter, 0),
			getValue(*statsIter, 1),
			getValue(*statsIter, 2),
			getValue(*statsIter, 3)
			);
	}
}

#ifdef _DEBUG

//To clear leaderboards set DEBUG_ENABLE_CLEAR_LEADERBOARDS to 1 and set DEBUG_CLEAR_LEADERBOARDS to be the bitmask of what you want to clear
//Leaderboards are updated on game exit so enter and exit a level to trigger the clear

//#define DEBUG_CLEAR_LEADERBOARDS			(LEADERBOARD_KILLS_EASY | LEADERBOARD_KILLS_NORMAL | LEADERBOARD_KILLS_HARD)
#define DEBUG_CLEAR_LEADERBOARDS			(0xFFFFFFFF)
#define DEBUG_ENABLE_CLEAR_LEADERBOARDS

void StatsCounter::WipeLeaderboards()
{

#if defined DEBUG_ENABLE_CLEAR_LEADERBOARDS

	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_KILLS_EASY )				XUserResetStatsViewAllUsers(STATS_VIEW_KILLS_EASY, NULL);
	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_KILLS_NORMAL )			XUserResetStatsViewAllUsers(STATS_VIEW_KILLS_NORMAL, NULL);
	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_KILLS_HARD )				XUserResetStatsViewAllUsers(STATS_VIEW_KILLS_HARD, NULL);
	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_MININGBLOCKS_PEACEFUL )	XUserResetStatsViewAllUsers(STATS_VIEW_MINING_BLOCKS_PEACEFUL, NULL);
	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_MININGBLOCKS_EASY )		XUserResetStatsViewAllUsers(STATS_VIEW_MINING_BLOCKS_EASY, NULL);
	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_MININGBLOCKS_NORMAL )	XUserResetStatsViewAllUsers(STATS_VIEW_MINING_BLOCKS_NORMAL, NULL);
	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_MININGBLOCKS_HARD )		XUserResetStatsViewAllUsers(STATS_VIEW_MINING_BLOCKS_HARD, NULL);
// 	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_MININGORE_PEACEFUL )		XUserResetStatsViewAllUsers(STATS_VIEW_MINING_ORE_PEACEFUL, NULL);
// 	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_MININGORE_EASY )			XUserResetStatsViewAllUsers(STATS_VIEW_MINING_ORE_EASY, NULL);
// 	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_MININGORE_NORMAL )		XUserResetStatsViewAllUsers(STATS_VIEW_MINING_ORE_NORMAL, NULL);
// 	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_MININGORE_HARD )			XUserResetStatsViewAllUsers(STATS_VIEW_MINING_ORE_HARD, NULL);
	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_FARMING_PEACEFUL )		XUserResetStatsViewAllUsers(STATS_VIEW_FARMING_PEACEFUL, NULL);
	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_FARMING_EASY )			XUserResetStatsViewAllUsers(STATS_VIEW_FARMING_EASY, NULL);
	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_FARMING_NORMAL )			XUserResetStatsViewAllUsers(STATS_VIEW_FARMING_NORMAL, NULL);
	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_FARMING_HARD )			XUserResetStatsViewAllUsers(STATS_VIEW_FARMING_HARD, NULL);
	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_TRAVELLING_PEACEFUL )	XUserResetStatsViewAllUsers(STATS_VIEW_TRAVELLING_PEACEFUL, NULL);
	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_TRAVELLING_EASY )		XUserResetStatsViewAllUsers(STATS_VIEW_TRAVELLING_EASY, NULL);
	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_TRAVELLING_NORMAL )		XUserResetStatsViewAllUsers(STATS_VIEW_TRAVELLING_NORMAL, NULL);
	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_TRAVELLING_HARD )		XUserResetStatsViewAllUsers(STATS_VIEW_TRAVELLING_HARD, NULL);
// 	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_NETHER_PEACEFUL )		XUserResetStatsViewAllUsers(STATS_VIEW_NETHER_PEACEFUL, NULL);
// 	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_NETHER_EASY )			XUserResetStatsViewAllUsers(STATS_VIEW_NETHER_EASY, NULL);
// 	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_NETHER_NORMAL )			XUserResetStatsViewAllUsers(STATS_VIEW_NETHER_NORMAL, NULL);
// 	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_NETHER_HARD )			XUserResetStatsViewAllUsers(STATS_VIEW_NETHER_HARD, NULL);
	if( DEBUG_CLEAR_LEADERBOARDS & LEADERBOARD_TRAVELLING_TOTAL )		XUserResetStatsViewAllUsers(STATS_VIEW_TRAVELLING_TOTAL, NULL);
	if( LeaderboardManager::Instance()->OpenSession() )
	{
		writeStats();
		LeaderboardManager::Instance()->CloseSession();
	}
#endif	
}
#endif
