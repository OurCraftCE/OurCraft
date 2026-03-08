#include "stdafx.h"
#include "NetworkPlayerXbox.h"

unsigned char NetworkPlayerXbox::GetSmallId()
{
	return m_pQNetPlayer ? m_pQNetPlayer->GetSmallId() : 0;
}

bool NetworkPlayerXbox::IsSameSystem(INetworkPlayer *player)
{
	NetworkPlayerXbox *other = dynamic_cast<NetworkPlayerXbox *>(player);
	if (!other || !m_pQNetPlayer || !other->m_pQNetPlayer) return false;
	return m_pQNetPlayer->IsSameSystem(other->m_pQNetPlayer);
}

bool NetworkPlayerXbox::IsHost()
{
	return m_pQNetPlayer ? m_pQNetPlayer->IsHost() : false;
}

bool NetworkPlayerXbox::IsLocal()
{
	return m_pQNetPlayer ? m_pQNetPlayer->IsLocal() : false;
}

int NetworkPlayerXbox::GetSessionIndex()
{
	return m_pQNetPlayer ? m_pQNetPlayer->GetSessionIndex() : 0;
}

int NetworkPlayerXbox::GetUserIndex()
{
	return m_userIndex;
}

const wchar_t *NetworkPlayerXbox::GetOnlineName()
{
	return m_pQNetPlayer ? m_pQNetPlayer->GetGamertag() : L"";
}

wstring NetworkPlayerXbox::GetDisplayName()
{
	return m_pQNetPlayer ? wstring(m_pQNetPlayer->GetGamertag()) : L"";
}

PlayerUID NetworkPlayerXbox::GetUID()
{
	return m_pQNetPlayer ? m_pQNetPlayer->GetXuid() : 0;
}
