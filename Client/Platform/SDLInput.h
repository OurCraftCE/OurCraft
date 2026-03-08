#pragma once

// SDL3-based replacement for C_4JInput (4J_Input.lib)
// safe to include from stdafx.h — no SDL3 headers needed

#include <cstdint>

// re-export 4J button defines so existing code doesn't need to change
#define	_360_JOY_BUTTON_A						0x00000001
#define	_360_JOY_BUTTON_B						0x00000002
#define	_360_JOY_BUTTON_X						0x00000004
#define	_360_JOY_BUTTON_Y						0x00000008
#define	_360_JOY_BUTTON_START					0x00000010
#define	_360_JOY_BUTTON_BACK					0x00000020
#define	_360_JOY_BUTTON_RB						0x00000040
#define	_360_JOY_BUTTON_LB						0x00000080
#define	_360_JOY_BUTTON_RTHUMB					0x00000100
#define	_360_JOY_BUTTON_LTHUMB					0x00000200
#define	_360_JOY_BUTTON_DPAD_UP					0x00000400
#define	_360_JOY_BUTTON_DPAD_DOWN				0x00000800
#define	_360_JOY_BUTTON_DPAD_LEFT				0x00001000
#define	_360_JOY_BUTTON_DPAD_RIGHT				0x00002000
#define	_360_JOY_BUTTON_LSTICK_RIGHT			0x00004000
#define	_360_JOY_BUTTON_LSTICK_LEFT				0x00008000
#define	_360_JOY_BUTTON_RSTICK_DOWN				0x00010000
#define	_360_JOY_BUTTON_RSTICK_UP				0x00020000
#define	_360_JOY_BUTTON_RSTICK_RIGHT			0x00040000
#define	_360_JOY_BUTTON_RSTICK_LEFT				0x00080000
#define	_360_JOY_BUTTON_LSTICK_DOWN				0x00100000
#define	_360_JOY_BUTTON_LSTICK_UP				0x00200000
#define	_360_JOY_BUTTON_RT						0x00400000
#define	_360_JOY_BUTTON_LT						0x00800000

#define _360_GTC_PLAY							0x01000000
#define _360_GTC_PAUSE							0x02000000
#define _360_GTC_MENU							0x04000000
#define _360_GTC_VIEW							0x08000000
#define _360_GTC_BACK							0x10000000

#define	MAP_STYLE_0		0
#define	MAP_STYLE_1		1
#define	MAP_STYLE_2		2

#define AXIS_MAP_LX		0
#define AXIS_MAP_LY		1
#define AXIS_MAP_RX		2
#define AXIS_MAP_RY		3

#define TRIGGER_MAP_0	0
#define TRIGGER_MAP_1	1

enum EKeyboardResult
{
	EKeyboard_Pending,
	EKeyboard_Cancelled,
	EKeyboard_ResultAccept,
	EKeyboard_ResultDecline,
};

typedef struct _STRING_VERIFY_RESPONSE
{
	uint16_t wNumStrings;
	long *pStringResult;
}
STRING_VERIFY_RESPONSE;

class C4JStringTable;

class SDLInput
{
public:
	static const int MAX_GAMEPADS = 8;
	static const int MAX_ACTIONS = 64;
	static const int MAX_MAP_STYLES = 4;

	enum EKeyboardMode
	{
		EKeyboardMode_Default,
		EKeyboardMode_Numeric,
		EKeyboardMode_Password,
		EKeyboardMode_Alphabet,
		EKeyboardMode_Full,
		EKeyboardMode_Alphabet_Extended,
		EKeyboardMode_IP_Address,
		EKeyboardMode_Phone,
		EKeyboardMode_URL,
		EKeyboardMode_Email
	};

	void Initialise(int iInputStateC, unsigned char ucMapC, unsigned char ucActionC, unsigned char ucMenuActionC);
	void Tick();
	void Shutdown();

	void SetDeadzoneAndMovementRange(unsigned int uiDeadzone, unsigned int uiMovementRangeMax);

	// action mapping (same interface as C_4JInput)
	void SetGameJoypadMaps(unsigned char ucMap, unsigned char ucAction, unsigned int uiActionVal);
	unsigned int GetGameJoypadMaps(unsigned char ucMap, unsigned char ucAction);
	void SetJoypadMapVal(int iPad, unsigned char ucMap);
	unsigned char GetJoypadMapVal(int iPad);

	// button state queries
	unsigned int GetValue(int iPad, unsigned char ucAction, bool bRepeat = false);
	bool ButtonPressed(int iPad, unsigned char ucAction = 255);
	bool ButtonReleased(int iPad, unsigned char ucAction);
	bool ButtonDown(int iPad, unsigned char ucAction = 255);

	// analog sticks (with optional remap for southpaw)
	float GetJoypadStick_LX(int iPad, bool bCheckMenuDisplay = true);
	float GetJoypadStick_LY(int iPad, bool bCheckMenuDisplay = true);
	float GetJoypadStick_RX(int iPad, bool bCheckMenuDisplay = true);
	float GetJoypadStick_RY(int iPad, bool bCheckMenuDisplay = true);
	unsigned char GetJoypadLTrigger(int iPad, bool bCheckMenuDisplay = true);
	unsigned char GetJoypadRTrigger(int iPad, bool bCheckMenuDisplay = true);

	void SetJoypadStickAxisMap(int iPad, unsigned int uiFrom, unsigned int uiTo);
	void SetJoypadStickTriggerMap(int iPad, unsigned int uiFrom, unsigned int uiTo);

	void SetKeyRepeatRate(float fRepeatDelaySecs, float fRepeatRateSecs);

	unsigned int GetConnectedGamepadCount();
	bool IsPadConnected(int iPad);
	bool IsPadLocked(int iPad);

	void SetJoypadSensitivity(int iPad, float fSensitivity);
	void SetMenuDisplayed(int iPad, bool bVal);
	float GetIdleSeconds(int iPad);

	// console-compat stubs (no-op on PC)
	void SetDebugSequence(const char* chSequenceA, int(*Func)(void*), void* lpParam);

	EKeyboardResult RequestKeyboard(const wchar_t* Title, const wchar_t* Text, unsigned long dwPad,
		unsigned int uiMaxChars, int(*Func)(void*, const bool), void* lpParam, EKeyboardMode eMode);
	EKeyboardResult RequestKeyboard(unsigned int uiTitle, const wchar_t* pwchDefault, unsigned int uiDesc,
		unsigned long dwPad, wchar_t* pwchResult, unsigned int uiResultSize,
		int(*Func)(void*, const bool), void* lpParam, EKeyboardMode eMode, C4JStringTable* pStringTable = nullptr);
	void DestroyKeyboard();
	bool IsCircleCrossSwapped();
	void GetText(uint16_t* UTF16String);

	bool VerifyStrings(wchar_t** pwStringA, int iStringC, int(*Func)(void*, STRING_VERIFY_RESPONSE*), void* lpParam);
	void CancelQueuedVerifyStrings(int(*Func)(void*, STRING_VERIFY_RESPONSE*), void* lpParam);
	void CancelAllVerifyInProgress();

	void SetEnabledGtcButtons(long long llEnabledButtons);
	void Resume();

	// SDL event processing — called from SDLEventBridge only, takes opaque event pointer
	void ProcessSDLEvent(const void* sdlEvent);

private:
	struct PadState
	{
		void* gamepad = nullptr;     // SDL_Gamepad* (opaque to avoid SDL include)
		unsigned int id = 0;         // SDL_JoystickID
		bool connected = false;

		float axisLX = 0, axisLY = 0;
		float axisRX = 0, axisRY = 0;
		float triggerL = 0, triggerR = 0;
		unsigned int buttonsRaw = 0;
		unsigned int buttonsPrev = 0;

		int axisMap[4] = { 0, 1, 2, 3 };
		int triggerMap[2] = { 0, 1 };

		unsigned char mapStyle = 0;
		float sensitivity = 1.0f;
		bool menuDisplayed = false;

		float idleTime = 0.0f;
	};

	PadState m_pads[MAX_GAMEPADS];

	// maps[style][action] = button bitmask
	unsigned int m_actionMaps[MAX_MAP_STYLES][MAX_ACTIONS];

	float m_deadzone = 0.2f;
	float m_movementMax = 1.0f;
	float m_repeatDelay = 0.3f;
	float m_repeatRate = 0.2f;

	int m_numMapStyles = 3;
	int m_numActions = 0;
	int m_numMenuActions = 0;

	float ApplyDeadzone(float value) const;
	unsigned int ReadButtonBitmask(void* gp, float lx, float ly, float rx, float ry, float lt, float rt) const;
	int FindEmptySlot() const;
	int FindPadByID(unsigned int id) const;
};

extern SDLInput g_SDLInput;

// redirect InputManager to g_SDLInput
#define InputManager g_SDLInput

// redirect C_4JInput:: enum references
typedef SDLInput C_4JInput;
