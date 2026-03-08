#include "stdafx.h"
#include "ControlsScreen.h"
#include "../Game/Options.h"
#include "../UI/SmallButton.h"
#include "..\World\FwdHeaders/net.minecraft.locale.h"
#ifdef _WINDOWS64
#include "../Input/KeyboardMouseInput.h"
#endif

ControlsScreen::ControlsScreen(Screen *lastScreen, Options *options)
{
	// 4J - added initialisers
	title == L"Controls";
	selectedKey = -1;

	this->lastScreen = lastScreen;
    this->options = options;
}

int ControlsScreen::getLeftScreenPosition()
{
	return width / 2 - 155;
}

void ControlsScreen::init()
{
    Language *language = Language::getInstance();

    int leftPos = getLeftScreenPosition();
    for (int i = 0; i < Options::keyMappings_length; i++)
	{
        buttons.push_back(new SmallButton(i, leftPos + i % 2 * ROW_WIDTH, height / 6 + 24 * (i >> 1), BUTTON_WIDTH, 20, options->getKeyMessage(i)));
    }

    buttons.push_back(new Button(200, width / 2 - 100, height / 6 + 24 * 7, language->getElement(L"gui.done")));
    title = language->getElement(L"controls.title");

}

void ControlsScreen::buttonClicked(Button *button)
{
    for (int i = 0; i < Options::keyMappings_length; i++)
	{
        buttons[i]->msg = options->getKeyMessage(i);
    }
    if (button->id == 200)
	{
        minecraft->setScreen(lastScreen);
    }
	else
	{
        selectedKey = button->id;
        button->msg = L"> " + options->getKeyMessage(button->id) + L" <";
    }
}

void ControlsScreen::keyPressed(wchar_t eventCharacter, int eventKey)
{
    if (selectedKey >= 0)
	{
        options->setKey(selectedKey, eventKey);
        buttons[selectedKey]->msg = options->getKeyMessage(selectedKey);
        selectedKey = -1;
    }
	else
	{
        Screen::keyPressed(eventCharacter, eventKey);
    }
}

void ControlsScreen::render(int xm, int ym, float a)
{
#ifdef _WINDOWS64
	// When waiting for a key rebind, also accept mouse button clicks
	if (selectedKey >= 0)
	{
		for (int btn = 0; btn < KeyboardMouseInput::MAX_MOUSE_BUTTONS; btn++)
		{
			if (g_KBMInput.IsMouseButtonPressed(btn))
			{
				// Store mouse button as negative code: -100 + button (matches KeyMapping convention)
				options->setKey(selectedKey, -100 + btn);
				buttons[selectedKey]->msg = options->getKeyMessage(selectedKey);
				selectedKey = -1;
				break;
			}
		}
	}
#endif

    renderBackground();
    drawCenteredString(font, title, width / 2, 20, 0xffffff);

    int leftPos = getLeftScreenPosition();
    for (int i = 0; i < Options::keyMappings_length; i++)
	{
        drawString(font, options->getKeyDescription(i), leftPos + i % 2 * ROW_WIDTH + BUTTON_WIDTH + 6, height / 6 + 24 * (i >> 1) + 7, 0xffffffff);
    }

    Screen::render(xm, ym, a);
}