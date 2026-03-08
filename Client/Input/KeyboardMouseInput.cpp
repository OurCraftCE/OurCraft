#include "stdafx.h"
#include "KeyboardMouseInput.h"
#include <cmath>

KeyboardMouseInput g_KBMInput;

const wchar_t* GetVKDisplayName(int keyCode)
{
	// Mouse button codes (negative, matching KeyMapping convention)
	if (keyCode == -100) return L"LMB";
	if (keyCode == -99)  return L"RMB";
	if (keyCode == -98)  return L"MMB";

	switch (keyCode)
	{
	case VK_SPACE:    return L"SPACE";
	case VK_LSHIFT:   return L"LSHIFT";
	case VK_RSHIFT:   return L"RSHIFT";
	case VK_LCONTROL: return L"LCTRL";
	case VK_RCONTROL: return L"RCTRL";
	case VK_LMENU:    return L"LALT";
	case VK_RMENU:    return L"RALT";
	case VK_TAB:      return L"TAB";
	case VK_ESCAPE:   return L"ESC";
	case VK_RETURN:   return L"ENTER";
	case VK_BACK:     return L"BACKSPACE";
	case VK_DELETE:   return L"DELETE";
	case VK_INSERT:   return L"INSERT";
	case VK_HOME:     return L"HOME";
	case VK_END:      return L"END";
	case VK_PRIOR:    return L"PGUP";
	case VK_NEXT:     return L"PGDN";
	case VK_UP:       return L"UP";
	case VK_DOWN:     return L"DOWN";
	case VK_LEFT:     return L"LEFT";
	case VK_RIGHT:    return L"RIGHT";
	case VK_F1:       return L"F1";
	case VK_F2:       return L"F2";
	case VK_F3:       return L"F3";
	case VK_F4:       return L"F4";
	case VK_F5:       return L"F5";
	case VK_F6:       return L"F6";
	case VK_F7:       return L"F7";
	case VK_F8:       return L"F8";
	case VK_F9:       return L"F9";
	case VK_F10:      return L"F10";
	case VK_F11:      return L"F11";
	case VK_F12:      return L"F12";
	default: break;
	}

	// A-Z keys
	if (keyCode >= 'A' && keyCode <= 'Z')
	{
		static wchar_t buf[2] = { 0, 0 };
		buf[0] = (wchar_t)keyCode;
		return buf;
	}
	// 0-9 keys
	if (keyCode >= '0' && keyCode <= '9')
	{
		static wchar_t buf[2] = { 0, 0 };
		buf[0] = (wchar_t)keyCode;
		return buf;
	}

	return L"???";
}

extern HWND g_hWnd;

// Forward declaration
static void ClipCursorToWindow(HWND hWnd);

void KeyboardMouseInput::Init()
{
	memset(m_keyDown, 0, sizeof(m_keyDown));
	memset(m_keyDownPrev, 0, sizeof(m_keyDownPrev));
	memset(m_keyPressedAccum, 0, sizeof(m_keyPressedAccum));
	memset(m_keyReleasedAccum, 0, sizeof(m_keyReleasedAccum));
	memset(m_keyPressed, 0, sizeof(m_keyPressed));
	memset(m_keyReleased, 0, sizeof(m_keyReleased));
	memset(m_mouseButtonDown, 0, sizeof(m_mouseButtonDown));
	memset(m_mouseButtonDownPrev, 0, sizeof(m_mouseButtonDownPrev));
	memset(m_mouseBtnPressedAccum, 0, sizeof(m_mouseBtnPressedAccum));
	memset(m_mouseBtnReleasedAccum, 0, sizeof(m_mouseBtnReleasedAccum));
	memset(m_mouseBtnPressed, 0, sizeof(m_mouseBtnPressed));
	memset(m_mouseBtnReleased, 0, sizeof(m_mouseBtnReleased));
	m_mouseX = 0;
	m_mouseY = 0;
	m_mouseDeltaX = 0;
	m_mouseDeltaY = 0;
	m_mouseDeltaAccumX = 0;
	m_mouseDeltaAccumY = 0;
	m_mouseWheel = 0;
	m_mouseWheelAccum = 0;
	m_mouseGrabbed = false;
	m_cursorHiddenForUI = false;
	m_windowFocused = true;
	m_hasInput = false;
	m_kbmActive = false;
	m_screenCursorHidden = false;

	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
	rid.usUsage = 0x02;     // HID_USAGE_GENERIC_MOUSE
	rid.dwFlags = 0;
	rid.hwndTarget = g_hWnd;
	RegisterRawInputDevices(&rid, 1, sizeof(rid));

}

void KeyboardMouseInput::ClearAllState()
{
	memset(m_keyDown, 0, sizeof(m_keyDown));
	memset(m_keyDownPrev, 0, sizeof(m_keyDownPrev));
	memset(m_keyPressedAccum, 0, sizeof(m_keyPressedAccum));
	memset(m_keyReleasedAccum, 0, sizeof(m_keyReleasedAccum));
	memset(m_keyPressed, 0, sizeof(m_keyPressed));
	memset(m_keyReleased, 0, sizeof(m_keyReleased));
	memset(m_mouseButtonDown, 0, sizeof(m_mouseButtonDown));
	memset(m_mouseButtonDownPrev, 0, sizeof(m_mouseButtonDownPrev));
	memset(m_mouseBtnPressedAccum, 0, sizeof(m_mouseBtnPressedAccum));
	memset(m_mouseBtnReleasedAccum, 0, sizeof(m_mouseBtnReleasedAccum));
	memset(m_mouseBtnPressed, 0, sizeof(m_mouseBtnPressed));
	memset(m_mouseBtnReleased, 0, sizeof(m_mouseBtnReleased));
	m_mouseDeltaX = 0;
	m_mouseDeltaY = 0;
	m_mouseDeltaAccumX = 0;
	m_mouseDeltaAccumY = 0;
	m_mouseWheel = 0;
	m_mouseWheelAccum = 0;
}

void KeyboardMouseInput::Tick()
{
	memcpy(m_keyDownPrev, m_keyDown, sizeof(m_keyDown));
	memcpy(m_mouseButtonDownPrev, m_mouseButtonDown, sizeof(m_mouseButtonDown));

	memcpy(m_keyPressed, m_keyPressedAccum, sizeof(m_keyPressedAccum));
	memcpy(m_keyReleased, m_keyReleasedAccum, sizeof(m_keyReleasedAccum));
	memset(m_keyPressedAccum, 0, sizeof(m_keyPressedAccum));
	memset(m_keyReleasedAccum, 0, sizeof(m_keyReleasedAccum));

	memcpy(m_mouseBtnPressed, m_mouseBtnPressedAccum, sizeof(m_mouseBtnPressedAccum));
	memcpy(m_mouseBtnReleased, m_mouseBtnReleasedAccum, sizeof(m_mouseBtnReleasedAccum));
	memset(m_mouseBtnPressedAccum, 0, sizeof(m_mouseBtnPressedAccum));
	memset(m_mouseBtnReleasedAccum, 0, sizeof(m_mouseBtnReleasedAccum));

	m_mouseDeltaX = m_mouseDeltaAccumX;
	m_mouseDeltaY = m_mouseDeltaAccumY;
	m_mouseDeltaAccumX = 0;
	m_mouseDeltaAccumY = 0;

	m_mouseWheel = m_mouseWheelAccum;
	m_mouseWheelAccum = 0;

	m_hasInput = (m_mouseDeltaX != 0 || m_mouseDeltaY != 0 || m_mouseWheel != 0);
	if (!m_hasInput)
	{
		for (int i = 0; i < MAX_KEYS; i++)
		{
			if (m_keyDown[i]) { m_hasInput = true; break; }
		}
	}
	if (!m_hasInput)
	{
		for (int i = 0; i < MAX_MOUSE_BUTTONS; i++)
		{
			if (m_mouseButtonDown[i]) { m_hasInput = true; break; }
		}
	}

	if (m_mouseGrabbed && g_hWnd)
	{
		RECT rc;
		GetClientRect(g_hWnd, &rc);
		POINT center;
		center.x = (rc.right - rc.left) / 2;
		center.y = (rc.bottom - rc.top) / 2;
		ClientToScreen(g_hWnd, &center);
		SetCursorPos(center.x, center.y);
	}
}

void KeyboardMouseInput::OnKeyDown(int vkCode)
{
	if (vkCode >= 0 && vkCode < MAX_KEYS)
	{
		if (!m_keyDown[vkCode])
			m_keyPressedAccum[vkCode] = true;
		m_keyDown[vkCode] = true;
	}
}

void KeyboardMouseInput::OnKeyUp(int vkCode)
{
	if (vkCode >= 0 && vkCode < MAX_KEYS)
	{
		if (m_keyDown[vkCode])
			m_keyReleasedAccum[vkCode] = true;
		m_keyDown[vkCode] = false;
	}
}

void KeyboardMouseInput::OnMouseButtonDown(int button)
{
	if (button >= 0 && button < MAX_MOUSE_BUTTONS)
	{
		if (!m_mouseButtonDown[button])
			m_mouseBtnPressedAccum[button] = true;
		m_mouseButtonDown[button] = true;
	}
}

void KeyboardMouseInput::OnMouseButtonUp(int button)
{
	if (button >= 0 && button < MAX_MOUSE_BUTTONS)
	{
		if (m_mouseButtonDown[button])
			m_mouseBtnReleasedAccum[button] = true;
		m_mouseButtonDown[button] = false;
	}
}

void KeyboardMouseInput::OnMouseMove(int x, int y)
{
	m_mouseX = x;
	m_mouseY = y;
}

void KeyboardMouseInput::OnMouseWheel(int delta)
{
	m_mouseWheelAccum += delta;
}

void KeyboardMouseInput::OnRawMouseDelta(int dx, int dy)
{
	m_mouseDeltaAccumX += dx;
	m_mouseDeltaAccumY += dy;
}

bool KeyboardMouseInput::IsKeyDown(int vkCode) const
{
	if (vkCode >= 0 && vkCode < MAX_KEYS)
		return m_keyDown[vkCode];
	return false;
}

bool KeyboardMouseInput::IsKeyPressed(int vkCode) const
{
	if (vkCode >= 0 && vkCode < MAX_KEYS)
		return m_keyPressed[vkCode];
	return false;
}

bool KeyboardMouseInput::IsKeyReleased(int vkCode) const
{
	if (vkCode >= 0 && vkCode < MAX_KEYS)
		return m_keyReleased[vkCode];
	return false;
}

bool KeyboardMouseInput::IsMouseButtonDown(int button) const
{
	if (button >= 0 && button < MAX_MOUSE_BUTTONS)
		return m_mouseButtonDown[button];
	return false;
}

bool KeyboardMouseInput::IsMouseButtonPressed(int button) const
{
	if (button >= 0 && button < MAX_MOUSE_BUTTONS)
		return m_mouseBtnPressed[button];
	return false;
}

bool KeyboardMouseInput::IsMouseButtonReleased(int button) const
{
	if (button >= 0 && button < MAX_MOUSE_BUTTONS)
		return m_mouseBtnReleased[button];
	return false;
}

void KeyboardMouseInput::SetMouseGrabbed(bool grabbed)
{
	if (m_mouseGrabbed == grabbed)
		return;

	m_mouseGrabbed = grabbed;
	if (grabbed && g_hWnd)
	{
		while (ShowCursor(FALSE) >= 0) {}
		ClipCursorToWindow(g_hWnd);
		
		RECT rc;
		GetClientRect(g_hWnd, &rc);
		POINT center;
		center.x = (rc.right - rc.left) / 2;
		center.y = (rc.bottom - rc.top) / 2;
		ClientToScreen(g_hWnd, &center);
		SetCursorPos(center.x, center.y);

		m_mouseDeltaAccumX = 0;
		m_mouseDeltaAccumY = 0;
	}
	else if (!grabbed && !m_cursorHiddenForUI && g_hWnd)
	{
		while (ShowCursor(TRUE) < 0) {}
		ClipCursor(NULL);
	}
}

void KeyboardMouseInput::SetCursorHiddenForUI(bool hidden)
{
	if (m_cursorHiddenForUI == hidden)
		return;

	m_cursorHiddenForUI = hidden;
	if (hidden && g_hWnd)
	{
		while (ShowCursor(FALSE) >= 0) {}
		ClipCursorToWindow(g_hWnd);

		RECT rc;
		GetClientRect(g_hWnd, &rc);
		POINT center;
		center.x = (rc.right - rc.left) / 2;
		center.y = (rc.bottom - rc.top) / 2;
		ClientToScreen(g_hWnd, &center);
		SetCursorPos(center.x, center.y);

		m_mouseDeltaAccumX = 0;
		m_mouseDeltaAccumY = 0;
	}
	else if (!hidden && !m_mouseGrabbed && g_hWnd)
	{
		while (ShowCursor(TRUE) < 0) {}
		ClipCursor(NULL);
	}
}

static void ClipCursorToWindow(HWND hWnd)
{
	if (!hWnd) return;
	RECT rc;
	GetClientRect(hWnd, &rc);
	POINT topLeft = { rc.left, rc.top };
	POINT bottomRight = { rc.right, rc.bottom };
	ClientToScreen(hWnd, &topLeft);
	ClientToScreen(hWnd, &bottomRight);
	RECT clipRect = { topLeft.x, topLeft.y, bottomRight.x, bottomRight.y };
	ClipCursor(&clipRect);
}

void KeyboardMouseInput::SetWindowFocused(bool focused)
{
	m_windowFocused = focused;
	if (focused)
	{
		if (m_mouseGrabbed || m_cursorHiddenForUI)
		{
			while (ShowCursor(FALSE) >= 0) {}
			ClipCursorToWindow(g_hWnd);
		}
		else
		{
			while (ShowCursor(TRUE) < 0) {}
			ClipCursor(NULL);
		}
	}
	else
	{
		while (ShowCursor(TRUE) < 0) {}
		ClipCursor(NULL);
	}
}

float KeyboardMouseInput::GetMoveX() const
{
	float x = 0.0f;
	if (m_keyDown[KEY_LEFT])  x += 1.0f;
	if (m_keyDown[KEY_RIGHT]) x -= 1.0f;
	return x;
}

float KeyboardMouseInput::GetMoveY() const
{
	float y = 0.0f;
	if (m_keyDown[KEY_FORWARD])  y += 1.0f;
	if (m_keyDown[KEY_BACKWARD]) y -= 1.0f;
	return y;
}

float KeyboardMouseInput::GetLookX(float sensitivity) const
{
	return (float)m_mouseDeltaX * sensitivity;
}

float KeyboardMouseInput::GetLookY(float sensitivity) const
{
	return (float)(-m_mouseDeltaY) * sensitivity;
}

void KeyboardMouseInput::SetKBMActive(bool active)
{
	m_kbmActive = active;
}

void KeyboardMouseInput::OnCharInput(wchar_t ch)
{
	if (m_charInputCount < 64)
		m_charInputBuffer[m_charInputCount++] = ch;
}

wchar_t KeyboardMouseInput::PopCharInput()
{
	if (m_charInputCount <= 0) return 0;
	wchar_t ch = m_charInputBuffer[0];
	for (int i = 1; i < m_charInputCount; i++)
		m_charInputBuffer[i - 1] = m_charInputBuffer[i];
	m_charInputCount--;
	return ch;
}
