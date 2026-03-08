#include "RmlSystemInterface.h"

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <cstdio>

static Uint64 s_startTicks = 0;

double RmlSDLSystemInterface::GetElapsedTime()
{
	if (s_startTicks == 0)
		s_startTicks = SDL_GetTicksNS();

	Uint64 now = SDL_GetTicksNS();
	return (double)(now - s_startTicks) / 1000000000.0;
}

bool RmlSDLSystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& message)
{
	const char* prefix = "";
	switch (type)
	{
	case Rml::Log::LT_ERROR:   prefix = "[RmlUi ERROR] ";   break;
	case Rml::Log::LT_WARNING: prefix = "[RmlUi WARN]  ";   break;
	case Rml::Log::LT_INFO:    prefix = "[RmlUi INFO]  ";   break;
	case Rml::Log::LT_DEBUG:   prefix = "[RmlUi DEBUG] ";   break;
	default:                    prefix = "[RmlUi]       ";   break;
	}

	SDL_Log("%s%s", prefix, message.c_str());

	// Return true to continue execution (false would trigger a breakpoint)
	return true;
}

void RmlSDLSystemInterface::SetClipboardText(const Rml::String& text)
{
	SDL_SetClipboardText(text.c_str());
}

void RmlSDLSystemInterface::GetClipboardText(Rml::String& text)
{
	char* clip = SDL_GetClipboardText();
	if (clip)
	{
		text = clip;
		SDL_free(clip);
	}
	else
	{
		text.clear();
	}
}
