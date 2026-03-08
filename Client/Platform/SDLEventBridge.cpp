#include "SDLEventBridge.h"
#include "SDLWindow.h"
#include "SDLInput.h"
#include "../Input/KeyboardMouseInput.h"
#include "RmlUIManager.h"
#include "UIScreenManager.h"

#include <SDL3/SDL.h>
#define SDL_MAIN_HANDLED
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

SDLEventBridge g_EventBridge;

void SDLEventBridge::Init(SDLWindow* window, SDLInput* gamepadInput, KeyboardMouseInput* kbmInput)
{
	m_window = window;
	m_gamepadInput = gamepadInput;
	m_kbmInput = kbmInput;
}

bool SDLEventBridge::PumpEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		RmlUIManager::Get().ProcessSDLEvent(event);

		switch (event.type)
		{
		case SDL_EVENT_QUIT:
			return false;

		case SDL_EVENT_WINDOW_RESIZED:
			if (m_window)
			{
				extern int g_iScreenWidth, g_iScreenHeight;
				g_iScreenWidth = event.window.data1;
				g_iScreenHeight = event.window.data2;
				m_window->OnResize(event.window.data1, event.window.data2);
			}
			break;

		case SDL_EVENT_WINDOW_FOCUS_GAINED:
			if (m_kbmInput)
				m_kbmInput->SetWindowFocused(true);
			break;

		case SDL_EVENT_WINDOW_FOCUS_LOST:
			if (m_kbmInput)
			{
				m_kbmInput->ClearAllState();
				m_kbmInput->SetWindowFocused(false);
				if (m_kbmInput->IsMouseGrabbed())
					m_kbmInput->SetMouseGrabbed(false);
			}
			break;

		case SDL_EVENT_KEY_DOWN:
		{
			if (event.key.repeat)
				break;

			int vk = SDLScancodeToVK(event.key.scancode);
			if (vk == 0)
				break;

			if (UIScreenManager::Get().IsWaitingForKey())
			{
				UIScreenManager::Get().OnKeyPressed(vk);
				break;
			}

			if (vk == VK_F11 || (vk == VK_RETURN && (event.key.mod & SDL_KMOD_ALT)))
			{
				if (m_window)
					m_window->ToggleFullscreen();
				break;
			}

			if (m_kbmInput)
				m_kbmInput->OnKeyDown(vk);
			break;
		}

		case SDL_EVENT_KEY_UP:
		{
			int vk = SDLScancodeToVK(event.key.scancode);
			if (vk != 0 && m_kbmInput)
				m_kbmInput->OnKeyUp(vk);
			break;
		}

		case SDL_EVENT_TEXT_INPUT:
		{
			if (m_kbmInput && event.text.text)
			{
				// SDL gives UTF-8; convert first char to wchar_t for KBM layer
				const char* text = event.text.text;
				if (text[0] != 0)
				{
					wchar_t wch = 0;
					MultiByteToWideChar(CP_UTF8, 0, text, -1, &wch, 1);
					if (wch)
						m_kbmInput->OnCharInput(wch);
				}
			}
			break;
		}

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		{
			int btn = SDLMouseButton(event.button.button);
			if (btn >= 0)
			{
				// negative code convention used by keybind rebinding
				if (UIScreenManager::Get().IsWaitingForKey())
				{
					UIScreenManager::Get().OnKeyPressed(-100 + btn);
					break;
				}
				if (m_kbmInput)
					m_kbmInput->OnMouseButtonDown(btn);
			}
			break;
		}

		case SDL_EVENT_MOUSE_BUTTON_UP:
		{
			int btn = SDLMouseButton(event.button.button);
			if (btn >= 0 && m_kbmInput)
				m_kbmInput->OnMouseButtonUp(btn);
			break;
		}

		case SDL_EVENT_MOUSE_MOTION:
			if (m_kbmInput)
			{
				m_kbmInput->OnMouseMove((int)event.motion.x, (int)event.motion.y);
				m_kbmInput->OnRawMouseDelta((int)event.motion.xrel, (int)event.motion.yrel);
			}
			break;

		case SDL_EVENT_MOUSE_WHEEL:
			if (m_kbmInput)
				m_kbmInput->OnMouseWheel((int)(event.wheel.y * 120.0f)); // match WHEEL_DELTA
			break;

		case SDL_EVENT_GAMEPAD_ADDED:
		case SDL_EVENT_GAMEPAD_REMOVED:
			if (m_gamepadInput)
				m_gamepadInput->ProcessSDLEvent(&event);
			break;
		}
	}

	return true;
}

int SDLEventBridge::SDLScancodeToVK(SDL_Scancode sc)
{
	// existing KeyboardMouseInput uses Windows VK codes throughout
	switch (sc)
	{
	case SDL_SCANCODE_A: return 'A';
	case SDL_SCANCODE_B: return 'B';
	case SDL_SCANCODE_C: return 'C';
	case SDL_SCANCODE_D: return 'D';
	case SDL_SCANCODE_E: return 'E';
	case SDL_SCANCODE_F: return 'F';
	case SDL_SCANCODE_G: return 'G';
	case SDL_SCANCODE_H: return 'H';
	case SDL_SCANCODE_I: return 'I';
	case SDL_SCANCODE_J: return 'J';
	case SDL_SCANCODE_K: return 'K';
	case SDL_SCANCODE_L: return 'L';
	case SDL_SCANCODE_M: return 'M';
	case SDL_SCANCODE_N: return 'N';
	case SDL_SCANCODE_O: return 'O';
	case SDL_SCANCODE_P: return 'P';
	case SDL_SCANCODE_Q: return 'Q';
	case SDL_SCANCODE_R: return 'R';
	case SDL_SCANCODE_S: return 'S';
	case SDL_SCANCODE_T: return 'T';
	case SDL_SCANCODE_U: return 'U';
	case SDL_SCANCODE_V: return 'V';
	case SDL_SCANCODE_W: return 'W';
	case SDL_SCANCODE_X: return 'X';
	case SDL_SCANCODE_Y: return 'Y';
	case SDL_SCANCODE_Z: return 'Z';

	case SDL_SCANCODE_0: return '0';
	case SDL_SCANCODE_1: return '1';
	case SDL_SCANCODE_2: return '2';
	case SDL_SCANCODE_3: return '3';
	case SDL_SCANCODE_4: return '4';
	case SDL_SCANCODE_5: return '5';
	case SDL_SCANCODE_6: return '6';
	case SDL_SCANCODE_7: return '7';
	case SDL_SCANCODE_8: return '8';
	case SDL_SCANCODE_9: return '9';

	case SDL_SCANCODE_F1:  return VK_F1;
	case SDL_SCANCODE_F2:  return VK_F2;
	case SDL_SCANCODE_F3:  return VK_F3;
	case SDL_SCANCODE_F4:  return VK_F4;
	case SDL_SCANCODE_F5:  return VK_F5;
	case SDL_SCANCODE_F6:  return VK_F6;
	case SDL_SCANCODE_F7:  return VK_F7;
	case SDL_SCANCODE_F8:  return VK_F8;
	case SDL_SCANCODE_F9:  return VK_F9;
	case SDL_SCANCODE_F10: return VK_F10;
	case SDL_SCANCODE_F11: return VK_F11;
	case SDL_SCANCODE_F12: return VK_F12;

	case SDL_SCANCODE_LSHIFT:   return VK_LSHIFT;
	case SDL_SCANCODE_RSHIFT:   return VK_RSHIFT;
	case SDL_SCANCODE_LCTRL:    return VK_LCONTROL;
	case SDL_SCANCODE_RCTRL:    return VK_RCONTROL;
	case SDL_SCANCODE_LALT:     return VK_LMENU;
	case SDL_SCANCODE_RALT:     return VK_RMENU;

	case SDL_SCANCODE_RETURN:    return VK_RETURN;
	case SDL_SCANCODE_ESCAPE:    return VK_ESCAPE;
	case SDL_SCANCODE_BACKSPACE: return VK_BACK;
	case SDL_SCANCODE_TAB:       return VK_TAB;
	case SDL_SCANCODE_SPACE:     return VK_SPACE;
	case SDL_SCANCODE_DELETE:    return VK_DELETE;
	case SDL_SCANCODE_INSERT:    return VK_INSERT;
	case SDL_SCANCODE_HOME:      return VK_HOME;
	case SDL_SCANCODE_END:       return VK_END;
	case SDL_SCANCODE_PAGEUP:    return VK_PRIOR;
	case SDL_SCANCODE_PAGEDOWN:  return VK_NEXT;

	case SDL_SCANCODE_UP:    return VK_UP;
	case SDL_SCANCODE_DOWN:  return VK_DOWN;
	case SDL_SCANCODE_LEFT:  return VK_LEFT;
	case SDL_SCANCODE_RIGHT: return VK_RIGHT;

	case SDL_SCANCODE_MINUS:        return VK_OEM_MINUS;
	case SDL_SCANCODE_EQUALS:       return VK_OEM_PLUS;
	case SDL_SCANCODE_LEFTBRACKET:  return VK_OEM_4;
	case SDL_SCANCODE_RIGHTBRACKET: return VK_OEM_6;
	case SDL_SCANCODE_SEMICOLON:    return VK_OEM_1;
	case SDL_SCANCODE_APOSTROPHE:   return VK_OEM_7;
	case SDL_SCANCODE_COMMA:        return VK_OEM_COMMA;
	case SDL_SCANCODE_PERIOD:       return VK_OEM_PERIOD;
	case SDL_SCANCODE_SLASH:        return VK_OEM_2;
	case SDL_SCANCODE_BACKSLASH:    return VK_OEM_5;
	case SDL_SCANCODE_GRAVE:        return VK_OEM_3;

	default:
		return 0;
	}
}

int SDLEventBridge::SDLMouseButton(int sdlButton)
{
	switch (sdlButton)
	{
	case SDL_BUTTON_LEFT:   return 0; // MOUSE_LEFT
	case SDL_BUTTON_RIGHT:  return 1; // MOUSE_RIGHT
	case SDL_BUTTON_MIDDLE: return 2; // MOUSE_MIDDLE
	default: return -1;
	}
}
