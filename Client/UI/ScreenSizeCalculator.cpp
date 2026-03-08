#include "stdafx.h"
#include "ScreenSizeCalculator.h"
#include "../Game/Options.h"

ScreenSizeCalculator::ScreenSizeCalculator(Options *options, int width, int height, int forceScale/*=-1*/)
{
    w = width;
    h = height;
	if( forceScale == -1 )
	{
		scale = 1;

		int maxScale = options->guiScale;
		if (maxScale == 0) maxScale = 1000;
		// Scale based on both dimensions so non-16:9 ratios work correctly
		while (scale < maxScale && w / (scale + 1) >= 320 && h / (scale + 1) >= 240)
		{
			scale++;
		}
	}
	else
	{
		scale = forceScale;
		if (scale < 1) scale = 1;
	}
    rawWidth = w / (double) scale;
    rawHeight = h / (double) scale;
    w = (int) ceil(rawWidth);
    h = (int) ceil(rawHeight);
}

int ScreenSizeCalculator::getWidth()
{
	return w;
}

int ScreenSizeCalculator::getHeight()
{
	return h;
}