#include "stdafx.h"
#include "AchievementScreen.h"
#include "../UI/SmallButton.h"
#include "../Game/Options.h"
#include "../Input/KeyMapping.h"
#include "../Textures/Font.h"
#include "../Render/Lighting.h"
#include "../Render/Textures.h"
#include "../Game/StatsCounter.h"
#include "../Render/ItemRenderer.h"
#include "..\World\IO\System.h"
#include "..\World\FwdHeaders/net.minecraft.locale.h"
#include "..\World\FwdHeaders/net.minecraft.world.level.tile.h"
#include "..\World\Math\JavaMath.h"
#ifdef _WINDOWS64
#include "Common\UI\UIScene.h"
#endif



AchievementScreen::AchievementScreen(StatsCounter *statsCounter)
{
	// 4J - added initialisers
    imageWidth = 256;
    imageHeight = 202;
    xLastScroll = 0;
    yLastScroll = 0;
	scrolling = 0;
#ifdef _WINDOWS64
	hiddenIggyScene = NULL;
#endif

	// 4J - TODO - investigate - these were static final ints before, but based on members of Achievements which
	// aren't final Or actually initialised
	xMin = Achievements::xMin * ACHIEVEMENT_COORD_SCALE - BIGMAP_WIDTH / 2;
	yMin = Achievements::yMin * ACHIEVEMENT_COORD_SCALE - BIGMAP_HEIGHT / 2;
	xMax = Achievements::xMax * ACHIEVEMENT_COORD_SCALE - BIGMAP_WIDTH / 2;
	yMax = Achievements::yMax * ACHIEVEMENT_COORD_SCALE - BIGMAP_HEIGHT / 2;

    this->statsCounter = statsCounter;
    int wBigMap = 141;
    int hBigMap = 141;

    xScrollO = xScrollP = xScrollTarget = Achievements::openInventory->x * ACHIEVEMENT_COORD_SCALE - wBigMap / 2 - 12;
    yScrollO = yScrollP = yScrollTarget = Achievements::openInventory->y * ACHIEVEMENT_COORD_SCALE - hBigMap / 2;

}

void AchievementScreen::init()
{
    buttons.clear();
    buttons.push_back(new SmallButton(1, width / 2 + 24, height / 2 + 74, 80, 20, I18n::get(L"gui.done")));
}

void AchievementScreen::buttonClicked(Button *button)
{
    if (button->id == 1)
	{
        minecraft->setScreen(NULL);
    }
    Screen::buttonClicked(button);
}

void AchievementScreen::keyPressed(char eventCharacter, int eventKey)
{
    if (eventKey == minecraft->options->keyBuild->key)
	{
        minecraft->setScreen(NULL);
    }
	else
	{
        Screen::keyPressed(eventCharacter, eventKey);
    }
}

void AchievementScreen::render(int mouseX, int mouseY, float a)
{
    if (Mouse::isButtonDown(0))
	{
        int xo = (width - imageWidth) / 2;
        int yo = (height - imageHeight) / 2;

        int xBigMap = xo + 8;
        int yBigMap = yo + 17;

        if (scrolling == 0 || scrolling == 1)
		{
            if (mouseX >= xBigMap && mouseX < xBigMap + BIGMAP_WIDTH && mouseY >= yBigMap && mouseY < yBigMap + BIGMAP_HEIGHT)
			{
                if (scrolling == 0)
				{
                    scrolling = 1;
                }
				else
				{
                    xScrollP -= mouseX - xLastScroll;
                    yScrollP -= mouseY - yLastScroll;
                    xScrollTarget = xScrollO = xScrollP;
                    yScrollTarget = yScrollO = yScrollP;
                }
                xLastScroll = mouseX;
                yLastScroll = mouseY;
            }
        }

        if (xScrollTarget < xMin) xScrollTarget = xMin;
        if (yScrollTarget < yMin) yScrollTarget = yMin;
        if (xScrollTarget >= xMax) xScrollTarget = xMax - 1;
        if (yScrollTarget >= yMax) yScrollTarget = yMax - 1;
    }
	else
	{
        scrolling = 0;
    }

    renderBackground();

    renderBg(mouseX, mouseY, a);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    renderLabels();

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

}

void AchievementScreen::tick()
{
    xScrollO = xScrollP;
    yScrollO = yScrollP;

    double xd = (xScrollTarget - xScrollP);
    double yd = (yScrollTarget - yScrollP);
    if (xd * xd + yd * yd < 4)
	{
        xScrollP += xd;
        yScrollP += yd;
    }
	else
	{
        xScrollP += xd * 0.85;
        yScrollP += yd * 0.85;
    }
}

void AchievementScreen::renderLabels()
{
    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;
    font->draw(L"Achievements", xo + 15, yo + 5, 0x404040);

//        font.draw(xScrollP + ", " + yScrollP, xo + 5, yo + 5 + BIGMAP_HEIGHT + 18, 0x404040);
//        font.drawWordWrap("Ride a pig off a cliff.", xo + 5, yo + 5 + BIGMAP_HEIGHT + 16, BIGMAP_WIDTH, 0x404040);

}

void AchievementScreen::renderBg(int xm, int ym, float a)
{
#ifdef _WINDOWS64
    int xScroll = Mth::floor(xScrollO + (xScrollP - xScrollO) * a);
    int yScroll = Mth::floor(yScrollO + (yScrollP - yScrollO) * a);

    if (xScroll < xMin) xScroll = xMin;
    if (yScroll < yMin) yScroll = yMin;
    if (xScroll >= xMax) xScroll = xMax - 1;
    if (yScroll >= yMax) yScroll = yMax - 1;

    int tex = minecraft->textures->loadTexture(TN_ACHIEVEMENT_BG);

    int xo = (width - imageWidth) / 2;
    int yo = (height - imageHeight) / 2;

    int xBigMap = xo + BIGMAP_X;
    int yBigMap = yo + BIGMAP_Y;

    blitOffset = 0;
    glDepthFunc(GL_GEQUAL);
    glPushMatrix();
    glTranslatef(0, 0, -200);

    {
        // Simplified background: dark fill (original terrain tile rendering used Tile::tex
        // which no longer exists in the Icon-based texture system)
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        fill(xBigMap, yBigMap, xBigMap + BIGMAP_WIDTH, yBigMap + BIGMAP_HEIGHT, 0xff2b2b2b);
        glEnable(GL_TEXTURE_2D);
    }
    glEnable(GL_DEPTH_TEST);

    glDepthFunc(GL_LEQUAL);

    glDisable(GL_TEXTURE_2D);

    AUTO_VAR(itEnd, Achievements::achievements->end());
    for (AUTO_VAR(it, Achievements::achievements->begin()); it != itEnd; it++)
    {
        Achievement *ach = *it;
        if (ach->requires == NULL) continue;

        int x1 = ach->x * ACHIEVEMENT_COORD_SCALE - (int) xScroll + 11 + xBigMap;
        int y1 = ach->y * ACHIEVEMENT_COORD_SCALE - (int) yScroll + 11 + yBigMap;

        int x2 = ach->requires->x * ACHIEVEMENT_COORD_SCALE - (int) xScroll + 11 + xBigMap;
        int y2 = ach->requires->y * ACHIEVEMENT_COORD_SCALE - (int) yScroll + 11 + yBigMap;

        int color = 0;

        bool taken = statsCounter->hasTaken(ach);
        bool canTake = statsCounter->canTake(ach);

        int alph = (int) (sin(System::currentTimeMillis() % 600 / 600.0 * PI * 2) > 0.6 ? 255 : 130);
        if (taken) color = 0xff707070;
        else if (canTake) color = 0x00ff00 + (alph << 24);
        else color = 0xff000000;

        hLine(x1, x2, y1, color);
        vLine(x2, y1, y2, color);
    }

    Achievement *hoveredAchievement = NULL;
    ItemRenderer *ir = new ItemRenderer();

    glPushMatrix();
    glRotatef(180, 1, 0, 0);
    Lighting::turnOn();
    glPopMatrix();
    glDisable(GL_LIGHTING);
    glEnable(GL_RESCALE_NORMAL);
    glEnable(GL_COLOR_MATERIAL);

    itEnd = Achievements::achievements->end();
    for (AUTO_VAR(it, Achievements::achievements->begin()); it != itEnd; it++)
    {
        Achievement *ach = *it;

        int x = ach->x * ACHIEVEMENT_COORD_SCALE - (int) xScroll;
        int y = ach->y * ACHIEVEMENT_COORD_SCALE - (int) yScroll;

        if (x >= -24 && y >= -24 && x <= BIGMAP_WIDTH && y <= BIGMAP_HEIGHT)
        {
            if (statsCounter->hasTaken(ach))
            {
                float br = 1.0f;
                glColor4f(br, br, br, 1);
            }
            else if (statsCounter->canTake(ach))
            {
                float br = (sin(System::currentTimeMillis() % 600 / 600.0 * PI * 2) < 0.6 ? 0.6f : 0.8f);
                glColor4f(br, br, br, 1);
            }
            else
            {
                float br = 0.3f;
                glColor4f(br, br, br, 1);
            }

            minecraft->textures->bind(tex);
            int xx = xBigMap + x;
            int yy = yBigMap + y;
            if (ach->isGolden())
            {
                this->blit(xx - 2, yy - 2, 26, 202, 26, 26);
            }
            else
            {
                this->blit(xx - 2, yy - 2, 0, 202, 26, 26);
            }

            if (!statsCounter->canTake(ach))
            {
                float br = 0.1f;
                glColor4f(br, br, br, 1);
                ir->setColor = false;
            }
            glEnable(GL_LIGHTING);
            glEnable(GL_CULL_FACE);
            ir->renderGuiItem(minecraft->font, minecraft->textures, ach->icon, xx + 3, yy + 3);
            glDisable(GL_LIGHTING);
            if (!statsCounter->canTake(ach))
            {
                ir->setColor = true;
            }
            glColor4f(1, 1, 1, 1);

            if (xm >= xBigMap && ym >= yBigMap && xm < xBigMap + BIGMAP_WIDTH && ym < yBigMap + BIGMAP_HEIGHT && xm >= xx && xm <= xx + 22 && ym >= yy && ym <= yy + 22) {
                hoveredAchievement = ach;
            }
        }
    }

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glColor4f(1, 1, 1, 1);
    minecraft->textures->bind(tex);
    blit(xo, yo, 0, 0, imageWidth, imageHeight);

    glPopMatrix();

    blitOffset = 0;
    glDepthFunc(GL_LEQUAL);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    Screen::render(xm, ym, a);

    if (hoveredAchievement != NULL)
    {
        Achievement *ach = hoveredAchievement;
        wstring name = ach->name;
        wstring descr = ach->getDescription();

        int x = xm + 12;
        int y = ym - 4;

        if (statsCounter->canTake(ach))
        {
            int twid = Math::_max(font->width(name), 120);
            int thei = font->wordWrapHeight(descr, twid);
            if (statsCounter->hasTaken(ach))
            {
                thei += 12;
            }
            fillGradient(x - 3, y - 3, x + twid + 3, y + thei + 3 + 12, 0xc0000000, 0xc0000000);

            font->drawWordWrap(descr, x, y + 12, twid, 0xffa0a0a0, this->height);
            if (statsCounter->hasTaken(ach))
            {
                font->drawShadow(I18n::get(L"achievement.taken"), x, y + thei + 4, 0xff9090ff);
            }
        }
        else
        {
            int twid = Math::_max(font->width(name), 120);
            wstring msg = I18n::get(L"achievement.requires", ach->requires->name.c_str());
            int thei = font->wordWrapHeight(msg, twid);
            fillGradient(x - 3, y - 3, x + twid + 3, y + thei + 12 + 3, 0xc0000000, 0xc0000000);
            font->drawWordWrap(msg, x, y + 12, twid, 0xff705050, this->height);
        }
        font->drawShadow(name, x, y, statsCounter->canTake(ach) ? ach->isGolden() ? 0xffffff80 : 0xffffffff : ach->isGolden() ? 0xff808040 : 0xff808080);
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    Lighting::turnOff();
#endif
}

void AchievementScreen::removed()
{
}

bool AchievementScreen::isPauseScreen()
{
	return true;
}