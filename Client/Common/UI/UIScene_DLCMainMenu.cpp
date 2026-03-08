#include "stdafx.h"
#include "UI.h"
#include "UIScene_DLCMainMenu.h"

#define PLAYER_ONLINE_TIMER_ID 0
#define PLAYER_ONLINE_TIMER_TIME 100

UIScene_DLCMainMenu::UIScene_DLCMainMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	// Setup all the Iggy references we need for this scene
	initialiseMovie();

	m_labelOffers.init(app.GetString(IDS_DOWNLOADABLE_CONTENT_OFFERS));
	m_buttonListOffers.init(eControl_OffersList);

	if(m_loadedResolution == eSceneResolution_1080)
	{
		m_labelXboxStore.init( app.GetString(IDS_XBOX_STORE) );
	}

	m_Timer.setVisible(false);

	// Refresh the local DLC catalog
	app.m_dlcCatalog.Refresh(L"dlc\\catalog.ini", L"dlc\\installed");

	// Only show categories that have entries in the catalog
	m_buttonListOffers.addItem(app.GetString(IDS_DLC_MENU_SKINPACKS), e_DLC_SkinPack);
	m_buttonListOffers.addItem(app.GetString(IDS_DLC_MENU_TEXTUREPACKS), e_DLC_TexturePacks);
	m_buttonListOffers.addItem(app.GetString(IDS_DLC_MENU_MASHUPPACKS), e_DLC_MashupPacks);

	TelemetryManager->RecordMenuShown(iPad, eUIScene_DLCMainMenu, 0);
}

UIScene_DLCMainMenu::~UIScene_DLCMainMenu()
{
}

wstring UIScene_DLCMainMenu::getMoviePath()
{
	return L"DLCMainMenu";
}

void UIScene_DLCMainMenu::updateTooltips()
{
	ui.SetTooltips( m_iPad, IDS_TOOLTIPS_SELECT, IDS_TOOLTIPS_BACK );
}

void UIScene_DLCMainMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	//app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d, down- %s, pressed- %s, released- %s\n", iPad, key, down?"TRUE":"FALSE", pressed?"TRUE":"FALSE", released?"TRUE":"FALSE");
	ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

	switch(key)
	{
	case ACTION_MENU_CANCEL:
		if(pressed)
		{
			navigateBack();
		}
		break;
	case ACTION_MENU_OK:
		sendInputToMovie(key, repeat, pressed, released);
		break;
	case ACTION_MENU_UP:
	case ACTION_MENU_DOWN:
	case ACTION_MENU_LEFT:
	case ACTION_MENU_RIGHT:
	case ACTION_MENU_PAGEUP:
	case ACTION_MENU_PAGEDOWN:
		sendInputToMovie(key, repeat, pressed, released);
		break;
	}
}

void UIScene_DLCMainMenu::handlePress(F64 controlId, F64 childId)
{
	switch((int)controlId)
	{
	case eControl_OffersList:
		{
			int iIndex = (int)childId;
			DLCOffersParam *param = new DLCOffersParam();
			param->iPad = m_iPad;

			param->iType = iIndex;
			// Navigate to offers for this DLC type (using local catalog)
			killTimer(PLAYER_ONLINE_TIMER_ID);
			ui.NavigateToScene(m_iPad, eUIScene_DLCOffersMenu, param);
			break;
		}
	};
}

void UIScene_DLCMainMenu::handleTimerComplete(int id)
{
}

int UIScene_DLCMainMenu::ExitDLCMainMenu(void *pParam,int iPad,CStorageManager::EMessageResult result)
{
	UIScene_DLCMainMenu* pClass = (UIScene_DLCMainMenu*)pParam;

	pClass->navigateBack();

	return 0;
}

void UIScene_DLCMainMenu::handleGainFocus(bool navBack)
{
	UIScene::handleGainFocus(navBack);

	updateTooltips();

	if(navBack)
	{
		// add the timer back in
	}
}

void UIScene_DLCMainMenu::tick()
{
	UIScene::tick();
}

