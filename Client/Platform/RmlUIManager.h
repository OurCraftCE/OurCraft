#pragma once

#include "RmlBgfxRenderer.h"
#include "RmlSystemInterface.h"

union SDL_Event;

namespace Rml { class Context; class ElementDocument; }

class RmlUIManager {
public:
	static RmlUIManager& Get();

	bool Init(int width, int height);
	void Shutdown();

	void SetViewportSize(int width, int height);

	void Update();
	void Render();

	void ProcessSDLEvent(const SDL_Event& event);

	Rml::Context* GetContext() { return m_context; }
	Rml::ElementDocument* LoadDocument(const char* path);

private:
	RmlUIManager() = default;
	~RmlUIManager() = default;

	RmlBgfxRenderer m_renderer;
	RmlSDLSystemInterface m_systemInterface;
	Rml::Context* m_context = nullptr;
	bool m_initialized = false;
};
