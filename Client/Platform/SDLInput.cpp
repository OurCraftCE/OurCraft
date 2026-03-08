#include <SDL3/SDL.h>
#include "SDLInput.h"
#include <cmath>
#include <cstring>

SDLInput g_SDLInput;

void SDLInput::Initialise(int iInputStateC, unsigned char ucMapC, unsigned char ucActionC, unsigned char ucMenuActionC)
{
	memset(m_actionMaps, 0, sizeof(m_actionMaps));
	memset(m_pads, 0, sizeof(m_pads));

	m_numMapStyles = ucMapC;
	m_numActions = ucActionC;
	m_numMenuActions = ucMenuActionC;

	for (int i = 0; i < MAX_GAMEPADS; i++)
	{
		m_pads[i].axisMap[0] = AXIS_MAP_LX;
		m_pads[i].axisMap[1] = AXIS_MAP_LY;
		m_pads[i].axisMap[2] = AXIS_MAP_RX;
		m_pads[i].axisMap[3] = AXIS_MAP_RY;
		m_pads[i].triggerMap[0] = TRIGGER_MAP_0;
		m_pads[i].triggerMap[1] = TRIGGER_MAP_1;
		m_pads[i].sensitivity = 1.0f;
	}

	// open any gamepads already connected at init time
	int count = 0;
	SDL_JoystickID* joysticks = SDL_GetGamepads(&count);
	if (joysticks)
	{
		for (int i = 0; i < count && i < MAX_GAMEPADS; i++)
		{
			SDL_Gamepad* gp = SDL_OpenGamepad(joysticks[i]);
			if (gp)
			{
				m_pads[i].gamepad = gp;
				m_pads[i].id = joysticks[i];
				m_pads[i].connected = true;
			}
		}
		SDL_free(joysticks);
	}
}

void SDLInput::Tick()
{
	for (int i = 0; i < MAX_GAMEPADS; i++)
	{
		PadState& pad = m_pads[i];
		pad.buttonsPrev = pad.buttonsRaw;

		if (!pad.connected || !pad.gamepad)
		{
			pad.buttonsRaw = 0;
			pad.axisLX = pad.axisLY = 0;
			pad.axisRX = pad.axisRY = 0;
			pad.triggerL = pad.triggerR = 0;
			continue;
		}

		SDL_Gamepad* gp = (SDL_Gamepad*)pad.gamepad;

		float rawLX = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFTX) / 32767.0f;
		float rawLY = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFTY) / 32767.0f;
		float rawRX = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_RIGHTX) / 32767.0f;
		float rawRY = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_RIGHTY) / 32767.0f;
		float rawLT = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) / 32767.0f;
		float rawRT = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) / 32767.0f;

		pad.axisLX = ApplyDeadzone(rawLX);
		pad.axisLY = ApplyDeadzone(rawLY);
		pad.axisRX = ApplyDeadzone(rawRX);
		pad.axisRY = ApplyDeadzone(rawRY);
		pad.triggerL = rawLT > 0.1f ? rawLT : 0.0f;
		pad.triggerR = rawRT > 0.1f ? rawRT : 0.0f;

		pad.buttonsRaw = ReadButtonBitmask(pad.gamepad,
			pad.axisLX, pad.axisLY, pad.axisRX, pad.axisRY,
			pad.triggerL, pad.triggerR);

		if (pad.buttonsRaw != 0 ||
			fabsf(pad.axisLX) > 0.1f || fabsf(pad.axisLY) > 0.1f ||
			fabsf(pad.axisRX) > 0.1f || fabsf(pad.axisRY) > 0.1f)
		{
			pad.idleTime = 0.0f;
		}
		else
		{
			pad.idleTime += 1.0f / 60.0f;
		}
	}
}

void SDLInput::Shutdown()
{
	for (int i = 0; i < MAX_GAMEPADS; i++)
	{
		if (m_pads[i].gamepad)
		{
			SDL_CloseGamepad((SDL_Gamepad*)m_pads[i].gamepad);
			m_pads[i].gamepad = nullptr;
			m_pads[i].connected = false;
		}
	}
}

void SDLInput::ProcessSDLEvent(const void* sdlEvent)
{
	const SDL_Event& event = *(const SDL_Event*)sdlEvent;
	switch (event.type)
	{
	case SDL_EVENT_GAMEPAD_ADDED:
	{
		int slot = FindEmptySlot();
		if (slot >= 0)
		{
			SDL_Gamepad* gp = SDL_OpenGamepad(event.gdevice.which);
			if (gp)
			{
				m_pads[slot].gamepad = gp;
				m_pads[slot].id = event.gdevice.which;
				m_pads[slot].connected = true;
			}
		}
		break;
	}
	case SDL_EVENT_GAMEPAD_REMOVED:
	{
		int slot = FindPadByID(event.gdevice.which);
		if (slot >= 0)
		{
			SDL_CloseGamepad((SDL_Gamepad*)m_pads[slot].gamepad);
			m_pads[slot].gamepad = nullptr;
			m_pads[slot].connected = false;
			m_pads[slot].buttonsRaw = 0;
			m_pads[slot].buttonsPrev = 0;
		}
		break;
	}
	}
}

void SDLInput::SetGameJoypadMaps(unsigned char ucMap, unsigned char ucAction, unsigned int uiActionVal)
{
	if (ucMap < MAX_MAP_STYLES && ucAction < MAX_ACTIONS)
		m_actionMaps[ucMap][ucAction] = uiActionVal;
}

unsigned int SDLInput::GetGameJoypadMaps(unsigned char ucMap, unsigned char ucAction)
{
	if (ucMap < MAX_MAP_STYLES && ucAction < MAX_ACTIONS)
		return m_actionMaps[ucMap][ucAction];
	return 0;
}

void SDLInput::SetJoypadMapVal(int iPad, unsigned char ucMap)
{
	if (iPad >= 0 && iPad < MAX_GAMEPADS)
		m_pads[iPad].mapStyle = ucMap;
}

unsigned char SDLInput::GetJoypadMapVal(int iPad)
{
	if (iPad >= 0 && iPad < MAX_GAMEPADS)
		return m_pads[iPad].mapStyle;
	return 0;
}

unsigned int SDLInput::GetValue(int iPad, unsigned char ucAction, bool bRepeat)
{
	if (iPad < 0 || iPad >= MAX_GAMEPADS)
		return 0;
	unsigned int mask = m_actionMaps[m_pads[iPad].mapStyle][ucAction];
	return m_pads[iPad].buttonsRaw & mask;
}

bool SDLInput::ButtonPressed(int iPad, unsigned char ucAction)
{
	if (iPad < 0 || iPad >= MAX_GAMEPADS)
		return false;
	if (ucAction == 255)
		return (m_pads[iPad].buttonsRaw & ~m_pads[iPad].buttonsPrev) != 0;
	unsigned int mask = m_actionMaps[m_pads[iPad].mapStyle][ucAction];
	unsigned int pressed = m_pads[iPad].buttonsRaw & ~m_pads[iPad].buttonsPrev;
	return (pressed & mask) != 0;
}

bool SDLInput::ButtonReleased(int iPad, unsigned char ucAction)
{
	if (iPad < 0 || iPad >= MAX_GAMEPADS)
		return false;
	unsigned int mask = m_actionMaps[m_pads[iPad].mapStyle][ucAction];
	unsigned int released = ~m_pads[iPad].buttonsRaw & m_pads[iPad].buttonsPrev;
	return (released & mask) != 0;
}

bool SDLInput::ButtonDown(int iPad, unsigned char ucAction)
{
	if (iPad < 0 || iPad >= MAX_GAMEPADS)
		return false;
	if (ucAction == 255)
		return m_pads[iPad].buttonsRaw != 0;
	unsigned int mask = m_actionMaps[m_pads[iPad].mapStyle][ucAction];
	return (m_pads[iPad].buttonsRaw & mask) != 0;
}

float SDLInput::GetJoypadStick_LX(int iPad, bool bCheckMenuDisplay)
{
	if (iPad < 0 || iPad >= MAX_GAMEPADS) return 0.0f;
	int axis = m_pads[iPad].axisMap[AXIS_MAP_LX];
	float vals[4] = { m_pads[iPad].axisLX, m_pads[iPad].axisLY, m_pads[iPad].axisRX, m_pads[iPad].axisRY };
	return vals[axis];
}

float SDLInput::GetJoypadStick_LY(int iPad, bool bCheckMenuDisplay)
{
	if (iPad < 0 || iPad >= MAX_GAMEPADS) return 0.0f;
	int axis = m_pads[iPad].axisMap[AXIS_MAP_LY];
	float vals[4] = { m_pads[iPad].axisLX, m_pads[iPad].axisLY, m_pads[iPad].axisRX, m_pads[iPad].axisRY };
	return vals[axis];
}

float SDLInput::GetJoypadStick_RX(int iPad, bool bCheckMenuDisplay)
{
	if (iPad < 0 || iPad >= MAX_GAMEPADS) return 0.0f;
	int axis = m_pads[iPad].axisMap[AXIS_MAP_RX];
	float vals[4] = { m_pads[iPad].axisLX, m_pads[iPad].axisLY, m_pads[iPad].axisRX, m_pads[iPad].axisRY };
	return vals[axis];
}

float SDLInput::GetJoypadStick_RY(int iPad, bool bCheckMenuDisplay)
{
	if (iPad < 0 || iPad >= MAX_GAMEPADS) return 0.0f;
	int axis = m_pads[iPad].axisMap[AXIS_MAP_RY];
	float vals[4] = { m_pads[iPad].axisLX, m_pads[iPad].axisLY, m_pads[iPad].axisRX, m_pads[iPad].axisRY };
	return vals[axis];
}

unsigned char SDLInput::GetJoypadLTrigger(int iPad, bool bCheckMenuDisplay)
{
	if (iPad < 0 || iPad >= MAX_GAMEPADS) return 0;
	int trig = m_pads[iPad].triggerMap[TRIGGER_MAP_0];
	float val = (trig == 0) ? m_pads[iPad].triggerL : m_pads[iPad].triggerR;
	return (unsigned char)(val * 255.0f);
}

unsigned char SDLInput::GetJoypadRTrigger(int iPad, bool bCheckMenuDisplay)
{
	if (iPad < 0 || iPad >= MAX_GAMEPADS) return 0;
	int trig = m_pads[iPad].triggerMap[TRIGGER_MAP_1];
	float val = (trig == 1) ? m_pads[iPad].triggerR : m_pads[iPad].triggerL;
	return (unsigned char)(val * 255.0f);
}

void SDLInput::SetDeadzoneAndMovementRange(unsigned int uiDeadzone, unsigned int uiMovementRangeMax)
{
	m_deadzone = (float)uiDeadzone / 32767.0f;
	m_movementMax = (float)uiMovementRangeMax / 32767.0f;
}

void SDLInput::SetJoypadStickAxisMap(int iPad, unsigned int uiFrom, unsigned int uiTo)
{
	if (iPad >= 0 && iPad < MAX_GAMEPADS && uiFrom < 4 && uiTo < 4)
		m_pads[iPad].axisMap[uiFrom] = (int)uiTo;
}

void SDLInput::SetJoypadStickTriggerMap(int iPad, unsigned int uiFrom, unsigned int uiTo)
{
	if (iPad >= 0 && iPad < MAX_GAMEPADS && uiFrom < 2 && uiTo < 2)
		m_pads[iPad].triggerMap[uiFrom] = (int)uiTo;
}

void SDLInput::SetKeyRepeatRate(float fRepeatDelaySecs, float fRepeatRateSecs)
{
	m_repeatDelay = fRepeatDelaySecs;
	m_repeatRate = fRepeatRateSecs;
}

void SDLInput::SetJoypadSensitivity(int iPad, float fSensitivity)
{
	if (iPad >= 0 && iPad < MAX_GAMEPADS)
		m_pads[iPad].sensitivity = fSensitivity;
}

void SDLInput::SetMenuDisplayed(int iPad, bool bVal)
{
	if (iPad >= 0 && iPad < MAX_GAMEPADS)
		m_pads[iPad].menuDisplayed = bVal;
}

unsigned int SDLInput::GetConnectedGamepadCount()
{
	unsigned int count = 0;
	for (int i = 0; i < MAX_GAMEPADS; i++)
		if (m_pads[i].connected)
			count++;
	return count;
}

bool SDLInput::IsPadConnected(int iPad)
{
	if (iPad >= 0 && iPad < MAX_GAMEPADS)
		return m_pads[iPad].connected;
	return false;
}

float SDLInput::GetIdleSeconds(int iPad)
{
	if (iPad >= 0 && iPad < MAX_GAMEPADS)
		return m_pads[iPad].idleTime;
	return 0.0f;
}

float SDLInput::ApplyDeadzone(float value) const
{
	if (fabsf(value) < m_deadzone)
		return 0.0f;
	float sign = value > 0 ? 1.0f : -1.0f;
	return sign * (fabsf(value) - m_deadzone) / (m_movementMax - m_deadzone);
}

unsigned int SDLInput::ReadButtonBitmask(void* gpVoid,
	float lx, float ly, float rx, float ry, float lt, float rt) const
{
	SDL_Gamepad* gp = (SDL_Gamepad*)gpVoid;
	unsigned int bits = 0;

	if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_SOUTH))  bits |= _360_JOY_BUTTON_A;
	if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_EAST))   bits |= _360_JOY_BUTTON_B;
	if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_WEST))   bits |= _360_JOY_BUTTON_X;
	if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_NORTH))  bits |= _360_JOY_BUTTON_Y;
	if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_START))  bits |= _360_JOY_BUTTON_START;
	if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_BACK))   bits |= _360_JOY_BUTTON_BACK;
	if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)) bits |= _360_JOY_BUTTON_RB;
	if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER))  bits |= _360_JOY_BUTTON_LB;
	if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_RIGHT_STICK))    bits |= _360_JOY_BUTTON_RTHUMB;
	if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_LEFT_STICK))     bits |= _360_JOY_BUTTON_LTHUMB;
	if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_UP))       bits |= _360_JOY_BUTTON_DPAD_UP;
	if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_DOWN))     bits |= _360_JOY_BUTTON_DPAD_DOWN;
	if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_LEFT))     bits |= _360_JOY_BUTTON_DPAD_LEFT;
	if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))    bits |= _360_JOY_BUTTON_DPAD_RIGHT;

	const float STICK_THRESHOLD = 0.5f;
	if (lx > STICK_THRESHOLD)  bits |= _360_JOY_BUTTON_LSTICK_RIGHT;
	if (lx < -STICK_THRESHOLD) bits |= _360_JOY_BUTTON_LSTICK_LEFT;
	if (ly > STICK_THRESHOLD)  bits |= _360_JOY_BUTTON_LSTICK_DOWN;
	if (ly < -STICK_THRESHOLD) bits |= _360_JOY_BUTTON_LSTICK_UP;
	if (rx > STICK_THRESHOLD)  bits |= _360_JOY_BUTTON_RSTICK_RIGHT;
	if (rx < -STICK_THRESHOLD) bits |= _360_JOY_BUTTON_RSTICK_LEFT;
	if (ry > STICK_THRESHOLD)  bits |= _360_JOY_BUTTON_RSTICK_DOWN;
	if (ry < -STICK_THRESHOLD) bits |= _360_JOY_BUTTON_RSTICK_UP;

	if (lt > 0.5f) bits |= _360_JOY_BUTTON_LT;
	if (rt > 0.5f) bits |= _360_JOY_BUTTON_RT;

	return bits;
}

int SDLInput::FindEmptySlot() const
{
	for (int i = 0; i < MAX_GAMEPADS; i++)
		if (!m_pads[i].connected)
			return i;
	return -1;
}

int SDLInput::FindPadByID(unsigned int id) const
{
	for (int i = 0; i < MAX_GAMEPADS; i++)
		if (m_pads[i].connected && m_pads[i].id == id)
			return i;
	return -1;
}

// console-compat stubs (no-op on PC)

bool SDLInput::IsPadLocked(int iPad) { return false; }
void SDLInput::SetDebugSequence(const char*, int(*)(void*), void*) {}

EKeyboardResult SDLInput::RequestKeyboard(const wchar_t*, const wchar_t*, unsigned long,
	unsigned int, int(*Func)(void*, const bool), void* lpParam, EKeyboardMode)
{
	if (Func) Func(lpParam, true);
	return EKeyboard_ResultAccept;
}

EKeyboardResult SDLInput::RequestKeyboard(unsigned int, const wchar_t*, unsigned int,
	unsigned long, wchar_t*, unsigned int, int(*Func)(void*, const bool), void* lpParam,
	EKeyboardMode, C4JStringTable*)
{
	if (Func) Func(lpParam, true);
	return EKeyboard_ResultAccept;
}

void SDLInput::DestroyKeyboard() {}
bool SDLInput::IsCircleCrossSwapped() { return false; }

void SDLInput::GetText(uint16_t* UTF16String)
{
	if (UTF16String) UTF16String[0] = 0;
}

bool SDLInput::VerifyStrings(wchar_t**, int, int(*Func)(void*, STRING_VERIFY_RESPONSE*), void* lpParam)
{
	if (Func)
	{
		STRING_VERIFY_RESPONSE resp = {};
		Func(lpParam, &resp);
	}
	return true;
}

void SDLInput::CancelQueuedVerifyStrings(int(*)(void*, STRING_VERIFY_RESPONSE*), void*) {}
void SDLInput::CancelAllVerifyInProgress() {}
void SDLInput::SetEnabledGtcButtons(long long) {}
void SDLInput::Resume() {}
