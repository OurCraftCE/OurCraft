#pragma once

#include <string>

// A struct that we store in the QoS data when we are hosting the session. Max size 1020 bytes.
typedef struct _GameSessionData
{
	unsigned short netVersion;
	unsigned int m_uiGameHostSettings;
	unsigned int texturePackParentId;
	unsigned char subTexturePackId;

	bool isReadyToJoin;
	bool isJoinable;

	char hostIP[64];
	int hostPort;
	wchar_t hostName[XUSER_NAME_SIZE];
	unsigned char playerCount;
	unsigned char maxPlayers;

	_GameSessionData()
	{
		netVersion = 0;
		m_uiGameHostSettings = 0;
		texturePackParentId = 0;
		subTexturePackId = 0;
		isReadyToJoin = false;
		isJoinable = true;
		memset(hostIP, 0, sizeof(hostIP));
		hostPort = 0;
		memset(hostName, 0, sizeof(hostName));
		playerCount = 0;
		maxPlayers = MINECRAFT_NET_MAX_PLAYERS;
	}
} GameSessionData;

struct SessionSearchResult
{
	std::wstring m_playerNames[MINECRAFT_NET_MAX_PLAYERS];
	PlayerUID m_playerXuids[MINECRAFT_NET_MAX_PLAYERS];
	DWORD dwOpenPublicSlots;
	DWORD dwFilledPublicSlots;
	DWORD dwOpenPrivateSlots;
	DWORD dwFilledPrivateSlots;
	struct {
		SessionID sessionID;
	} info;

	SessionSearchResult()
	{
		dwOpenPublicSlots = 0;
		dwFilledPublicSlots = 0;
		dwOpenPrivateSlots = 0;
		dwFilledPrivateSlots = 0;
		for (int i = 0; i < MINECRAFT_NET_MAX_PLAYERS; i++)
			m_playerXuids[i] = 0;
	}
};

class FriendSessionInfo
{
public:
	SessionID sessionId;
	wchar_t *displayLabel;
	unsigned char displayLabelLength;
	unsigned char displayLabelViewableStartIndex;
	GameSessionData data;
	SessionSearchResult searchResult;
	bool hasPartyMember;

	FriendSessionInfo()
	{
		displayLabel = NULL;
		displayLabelLength = 0;
		displayLabelViewableStartIndex = 0;
		hasPartyMember = false;
	}

	~FriendSessionInfo()
	{
		if(displayLabel!=NULL)
			delete displayLabel;
	}
};
