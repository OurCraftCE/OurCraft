#include "stdafx.h"
#include "UI.h"
#include "UIScene_HelpAndOptionsMenu.h"
#include "..\..\OurCraft.h"

UIScene_HelpAndOptionsMenu::UIScene_HelpAndOptionsMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	// Setup all the Iggy references we need for this scene
	initialiseMovie();

	m_bNotInGame=(Minecraft::GetInstance()->level==NULL);

	m_buttons[BUTTON_HAO_CHANGESKIN].init(app.GetString(IDS_CHANGE_SKIN),BUTTON_HAO_CHANGESKIN);
	m_buttons[BUTTON_HAO_HOWTOPLAY].init(app.GetString(IDS_HOW_TO_PLAY),BUTTON_HAO_HOWTOPLAY);
	m_buttons[BUTTON_HAO_CONTROLS].init(app.GetString(IDS_CONTROLS),BUTTON_HAO_CONTROLS);
	m_buttons[BUTTON_HAO_SETTINGS].init(app.GetString(IDS_SETTINGS),BUTTON_HAO_SETTINGS);
	m_buttons[BUTTON_HAO_CREDITS].init(app.GetString(IDS_CREDITS),BUTTON_HAO_CREDITS);
	m_buttons[BUTTON_HAO_LANGUAGE].init(L"Language",BUTTON_HAO_LANGUAGE);
	m_buttons[BUTTON_HAO_DEBUG].init(app.GetString(IDS_DEBUG_SETTINGS),BUTTON_HAO_DEBUG);

	doHorizontalResizeCheck();

#ifdef _FINAL_BUILD
	removeControl( &m_buttons[BUTTON_HAO_DEBUG], false);
	if(!app.DebugSettingsOn()) removeControl( &m_buttons[BUTTON_HAO_DEBUG], false);

	// 4J-PB - in order to buy the skin packs, we need the signed offer ids for them, which we get in the availability info
	// we need to retrieve this info though, so do it here
	app.AddDLCRequest(e_Marketplace_Content); // content is skin packs, texture packs and mash-up packs

	// we also need to mount the local DLC so we can tell what's been purchased
	app.StartInstallDLCProcess(iPad);

	if(app.GetLocalPlayerCount()>1)
	{
		// no credits in splitscreen
		removeControl( &m_buttons[BUTTON_HAO_CREDITS], false);

#if TO_BE_IMPLEMENTED
		app.AdjustSplitscreenScene(m_hObj,&m_OriginalPosition,m_iPad,false);
#endif
	}

	if(!ProfileManager.IsFullVersion() )//|| ProfileManager.IsGuest(m_iPad))
	{
		removeControl( &m_buttons[BUTTON_HAO_CHANGESKIN], false);
	}

	//StorageManager.TMSPP_GetUserQuotaInfo(CStorageManager::eGlobalStorage_TitleUser,iPad);
	//StorageManager.WebServiceRequestGetFriends(iPad);
#endif
}

UIScene_HelpAndOptionsMenu::~UIScene_HelpAndOptionsMenu()
{
}

wstring UIScene_HelpAndOptionsMenu::getMoviePath()
{
	if(app.GetLocalPlayerCount() > 1)
	{
		return L"HelpAndOptionsMenuSplit";
	}
	else
	{
		return L"HelpAndOptionsMenu";
	}
}

void UIScene_HelpAndOptionsMenu::updateTooltips()
{
	ui.SetTooltips( m_iPad, IDS_TOOLTIPS_SELECT,IDS_TOOLTIPS_BACK);
}

void UIScene_HelpAndOptionsMenu::updateComponents()
{
	bool bNotInGame=(Minecraft::GetInstance()->level==NULL);
	if(bNotInGame)
	{
		m_parentLayer->showComponent(m_iPad,eUIComponent_Panorama,true);
		m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,true);
	}
	else
	{
		m_parentLayer->showComponent(m_iPad,eUIComponent_Panorama,false);

		if( app.GetLocalPlayerCount() == 1 ) m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,true);
		else m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,false);

	}
}

void UIScene_HelpAndOptionsMenu::handleReload()
{
#ifdef _FINAL_BUILD
	removeControl( &m_buttons[BUTTON_HAO_DEBUG], false);
	if(!app.DebugSettingsOn()) removeControl( &m_buttons[BUTTON_HAO_DEBUG], false);

	if(app.GetLocalPlayerCount()>1)
	{
		// no credits in splitscreen
		removeControl( &m_buttons[BUTTON_HAO_CREDITS], false);

#if TO_BE_IMPLEMENTED
		app.AdjustSplitscreenScene(m_hObj,&m_OriginalPosition,m_iPad,false);
#endif
	}

	if(!ProfileManager.IsFullVersion() )//|| ProfileManager.IsGuest(m_iPad))
	{
#if TO_BE_IMPLEMENTED
		m_Buttons[BUTTON_HAO_CHANGESKIN].SetEnable(FALSE);
		m_Buttons[BUTTON_HAO_CHANGESKIN].EnableInput(FALSE);
		// set the focus to the second button

		XuiElementSetUserFocus(m_Buttons[BUTTON_HAO_HOWTOPLAY].m_hObj, m_iPad);
#endif
	}

	if(!ProfileManager.IsFullVersion() )//|| ProfileManager.IsGuest(m_iPad))
	{
		removeControl( &m_buttons[BUTTON_HAO_CHANGESKIN], false);
	}

	doHorizontalResizeCheck();
#endif
}

void UIScene_HelpAndOptionsMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	//app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d, down- %s, pressed- %s, released- %s\n", iPad, key, down?"TRUE":"FALSE", pressed?"TRUE":"FALSE", released?"TRUE":"FALSE");

	ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

	switch(key)
	{
	case ACTION_MENU_CANCEL:
		if(pressed && !repeat)
		{
			navigateBack();
		}
		break;
	case ACTION_MENU_OK:
	//CD - Added for audio
	if(pressed)
	{
		ui.PlayUISFX(eSFX_Press);
	}

	case ACTION_MENU_UP:
	case ACTION_MENU_DOWN:
		sendInputToMovie(key, repeat, pressed, released);
		break;
	}
}

void UIScene_HelpAndOptionsMenu::handlePress(F64 controlId, F64 childId)
{
	switch((int)controlId)
	{
	case BUTTON_HAO_CHANGESKIN:
		ui.NavigateToScene(m_iPad, eUIScene_SkinSelectMenu);
		break;
	case BUTTON_HAO_HOWTOPLAY:
		ui.NavigateToScene(m_iPad, eUIScene_HowToPlayMenu);
		break;
	case BUTTON_HAO_CONTROLS:
		ui.NavigateToScene(m_iPad, eUIScene_ControlsMenu);
		break;
	case BUTTON_HAO_SETTINGS:
		ui.NavigateToScene(m_iPad, eUIScene_SettingsMenu);
		break;
	case BUTTON_HAO_CREDITS:
		ui.NavigateToScene(m_iPad, eUIScene_Credits);
		break;
	case BUTTON_HAO_LANGUAGE:
		ui.NavigateToScene(m_iPad, eUIScene_LanguageMenu);
		break;
	case BUTTON_HAO_DEBUG:
		ui.NavigateToScene(m_iPad, eUIScene_DebugOptions);
		break;
	}
}
