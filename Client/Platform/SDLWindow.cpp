#define SDL_MAIN_HANDLED
#include "SDLWindow.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

bool SDLWindow::Init(const char* title, int width, int height, bool fullscreen)
{
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO))
	{
		SDL_Log("SDL_Init failed: %s", SDL_GetError());
		return false;
	}

	m_width = width;
	m_height = height;
	m_fullscreen = fullscreen;

	Uint32 flags = SDL_WINDOW_RESIZABLE;
	if (fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN;

	m_window = SDL_CreateWindow(title, width, height, flags);
	if (!m_window)
	{
		SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
		return false;
	}

	return true;
}

void SDLWindow::Shutdown()
{
	if (m_window)
	{
		SDL_DestroyWindow(m_window);
		m_window = nullptr;
	}
	SDL_Quit();
}

bool SDLWindow::PollEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_EVENT_QUIT:
			m_shouldClose = true;
			return false;

		case SDL_EVENT_WINDOW_RESIZED:
			m_width = event.window.data1;
			m_height = event.window.data2;
			break;

		case SDL_EVENT_WINDOW_FOCUS_GAINED:
			m_focused = true;
			break;

		case SDL_EVENT_WINDOW_FOCUS_LOST:
			m_focused = false;
			if (m_cursorGrabbed)
				SetCursorGrabbed(false);
			break;
		}
	}

	return !m_shouldClose;
}

void* SDLWindow::GetNativeWindowHandle() const
{
#ifdef _WIN32
	SDL_PropertiesID props = SDL_GetWindowProperties(m_window);
	return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#else
	return nullptr;
#endif
}

void* SDLWindow::GetNativeDisplayHandle() const
{
	return nullptr; // not needed on Windows; used on Linux (X11/Wayland)
}

void SDLWindow::SetFullscreen(bool fullscreen)
{
	if (m_fullscreen == fullscreen)
		return;

	SDL_SetWindowFullscreen(m_window, fullscreen);
	m_fullscreen = fullscreen;
}

void SDLWindow::ToggleFullscreen()
{
	SetFullscreen(!m_fullscreen);
}

void SDLWindow::SetCursorGrabbed(bool grabbed)
{
	m_cursorGrabbed = grabbed;
	SDL_SetWindowRelativeMouseMode(m_window, grabbed);
}

void SDLWindow::SetCursorVisible(bool visible)
{
	if (visible)
		SDL_ShowCursor();
	else
		SDL_HideCursor();
}
