#include "stdafx.h"
#include "Screen.h"
#include "../UI/Button.h"
#include "../Particles/GuiParticles.h"
#include "../Render/Tesselator.h"
#include "../Render/Textures.h"
#include "..\World\Entities\SoundTypes.h"
#ifdef _WINDOWS64
#include "../Input/KeyboardMouseInput.h"
#endif



Screen::Screen()	// 4J added
{
	minecraft = NULL;
	width = 0;
    height = 0;
	passEvents = false;
	font = NULL;
	particles = NULL;
	clickedButton = NULL;
}

void Screen::render(int xm, int ym, float a)
{
	AUTO_VAR(itEnd, buttons.end());
	for (AUTO_VAR(it, buttons.begin()); it != itEnd; it++)
	{
        Button *button = *it; //buttons[i];
        button->render(minecraft, xm, ym);
    }
}

void Screen::keyPressed(wchar_t eventCharacter, int eventKey)
{
	if (eventKey == Keyboard::KEY_ESCAPE)
	{
		minecraft->setScreen(NULL);
//    minecraft->grabMouse();	// 4J - removed
	}
}

wstring Screen::getClipboard()
{
#ifdef _WINDOWS64
	wstring result;
	if (OpenClipboard(NULL))
	{
		HANDLE hData = GetClipboardData(CF_UNICODETEXT);
		if (hData)
		{
			wchar_t *pText = (wchar_t *)GlobalLock(hData);
			if (pText)
			{
				result = pText;
				GlobalUnlock(hData);
			}
		}
		CloseClipboard();
	}
	return result;
#else
	return L"";
#endif
}

void Screen::setClipboard(const wstring& str)
{
#ifdef _WINDOWS64
	if (OpenClipboard(NULL))
	{
		EmptyClipboard();
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (str.size() + 1) * sizeof(wchar_t));
		if (hMem)
		{
			wchar_t *pMem = (wchar_t *)GlobalLock(hMem);
			if (pMem)
			{
				wcscpy_s(pMem, str.size() + 1, str.c_str());
				GlobalUnlock(hMem);
				SetClipboardData(CF_UNICODETEXT, hMem);
			}
		}
		CloseClipboard();
	}
#endif
}

void Screen::mouseClicked(int x, int y, int buttonNum)
{
    if (buttonNum == 0)
	{
		AUTO_VAR(itEnd, buttons.end());
		for (AUTO_VAR(it, buttons.begin()); it != itEnd; it++)
		{
            Button *button = *it; //buttons[i];
            if (button->clicked(minecraft, x, y))
			{
                clickedButton = button;
                minecraft->soundEngine->playUI(eSoundType_RANDOM_CLICK, 1, 1);
                buttonClicked(button);
            }
        }
    }
}

void Screen::mouseReleased(int x, int y, int buttonNum)
{
    if (clickedButton!=NULL && buttonNum==0)
	{
        clickedButton->released(x, y);
        clickedButton = NULL;
    }
}

void Screen::buttonClicked(Button *button)
{
}

void Screen::init(Minecraft *minecraft, int width, int height)
{
    particles = new GuiParticles(minecraft);
    this->minecraft = minecraft;
    this->font = minecraft->font;
    this->width = width;
    this->height = height;
    buttons.clear();
    init();
}

void Screen::setSize(int width, int height)
{
    this->width = width;
    this->height = height;
}

void Screen::init()
{
}

void Screen::updateEvents()
{
#ifdef _WINDOWS64
	// Process mouse and keyboard input from KBM system
	int mx = g_KBMInput.GetMouseX() * width / minecraft->width;
	int my = g_KBMInput.GetMouseY() * height / minecraft->height;

	// Left mouse button
	if(g_KBMInput.IsMouseButtonPressed(KeyboardMouseInput::MOUSE_LEFT))
	{
		mouseClicked(mx, my, 0);
	}
	else if(g_KBMInput.IsMouseButtonReleased(KeyboardMouseInput::MOUSE_LEFT))
	{
		mouseReleased(mx, my, 0);
	}

	// Right mouse button
	if(g_KBMInput.IsMouseButtonPressed(KeyboardMouseInput::MOUSE_RIGHT))
	{
		mouseClicked(mx, my, 1);
	}
	else if(g_KBMInput.IsMouseButtonReleased(KeyboardMouseInput::MOUSE_RIGHT))
	{
		mouseReleased(mx, my, 1);
	}

	// Middle mouse button
	if(g_KBMInput.IsMouseButtonPressed(KeyboardMouseInput::MOUSE_MIDDLE))
	{
		mouseClicked(mx, my, 2);
	}

	// Mouse wheel -> scroll events (simulate up/down arrow presses)
	int wheel = g_KBMInput.GetMouseWheel();
	if(wheel > 0)
	{
		keyPressed(0, Keyboard::KEY_UP);
	}
	else if(wheel < 0)
	{
		keyPressed(0, Keyboard::KEY_DOWN);
	}

	// Non-printable / control key events
	if(g_KBMInput.IsKeyPressed(VK_ESCAPE))
	{
		keyPressed(0, Keyboard::KEY_ESCAPE);
	}
	if(g_KBMInput.IsKeyPressed(VK_RETURN))
	{
		keyPressed(0, Keyboard::KEY_RETURN);
	}
	if(g_KBMInput.IsKeyPressed(VK_BACK))
	{
		keyPressed(0, Keyboard::KEY_BACK);
	}
	if(g_KBMInput.IsKeyPressed(VK_TAB))
	{
		keyPressed(0, Keyboard::KEY_TAB);
		tabPressed();
	}
	if(g_KBMInput.IsKeyPressed(VK_UP))
	{
		keyPressed(0, Keyboard::KEY_UP);
	}
	if(g_KBMInput.IsKeyPressed(VK_DOWN))
	{
		keyPressed(0, Keyboard::KEY_DOWN);
	}
	if(g_KBMInput.IsKeyPressed(VK_LEFT))
	{
		keyPressed(0, Keyboard::KEY_LEFT);
	}
	if(g_KBMInput.IsKeyPressed(VK_RIGHT))
	{
		keyPressed(0, Keyboard::KEY_RIGHT);
	}

	// Printable character input: sends both the character AND the key constant.
	// Subclasses decide whether to use the character (text input) or the key (action).
	for(int vk = 0x20; vk <= 0x7E; vk++)
	{
		if(g_KBMInput.IsKeyPressed(vk))
		{
			wchar_t ch = (wchar_t)vk;
			// Convert to lowercase if shift is not held
			if(vk >= 'A' && vk <= 'Z' && !g_KBMInput.IsKeyDown(VK_SHIFT))
			{
				ch = (wchar_t)(vk + 32);
			}

			// Map VK to Keyboard constant for specific action keys
			int keyConst = -1;
			if(vk == 'E') keyConst = Keyboard::KEY_E;
			else if(vk == 'T') keyConst = Keyboard::KEY_T;
			else if(vk == ' ') keyConst = Keyboard::KEY_SPACE;
			else if(vk >= '1' && vk <= '9') keyConst = Keyboard::KEY_1 + (vk - '1');
			else if(vk == '0') keyConst = Keyboard::KEY_1 + 9;

			keyPressed(ch, keyConst);
		}
	}
#else
	/* 4J - TODO
    while (Mouse.next()) {
        mouseEvent();
    }

    while (Keyboard.next()) {
        keyboardEvent();
    }
	*/
#endif
}

void Screen::mouseEvent()
{
	/* 4J - TODO
    if (Mouse.getEventButtonState()) {
        int xm = Mouse.getEventX() * width / minecraft.width;
        int ym = height - Mouse.getEventY() * height / minecraft.height - 1;
        mouseClicked(xm, ym, Mouse.getEventButton());
    } else {
        int xm = Mouse.getEventX() * width / minecraft.width;
        int ym = height - Mouse.getEventY() * height / minecraft.height - 1;
        mouseReleased(xm, ym, Mouse.getEventButton());
    }
	*/
}

void Screen::keyboardEvent()
{
	/* 4J - TODO
    if (Keyboard.getEventKeyState()) {
        if (Keyboard.getEventKey() == Keyboard.KEY_F11) {
            minecraft.toggleFullScreen();
            return;
        }
        keyPressed(Keyboard.getEventCharacter(), Keyboard.getEventKey());
    }
	*/
}

void Screen::tick()
{
}

void Screen::removed()
{
}

void Screen::renderBackground()
{
	renderBackground(0);
}

void Screen::renderBackground(int vo)
{
	if (minecraft->level != NULL)
	{
		fillGradient(0, 0, width, height, 0xc0101010, 0xd0101010);
	}
	else
	{
		renderDirtBackground(vo);
	}
}

void Screen::renderDirtBackground(int vo)
{
#ifdef _WINDOWS64
	// Simple dark background (original used loadTexture with old API)
	fillGradient(0, 0, width, height, 0xff101010, 0xff202020);
#else
	// 4J Unused - old Java API (loadTexture signature changed)
#if 0
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    Tesselator *t = Tesselator::getInstance();
    glBindTexture(GL_TEXTURE_2D, minecraft->textures->loadTexture(L"/gui/background.png"));
    glColor4f(1, 1, 1, 1);
    float s = 32;
    t->begin();
    t->color(0x404040);
    t->vertexUV((float)(0), (float)( height), (float)( 0), (float)( 0), (float)( height / s + vo));
    t->vertexUV((float)(width), (float)( height), (float)( 0), (float)( width / s), (float)( height / s + vo));
    t->vertexUV((float)(width), (float)( 0), (float)( 0), (float)( width / s), (float)( 0 + vo));
    t->vertexUV((float)(0), (float)( 0), (float)( 0), (float)( 0), (float)( 0 + vo));
    t->end();
#endif
#endif
}

bool Screen::isPauseScreen()
{
	return true;
}

void Screen::confirmResult(bool result, int id)
{
}

void Screen::tabPressed()
{
}
