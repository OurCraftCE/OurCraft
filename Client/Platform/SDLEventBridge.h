#pragma once

#include <SDL3/SDL.h>

class SDLWindow;
class SDLInput;
class KeyboardMouseInput;

// routes SDL events to g_KBMInput (KeyboardMouseInput) and SDLInput (gamepad);
// replaces the Win32 WndProc event routing
class SDLEventBridge
{
public:
	void Init(SDLWindow* window, SDLInput* gamepadInput, KeyboardMouseInput* kbmInput);

	// pump all SDL events and dispatch to the right handler; returns false on SDL_EVENT_QUIT
	bool PumpEvents();

private:
	SDLWindow* m_window = nullptr;
	SDLInput* m_gamepadInput = nullptr;
	KeyboardMouseInput* m_kbmInput = nullptr;

	// map SDL scancode to Windows VK code (for g_KBMInput compatibility)
	static int SDLScancodeToVK(SDL_Scancode sc);

	// map SDL mouse button to our button index
	static int SDLMouseButton(int sdlButton);
};

extern SDLEventBridge g_EventBridge;
