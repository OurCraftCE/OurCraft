#pragma once
#include "Screen.h"
class Options;
using namespace std;

class VideoSettingsScreen : public Screen
{
private:
	Screen *lastScreen;
protected:
	wstring title;
private:
	Options *options;

	// Button IDs for new video settings
	static const int BUTTON_RESOLUTION = 101;
	static const int BUTTON_WINDOW_MODE = 102;
	static const int BUTTON_FPS_LIMIT = 103;

	int resolutionIndex;
	void cycleResolution();
	void cycleWindowMode();
	void cycleFPSLimit();
	wstring getResolutionString();
	wstring getWindowModeString();
	wstring getFPSLimitString();

public:
	VideoSettingsScreen(Screen *lastScreen, Options *options);
    virtual void init();
protected:
	virtual void buttonClicked(Button *button);
public:
	virtual void render(int xm, int ym, float a);
};