#pragma once
#include <windows.h>

// Maps gamepad button prompts to KBM equivalents for UI display.
// When KBM is active, UI prompts should show keyboard/mouse labels
// instead of gamepad button icons (A/B/X/Y -> LMB/ESC/E/etc).

struct InputPrompt
{
	const wchar_t* gamepadText;   // e.g. "A", "B", "X", "Y"
	const wchar_t* kbmText;      // e.g. "LMB", "ESC", "E", "Q"
	int gamepadIcon;              // gamepad icon index
	int kbmIcon;                  // kbm icon index (-1 = text only)
};

// Standard prompt mappings for common actions
namespace InputPrompts
{
	// Action prompts - gamepad button -> KBM equivalent
	static const InputPrompt Confirm      = { L"\x0080", L"LMB",   0, -1 };  // A -> Left Mouse
	static const InputPrompt Cancel       = { L"\x0081", L"ESC",   1, -1 };  // B -> Escape
	static const InputPrompt Use          = { L"\x0082", L"RMB",   2, -1 };  // X -> Right Mouse
	static const InputPrompt Alternate    = { L"\x0083", L"MMB",   3, -1 };  // Y -> Middle Mouse
	static const InputPrompt Inventory    = { L"\x0083", L"E",     3, -1 };  // Y -> E key
	static const InputPrompt Drop         = { L"\x0082", L"Q",     2, -1 };  // X -> Q key
	static const InputPrompt Jump         = { L"\x0080", L"SPACE", 0, -1 };  // A -> Space
	static const InputPrompt Sneak        = { L"\x0085", L"LSHIFT",5, -1 };  // RS -> Left Shift
	static const InputPrompt Sprint       = { L"\x0084", L"LCTRL", 4, -1 };  // LS -> Left Ctrl
	static const InputPrompt Pause        = { L"\x0086", L"ESC",   6, -1 };  // Start -> Escape
	static const InputPrompt Crafting     = { L"\x0082", L"TAB",   2, -1 };  // X -> Tab
	static const InputPrompt PlayerList   = { L"\x0087", L"TAB",   7, -1 };  // Back -> Tab
	static const InputPrompt ScrollUp     = { L"\x0088", L"MWUP",  8, -1 };  // RB -> Mouse Wheel Up
	static const InputPrompt ScrollDown   = { L"\x0089", L"MWDN",  9, -1 };  // LB -> Mouse Wheel Down

	// Returns the appropriate prompt text based on the active input mode
	inline const wchar_t* GetText(const InputPrompt& prompt, bool isKBM)
	{
		return isKBM ? prompt.kbmText : prompt.gamepadText;
	}
};
