#include "stdafx.h"
#include "UI.h"
#include "UIScene_Intro.h"


UIScene_Intro::UIScene_Intro(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	// Setup all the Iggy references we need for this scene
	initialiseMovie();
	m_bIgnoreNavigate = false;
	m_bAnimationEnded = false;

	bool bSkipESRB = true;

	// 4J Stu - These map to values in the Actionscript
	int platformIdx = 0;

	IggyDataValue result;
	IggyDataValue value[2];
	value[0].type = IGGY_DATATYPE_number;
	value[0].number = platformIdx;

	value[1].type = IGGY_DATATYPE_boolean;
	value[1].boolval = bSkipESRB;
	IggyResult out = IggyPlayerCallMethodRS ( getMovie() , &result, IggyPlayerRootPath( getMovie() ), m_funcSetIntroPlatform , 2 , value );

}

wstring UIScene_Intro::getMoviePath()
{
	return L"Intro";
}

void UIScene_Intro::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

	switch(key)
	{
	case ACTION_MENU_OK:
		if(!m_bIgnoreNavigate)
		{
			m_bIgnoreNavigate = true;
			//ui.NavigateToHomeMenu();
			ui.NavigateToScene(0,eUIScene_MainMenu);
		}
		break;
	}
}


void UIScene_Intro::handleAnimationEnd()
{
	if(!m_bIgnoreNavigate)
	{
		m_bIgnoreNavigate = true;
		//ui.NavigateToHomeMenu();
		// Don't navigate to the main menu if we don't have focus, as we could have the quadrant sign-in or a join game timer screen running, and then when Those finish they'll
		// give the main menu focus which clears the signed in players and therefore breaks transitioning into the game
		if( hasFocus( m_iPad ) )
		{
			ui.NavigateToScene(0,eUIScene_MainMenu);
		}
		else
		{
			m_bAnimationEnded = true;
		}
	}
}

void UIScene_Intro::handleGainFocus(bool navBack)
{
	// Only relevant on xbox one - if we didn't navigate to the main menu at animation end due to the timer or quadrant sign-in being up, then we'll need to
	// do it now in case the user has cancelled or joining a game failed
	if( m_bAnimationEnded )
	{
		ui.NavigateToScene(0,eUIScene_MainMenu);
	}
}
