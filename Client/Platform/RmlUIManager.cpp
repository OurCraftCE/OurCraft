#include "RmlUIManager.h"
#include "UIScreenManager.h"
#include "HUDOverlay.h"
#include "InventoryOverlay.h"
#include "CraftingOverlay.h"
#include "ChatOverlay.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Input.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

static Rml::Input::KeyIdentifier SDLKeyToRml(SDL_Keycode key)
{
	using KI = Rml::Input::KeyIdentifier;
	switch (key)
	{
	case SDLK_A: return KI::KI_A;
	case SDLK_B: return KI::KI_B;
	case SDLK_C: return KI::KI_C;
	case SDLK_D: return KI::KI_D;
	case SDLK_E: return KI::KI_E;
	case SDLK_F: return KI::KI_F;
	case SDLK_G: return KI::KI_G;
	case SDLK_H: return KI::KI_H;
	case SDLK_I: return KI::KI_I;
	case SDLK_J: return KI::KI_J;
	case SDLK_K: return KI::KI_K;
	case SDLK_L: return KI::KI_L;
	case SDLK_M: return KI::KI_M;
	case SDLK_N: return KI::KI_N;
	case SDLK_O: return KI::KI_O;
	case SDLK_P: return KI::KI_P;
	case SDLK_Q: return KI::KI_Q;
	case SDLK_R: return KI::KI_R;
	case SDLK_S: return KI::KI_S;
	case SDLK_T: return KI::KI_T;
	case SDLK_U: return KI::KI_U;
	case SDLK_V: return KI::KI_V;
	case SDLK_W: return KI::KI_W;
	case SDLK_X: return KI::KI_X;
	case SDLK_Y: return KI::KI_Y;
	case SDLK_Z: return KI::KI_Z;
	case SDLK_0: return KI::KI_0;
	case SDLK_1: return KI::KI_1;
	case SDLK_2: return KI::KI_2;
	case SDLK_3: return KI::KI_3;
	case SDLK_4: return KI::KI_4;
	case SDLK_5: return KI::KI_5;
	case SDLK_6: return KI::KI_6;
	case SDLK_7: return KI::KI_7;
	case SDLK_8: return KI::KI_8;
	case SDLK_9: return KI::KI_9;
	case SDLK_BACKSPACE: return KI::KI_BACK;
	case SDLK_TAB: return KI::KI_TAB;
	case SDLK_RETURN: return KI::KI_RETURN;
	case SDLK_ESCAPE: return KI::KI_ESCAPE;
	case SDLK_SPACE: return KI::KI_SPACE;
	case SDLK_LEFT: return KI::KI_LEFT;
	case SDLK_RIGHT: return KI::KI_RIGHT;
	case SDLK_UP: return KI::KI_UP;
	case SDLK_DOWN: return KI::KI_DOWN;
	case SDLK_INSERT: return KI::KI_INSERT;
	case SDLK_DELETE: return KI::KI_DELETE;
	case SDLK_HOME: return KI::KI_HOME;
	case SDLK_END: return KI::KI_END;
	case SDLK_PAGEUP: return KI::KI_PRIOR;
	case SDLK_PAGEDOWN: return KI::KI_NEXT;
	case SDLK_F1: return KI::KI_F1;
	case SDLK_F2: return KI::KI_F2;
	case SDLK_F3: return KI::KI_F3;
	case SDLK_F4: return KI::KI_F4;
	case SDLK_F5: return KI::KI_F5;
	case SDLK_F6: return KI::KI_F6;
	case SDLK_F7: return KI::KI_F7;
	case SDLK_F8: return KI::KI_F8;
	case SDLK_F9: return KI::KI_F9;
	case SDLK_F10: return KI::KI_F10;
	case SDLK_F11: return KI::KI_F11;
	case SDLK_F12: return KI::KI_F12;
	case SDLK_LSHIFT: return KI::KI_LSHIFT;
	case SDLK_RSHIFT: return KI::KI_RSHIFT;
	case SDLK_LCTRL: return KI::KI_LCONTROL;
	case SDLK_RCTRL: return KI::KI_RCONTROL;
	case SDLK_LALT: return KI::KI_LMENU;
	case SDLK_RALT: return KI::KI_RMENU;
	default: return KI::KI_UNKNOWN;
	}
}

static int SDLModToRml(SDL_Keymod mod)
{
	int rmlMod = 0;
	if (mod & SDL_KMOD_CTRL)  rmlMod |= Rml::Input::KM_CTRL;
	if (mod & SDL_KMOD_SHIFT) rmlMod |= Rml::Input::KM_SHIFT;
	if (mod & SDL_KMOD_ALT)   rmlMod |= Rml::Input::KM_ALT;
	return rmlMod;
}

RmlUIManager& RmlUIManager::Get()
{
	static RmlUIManager instance;
	return instance;
}

bool RmlUIManager::Init(int width, int height)
{
	if (m_initialized)
		return true;

	Rml::SetSystemInterface(&m_systemInterface);
	Rml::SetRenderInterface(&m_renderer);

	if (!Rml::Initialise())
	{
		SDL_Log("[RmlUIManager] Rml::Initialise() failed");
		return false;
	}

	if (!m_renderer.Init(width, height))
	{
		SDL_Log("[RmlUIManager] RmlBgfxRenderer::Init() failed");
		Rml::Shutdown();
		return false;
	}

	m_context = Rml::CreateContext("main", Rml::Vector2i(width, height));
	if (!m_context)
	{
		SDL_Log("[RmlUIManager] Rml::CreateContext() failed");
		m_renderer.Shutdown();
		Rml::Shutdown();
		return false;
	}

	if (!Rml::LoadFontFace("Common/Media/font/Mojangles.ttf", true))
	{
		SDL_Log("[RmlUIManager] Mojangles.ttf not found, trying system font");
		if (!Rml::LoadFontFace("C:/Windows/Fonts/segoeui.ttf", true))
		{
			SDL_Log("[RmlUIManager] WARNING: No font loaded, UI text will not render");
		}
	}

	m_initialized = true;
	SDL_Log("[RmlUIManager] Initialized (%dx%d)", width, height);
	return true;
}

void RmlUIManager::Shutdown()
{
	if (!m_initialized)
		return;

	if (m_context)
	{
		Rml::RemoveContext("main");
		m_context = nullptr;
	}

	m_renderer.Shutdown();
	Rml::Shutdown();
	m_initialized = false;

	SDL_Log("[RmlUIManager] Shutdown complete");
}

void RmlUIManager::SetViewportSize(int width, int height)
{
	m_renderer.SetViewportSize(width, height);
	if (m_context)
		m_context->SetDimensions(Rml::Vector2i(width, height));
}

void RmlUIManager::Update()
{
	if (!m_initialized || !m_context)
		return;
	HUDOverlay::Get().Tick();
	InventoryOverlay::Get().Tick();
	CraftingOverlay::Get().Tick();
	m_context->Update();
}

void RmlUIManager::Render()
{
	if (!m_initialized || !m_context)
		return;

	m_renderer.BeginFrame();
	m_context->Render();
}

Rml::ElementDocument* RmlUIManager::LoadDocument(const char* path)
{
	if (!m_context)
		return nullptr;

	Rml::ElementDocument* doc = m_context->LoadDocument(path);
	if (doc)
		doc->Show();
	return doc;
}

void RmlUIManager::ProcessSDLEvent(const SDL_Event& event)
{
	if (!m_initialized || !m_context)
		return;

	if (event.type == SDL_EVENT_KEY_DOWN) {
		if (UIScreenManager::Get().IsWaitingForKey())
			return;

		auto screen = UIScreenManager::Get().GetCurrentScreen();

		if (event.key.key == SDLK_ESCAPE) {
			// Esc closes overlays first, then toggles pause
			if (screen == UIScreenManager::SCREEN_INVENTORY) {
				InventoryOverlay::Get().Hide();
				UIScreenManager::Get().ShowScreen(UIScreenManager::SCREEN_IN_GAME);
				HUDOverlay::Get().Show();
				return;
			}
			if (screen == UIScreenManager::SCREEN_CRAFTING) {
				CraftingOverlay::Get().Hide();
				UIScreenManager::Get().ShowScreen(UIScreenManager::SCREEN_IN_GAME);
				HUDOverlay::Get().Show();
				return;
			}
			if (screen == UIScreenManager::SCREEN_CHAT) {
				ChatOverlay::Get().Hide();
				UIScreenManager::Get().ShowScreen(UIScreenManager::SCREEN_IN_GAME);
				HUDOverlay::Get().Show();
				return;
			}
			if (screen == UIScreenManager::SCREEN_IN_GAME ||
				screen == UIScreenManager::SCREEN_PAUSE) {
				UIScreenManager::Get().TogglePause();
				return;
			}
		}

		if (event.key.key == SDLK_E) {
			if (screen == UIScreenManager::SCREEN_IN_GAME) {
				HUDOverlay::Get().Hide();
				InventoryOverlay::Get().Show();
				UIScreenManager::Get().ShowScreen(UIScreenManager::SCREEN_INVENTORY);
				return;
			}
			if (screen == UIScreenManager::SCREEN_INVENTORY) {
				InventoryOverlay::Get().Hide();
				UIScreenManager::Get().ShowScreen(UIScreenManager::SCREEN_IN_GAME);
				HUDOverlay::Get().Show();
				return;
			}
		}

		if (event.key.key == SDLK_T && screen == UIScreenManager::SCREEN_IN_GAME) {
			HUDOverlay::Get().Hide();
			ChatOverlay::Get().Show();
			UIScreenManager::Get().ShowScreen(UIScreenManager::SCREEN_CHAT);
			return;
		}

		if (event.key.key == SDLK_RETURN && screen == UIScreenManager::SCREEN_CHAT) {
			ChatOverlay::Get().SendCurrentMessage();
			ChatOverlay::Get().Hide();
			UIScreenManager::Get().ShowScreen(UIScreenManager::SCREEN_IN_GAME);
			HUDOverlay::Get().Show();
			return;
		}
	}

	switch (event.type)
	{
	case SDL_EVENT_MOUSE_MOTION:
		m_context->ProcessMouseMove((int)event.motion.x, (int)event.motion.y, SDLModToRml(SDL_GetModState()));
		break;

	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	{
		int button = 0;
		if (event.button.button == SDL_BUTTON_LEFT) button = 0;
		else if (event.button.button == SDL_BUTTON_RIGHT) button = 1;
		else if (event.button.button == SDL_BUTTON_MIDDLE) button = 2;
		m_context->ProcessMouseButtonDown(button, SDLModToRml(SDL_GetModState()));
		break;
	}

	case SDL_EVENT_MOUSE_BUTTON_UP:
	{
		int button = 0;
		if (event.button.button == SDL_BUTTON_LEFT) button = 0;
		else if (event.button.button == SDL_BUTTON_RIGHT) button = 1;
		else if (event.button.button == SDL_BUTTON_MIDDLE) button = 2;
		m_context->ProcessMouseButtonUp(button, SDLModToRml(SDL_GetModState()));
		break;
	}

	case SDL_EVENT_MOUSE_WHEEL:
		m_context->ProcessMouseWheel(Rml::Vector2f(event.wheel.x, -event.wheel.y), SDLModToRml(SDL_GetModState()));
		break;

	case SDL_EVENT_KEY_DOWN:
	{
		Rml::Input::KeyIdentifier key = SDLKeyToRml(event.key.key);
		if (key != Rml::Input::KI_UNKNOWN)
			m_context->ProcessKeyDown(key, SDLModToRml(SDL_GetModState()));
		break;
	}

	case SDL_EVENT_KEY_UP:
	{
		Rml::Input::KeyIdentifier key = SDLKeyToRml(event.key.key);
		if (key != Rml::Input::KI_UNKNOWN)
			m_context->ProcessKeyUp(key, SDLModToRml(SDL_GetModState()));
		break;
	}

	case SDL_EVENT_TEXT_INPUT:
		m_context->ProcessTextInput(event.text.text);
		break;

	default:
		break;
	}
}
