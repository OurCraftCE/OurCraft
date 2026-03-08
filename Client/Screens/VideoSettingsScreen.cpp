#include "stdafx.h"
#include "VideoSettingsScreen.h"
#include "../UI/SmallButton.h"
#include "../UI/SlideButton.h"
#include "../Game/Options.h"
#include "ControlsScreen.h"
#include "..\World\FwdHeaders/net.minecraft.locale.h"

// Common resolutions for the resolution cycle button
struct Resolution { int w; int h; const wchar_t* name; };
static const Resolution s_resolutions[] = {
	{ 1280,  720, L"1280x720 (720p)" },
	{ 1600,  900, L"1600x900" },
	{ 1920, 1080, L"1920x1080 (1080p)" },
	{ 2560, 1080, L"2560x1080 (21:9)" },
	{ 2560, 1440, L"2560x1440 (1440p)" },
	{ 3440, 1440, L"3440x1440 (21:9)" },
	{ 3840, 2160, L"3840x2160 (4K)" },
};
static const int s_numResolutions = sizeof(s_resolutions) / sizeof(s_resolutions[0]);

static const int s_fpsLimits[] = { 0, 30, 60, 120, 144, 240 };
static const int s_numFpsLimits = sizeof(s_fpsLimits) / sizeof(s_fpsLimits[0]);

VideoSettingsScreen::VideoSettingsScreen(Screen *lastScreen, Options *options)
{
	this->title = L"Video Settings"; // 4J - added
    this->lastScreen = lastScreen;
    this->options = options;
    this->resolutionIndex = 2; // default 1920x1080
}

wstring VideoSettingsScreen::getResolutionString()
{
	return L"Resolution: " + wstring(s_resolutions[resolutionIndex].name);
}

wstring VideoSettingsScreen::getWindowModeString()
{
	if (options->borderlessFullscreen) return L"Window Mode: Borderless";
	if (options->windowFullscreen) return L"Window Mode: Fullscreen";
	return L"Window Mode: Windowed";
}

wstring VideoSettingsScreen::getFPSLimitString()
{
	if (options->targetFPS == 0) return L"FPS Limit: Unlimited";
	return L"FPS Limit: " + _toString<int>(options->targetFPS);
}

void VideoSettingsScreen::cycleResolution()
{
	resolutionIndex = (resolutionIndex + 1) % s_numResolutions;
	options->windowWidth = s_resolutions[resolutionIndex].w;
	options->windowHeight = s_resolutions[resolutionIndex].h;
}

void VideoSettingsScreen::cycleWindowMode()
{
	if (!options->windowFullscreen && !options->borderlessFullscreen)
	{
		options->windowFullscreen = true;
		options->borderlessFullscreen = false;
	}
	else if (options->windowFullscreen && !options->borderlessFullscreen)
	{
		options->windowFullscreen = false;
		options->borderlessFullscreen = true;
	}
	else
	{
		options->windowFullscreen = false;
		options->borderlessFullscreen = false;
	}
}

void VideoSettingsScreen::cycleFPSLimit()
{
	for (int i = 0; i < s_numFpsLimits; i++)
	{
		if (s_fpsLimits[i] == options->targetFPS)
		{
			options->targetFPS = s_fpsLimits[(i + 1) % s_numFpsLimits];
			return;
		}
	}
	options->targetFPS = 0;
}

void VideoSettingsScreen::init()
{
    Language *language = Language::getInstance();
    this->title = language->getElement(L"options.videoTitle");

	// Find current resolution index
	for (int i = 0; i < s_numResolutions; i++)
	{
		if (s_resolutions[i].w == options->windowWidth && s_resolutions[i].h == options->windowHeight)
		{
			resolutionIndex = i;
			break;
		}
	}

	// 4J - this was as static array but moving it into the function to remove any issues with static initialisation order
	const Options::Option *items[8] = {
			        Options::Option::GRAPHICS, Options::Option::RENDER_DISTANCE, Options::Option::AMBIENT_OCCLUSION, Options::Option::FRAMERATE_LIMIT, Options::Option::ANAGLYPH, Options::Option::VIEW_BOBBING,
			        Options::Option::GUI_SCALE, Options::Option::ADVANCED_OPENGL
	};

	for (int position = 0; position < 8; position++)
	{
		const Options::Option *item = items[position];
        if (!item->isProgress())
		{
            buttons.push_back(new SmallButton(item->getId(), width / 2 - 155 + position % 2 * 160, height / 6 + 24 * (position >> 1), item, options->getMessage(item)));
        }
		else
		{
            buttons.push_back(new SlideButton(item->getId(), width / 2 - 155 + position % 2 * 160, height / 6 + 24 * (position >> 1), item, options->getMessage(item), options->getProgressValue(item)));
        }
    }

	// PC video settings buttons
	int yBase = height / 6 + 24 * 4 + 12;
	buttons.push_back(new Button(BUTTON_RESOLUTION, width / 2 - 155, yBase, 150, 20, getResolutionString()));
	buttons.push_back(new Button(BUTTON_WINDOW_MODE, width / 2 + 5, yBase, 150, 20, getWindowModeString()));
	buttons.push_back(new Button(BUTTON_FPS_LIMIT, width / 2 - 155, yBase + 24, 150, 20, getFPSLimitString()));

    buttons.push_back(new Button(200, width / 2 - 100, height / 6 + 24 * 7, language->getElement(L"gui.done")));

}

void VideoSettingsScreen::buttonClicked(Button *button)
{
    if (!button->active) return;
    if (button->id < 100 && (dynamic_cast<SmallButton *>(button) != NULL))
	{
        options->toggle(((SmallButton *) button)->getOption(), 1);
        button->msg = options->getMessage(Options::Option::getItem(button->id));
    }

    switch (button->id)
    {
    case BUTTON_RESOLUTION:
        cycleResolution();
        button->msg = getResolutionString();
        break;
    case BUTTON_WINDOW_MODE:
        cycleWindowMode();
        button->msg = getWindowModeString();
        break;
    case BUTTON_FPS_LIMIT:
        cycleFPSLimit();
        button->msg = getFPSLimitString();
        break;
    case 200:
        minecraft->options->save();
        minecraft->setScreen(lastScreen);
        break;
    }

    ScreenSizeCalculator ssc(minecraft->options, minecraft->width, minecraft->height);
    int screenWidth = ssc.getWidth();
    int screenHeight = ssc.getHeight();
    Screen::init(minecraft, screenWidth, screenHeight);	// 4J - was this.init
}

void VideoSettingsScreen::render(int xm, int ym, float a)
{
    renderBackground();
    drawCenteredString(font, title, width / 2, 20, 0xffffff);
    Screen::render(xm, ym, a);
}
