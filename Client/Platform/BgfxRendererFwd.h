#pragma once

// forward declaration — use in files that don't need the full bgfx API (avoids C++20 requirement)

class SDLWindow;

class BgfxRenderer
{
public:
	bool Init(SDLWindow& window, int width, int height);
	void Shutdown();
	void Resize(int width, int height);
	void StartFrame();
	void Present();
};

extern BgfxRenderer g_BgfxRenderer;
