#pragma once

#include "CommandDispatcher.h"
#include "AdminLogCommand.h"

class ServerCommandDispatcher : public CommandDispatcher, public AdminLogCommand
{
public:
	ServerCommandDispatcher();
	void logAdminCommand(shared_ptr<CommandSender> source, int type, ChatPacket::EChatPacketMessage messageType, const wstring& message = L"", int customData = -1, const wstring& additionalMessage = L"");
};