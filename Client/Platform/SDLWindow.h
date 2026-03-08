#pragma once

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

struct bgfx_platform_data;

class SDLWindow
{
public:
	bool Init(const char* title, int width, int height, bool fullscreen = false);
	void Shutdown();

	// returns false when SDL_EVENT_QUIT received
	bool PollEvents();

	bool ShouldClose() const { return m_shouldClose; }

	SDL_Window* GetSDLHandle() const { return m_window; }

	// native HWND for bgfx/D3D init
	void* GetNativeWindowHandle() const;
	void* GetNativeDisplayHandle() const;

	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }
	bool IsFullscreen() const { return m_fullscreen; }
	bool IsFocused() const { return m_focused; }

	void SetFullscreen(bool fullscreen);
	void ToggleFullscreen();

	// called by SDLEventBridge when window is resized
	void OnResize(int w, int h) { m_width = w; m_height = h; }

	void SetCursorGrabbed(bool grabbed);
	bool IsCursorGrabbed() const { return m_cursorGrabbed; }
	void SetCursorVisible(bool visible);

private:
	SDL_Window* m_window = nullptr;
	int m_width = 1920;
	int m_height = 1080;
	bool m_fullscreen = false;
	bool m_focused = true;
	bool m_shouldClose = false;
	bool m_cursorGrabbed = false;
};
