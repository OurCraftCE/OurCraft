#pragma once

#include "NetworkPlayerInterface.h"

class IQNetPlayer;

// NetworkPlayerXbox wraps IQNetPlayer, delegating to the underlying QNet player stub
class NetworkPlayerXbox : public INetworkPlayer
{
public:
	NetworkPlayerXbox(IQNetPlayer *pQNetPlayer) : m_pQNetPlayer(pQNetPlayer) {}
	virtual ~NetworkPlayerXbox() {}

	IQNetPlayer *GetQNetPlayer() { return m_pQNetPlayer; }

	// INetworkPlayer interface — delegate to IQNetPlayer
	virtual unsigned char GetSmallId() override;
	virtual void SendData(INetworkPlayer *player, const void *pvData, int dataSize, bool lowPriority) override {}
	virtual bool IsSameSystem(INetworkPlayer *player) override;
	virtual int GetSendQueueSizeBytes(INetworkPlayer *player, bool lowPriority) override { return 0; }
	virtual int GetSendQueueSizeMessages(INetworkPlayer *player, bool lowPriority) override { return 0; }
	virtual int GetCurrentRtt() override { return 0; }
	virtual bool IsHost() override;
	virtual bool IsGuest() override { return false; }
	virtual bool IsLocal() override;
	virtual int GetSessionIndex() override;
	virtual bool IsTalking() override { return false; }
	virtual bool IsMutedByLocalUser(int userIndex) override { return false; }
	virtual bool HasVoice() override { return false; }
	virtual bool HasCamera() override { return false; }
	virtual int GetUserIndex() override;
	virtual void SetSocket(Socket *pSocket) override { m_pSocket = pSocket; }
	virtual Socket *GetSocket() override { return m_pSocket; }
	virtual const wchar_t *GetOnlineName() override;
	virtual wstring GetDisplayName() override;
	virtual PlayerUID GetUID() override;

private:
	IQNetPlayer *m_pQNetPlayer;
	Socket *m_pSocket = nullptr;
	int m_userIndex = 0;

public:
	void SetUserIndex(int idx) { m_userIndex = idx; }
};
