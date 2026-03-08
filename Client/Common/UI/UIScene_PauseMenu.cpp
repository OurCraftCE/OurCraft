#include "stdafx.h"
#include "UI.h"
#include "UIScene_PauseMenu.h"
#include "..\..\MinecraftServer.h"
#include "..\..\Game/MultiPlayerLocalPlayer.h"
#include "../../Textures/TexturePackRepository.h"
#include "../../Textures/TexturePack.h"
#include "../../Textures/DLCTexturePack.h"
#include "..\..\..\World\Math\StringHelpers.h"
#include "../../Screens/AchievementScreen.h"
#include "../../Game/StatsCounter.h"
#include "..\Discord\DiscordManager.h"


UIScene_PauseMenu::UIScene_PauseMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	// Setup all the Iggy references we need for this scene
	initialiseMovie();
	m_bIgnoreInput=false;
	m_eAction=eAction_None;

	m_buttons[BUTTON_PAUSE_RESUMEGAME].init(app.GetString(IDS_RESUME_GAME),BUTTON_PAUSE_RESUMEGAME);
	m_buttons[BUTTON_PAUSE_HELPANDOPTIONS].init(app.GetString(IDS_HELP_AND_OPTIONS),BUTTON_PAUSE_HELPANDOPTIONS);
	m_buttons[BUTTON_PAUSE_ACHIEVEMENTS].init(app.GetString(IDS_ACHIEVEMENTS),BUTTON_PAUSE_ACHIEVEMENTS);
	m_buttons[BUTTON_PAUSE_SAVEGAME].init(app.GetString(IDS_SAVE_GAME),BUTTON_PAUSE_SAVEGAME);
	m_buttons[BUTTON_PAUSE_EXITGAME].init(app.GetString(IDS_EXIT_GAME),BUTTON_PAUSE_EXITGAME);

	if(!ProfileManager.IsFullVersion())
	{
		// hide the trial timer
		ui.ShowTrialTimer(false);
	}

	updateControlsVisibility();

	doHorizontalResizeCheck();

	// get rid of the quadrant display if it's on
	ui.HidePressStart();

#if TO_BE_IMPLEMENTED
	XuiSetTimer(m_hObj,IGNORE_KEYPRESS_TIMERID,IGNORE_KEYPRESS_TIME);
#endif

	if( g_NetworkManager.IsLocalGame() && g_NetworkManager.GetPlayerCount() == 1 )
	{
		app.SetXuiServerAction(ProfileManager.GetPrimaryPad(),eXuiServerAction_PauseServer,(void *)TRUE);
	}

	TelemetryManager->RecordMenuShown(m_iPad, eUIScene_PauseMenu, 0);
	TelemetryManager->RecordPauseOrInactive(m_iPad);

	// Update Discord presence to show paused state
	DiscordManager::Get().SetPausedPresence();

	Minecraft *pMinecraft = Minecraft::GetInstance();
	if(pMinecraft != NULL && pMinecraft->localgameModes[iPad] != NULL )
	{
		TutorialMode *gameMode = (TutorialMode *)pMinecraft->localgameModes[iPad];

		// This just allows it to be shown
		gameMode->getTutorial()->showTutorialPopup(false);
	}
	m_bErrorDialogRunning = false;
}

UIScene_PauseMenu::~UIScene_PauseMenu()
{
	// Resume Discord presence when leaving pause menu
	DiscordManager::Get().ResumeGamePresence();

	Minecraft *pMinecraft = Minecraft::GetInstance();
	if(pMinecraft != NULL && pMinecraft->localgameModes[m_iPad] != NULL )
	{
		TutorialMode *gameMode = (TutorialMode *)pMinecraft->localgameModes[m_iPad];

		// This just allows it to be shown
		gameMode->getTutorial()->showTutorialPopup(true);
	}

	m_parentLayer->showComponent(m_iPad,eUIComponent_Panorama,false);
	m_parentLayer->showComponent(m_iPad,eUIComponent_MenuBackground,false);
	m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,false);
}

wstring UIScene_PauseMenu::getMoviePath()
{
	if(app.GetLocalPlayerCount() > 1)
	{
		return L"PauseMenuSplit";
	}
	else
	{
		return L"PauseMenu";
	}
}

void UIScene_PauseMenu::tick()
{
	UIScene::tick();


	if(!m_bTrialTexturePack && m_savesDisabled != (app.GetGameHostOption(eGameHostOption_DisableSaving) != 0) && ProfileManager.GetPrimaryPad() == m_iPad )
	{
		// We show the save button if saves are disabled as this lets us show a prompt to enable them (via purchasing a texture pack)
		if( app.GetGameHostOption(eGameHostOption_DisableSaving) )
		{
			m_savesDisabled = true;
			m_buttons[BUTTON_PAUSE_SAVEGAME].setLabel( app.GetString(IDS_SAVE_GAME) );
		}
		else
		{
			m_savesDisabled = false;
			m_buttons[BUTTON_PAUSE_SAVEGAME].setLabel( app.GetString(IDS_DISABLE_AUTOSAVE) );
		}
	}

}

void UIScene_PauseMenu::updateTooltips()
{
	bool bUserisClientSide = ProfileManager.IsSignedInLive(m_iPad);
	bool bIsisPrimaryHost=g_NetworkManager.IsHost() && (ProfileManager.GetPrimaryPad()==m_iPad);

	bool bDisplayBanTip = !g_NetworkManager.IsLocalGame() && !bIsisPrimaryHost && !ProfileManager.IsGuest(m_iPad);

	int iY = -1;
	int iRB = -1;
	int iX = -1;

	if(ProfileManager.IsFullVersion())
	{
		if(StorageManager.GetSaveDisabled())
		{
			iX = bIsisPrimaryHost?IDS_TOOLTIPS_SELECTDEVICE:-1;
			iRB = bDisplayBanTip?IDS_TOOLTIPS_BANLEVEL:-1;
			if( CSocialManager::Instance()->IsTitleAllowedToPostImages() && CSocialManager::Instance()->AreAllUsersAllowedToPostImages() && bUserisClientSide )
			{
				iY = IDS_TOOLTIPS_SHARE;
			}		
		}
		else
		{
			iX = bIsisPrimaryHost?IDS_TOOLTIPS_CHANGEDEVICE:-1;
			iRB = bDisplayBanTip?IDS_TOOLTIPS_BANLEVEL:-1;
			if( CSocialManager::Instance()->IsTitleAllowedToPostImages() && CSocialManager::Instance()->AreAllUsersAllowedToPostImages() && bUserisClientSide)
			{
				iY = IDS_TOOLTIPS_SHARE;
			}	
		}
	}
	ui.SetTooltips( m_iPad, IDS_TOOLTIPS_SELECT,IDS_TOOLTIPS_BACK,iX,iY, -1,-1,-1,iRB);
}

void UIScene_PauseMenu::updateComponents()
{
	m_parentLayer->showComponent(m_iPad,eUIComponent_Panorama,false);
	m_parentLayer->showComponent(m_iPad,eUIComponent_MenuBackground,true);

	if( app.GetLocalPlayerCount() == 1 ) m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,true);
	else m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,false);
}

void UIScene_PauseMenu::handlePreReload()
{
	if(ProfileManager.GetPrimaryPad() == m_iPad)
	{
		// 4J-TomK - check for all possible labels being fed into BUTTON_PAUSE_SAVEGAME (Bug 163775)
		// this has to be done before button initialisation!
		wchar_t saveButtonLabels[2][256];
		swprintf( saveButtonLabels[0], 256, L"%ls", app.GetString( IDS_SAVE_GAME ));
		swprintf( saveButtonLabels[1], 256, L"%ls", app.GetString( IDS_DISABLE_AUTOSAVE ));
		m_buttons[BUTTON_PAUSE_SAVEGAME].setAllPossibleLabels(2,saveButtonLabels);
	}
}

void UIScene_PauseMenu::handleReload()
{
	updateTooltips();
	updateControlsVisibility();	

	if(ProfileManager.GetPrimaryPad() == m_iPad)
	{
		// We show the save button if saves are disabled as this lets us show a prompt to enable them (via purchasing a texture pack)
		if( app.GetGameHostOption(eGameHostOption_DisableSaving) || m_bTrialTexturePack )
		{
			m_savesDisabled = true;
			m_buttons[BUTTON_PAUSE_SAVEGAME].setLabel( app.GetString(IDS_SAVE_GAME) );
		}
		else
		{
			m_savesDisabled = false;
			m_buttons[BUTTON_PAUSE_SAVEGAME].setLabel( app.GetString(IDS_DISABLE_AUTOSAVE) );
		}
	}

	doHorizontalResizeCheck();
}

void UIScene_PauseMenu::updateControlsVisibility()
{
	if(ProfileManager.GetPrimaryPad()==m_iPad)
	{
		if( !g_NetworkManager.IsHost() )
		{
			removeControl( &m_buttons[BUTTON_PAUSE_SAVEGAME], false );
		}
	}
	else
	{
		removeControl( &m_buttons[BUTTON_PAUSE_SAVEGAME], false );
	}
}

void UIScene_PauseMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	if(m_bIgnoreInput)
	{
		return;
	}

	//app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d, down- %s, pressed- %s, released- %s\n", iPad, key, down?"TRUE":"FALSE", pressed?"TRUE":"FALSE", released?"TRUE":"FALSE");
	ui.AnimateKeyPress(iPad, key, repeat, pressed, released);

	bool bIsisPrimaryHost=g_NetworkManager.IsHost() && (ProfileManager.GetPrimaryPad()==iPad);
	bool bDisplayBanTip = !g_NetworkManager.IsLocalGame() && !bIsisPrimaryHost && !ProfileManager.IsGuest(iPad);

	switch(key)
	{
	case ACTION_MENU_GTC_RESUME:
	case ACTION_MENU_CANCEL:
		if(pressed)
		{
			//DurangoStatsDebugger::PrintStats(iPad);

			if( iPad == ProfileManager.GetPrimaryPad() && g_NetworkManager.IsLocalGame() )
			{
				app.SetXuiServerAction(ProfileManager.GetPrimaryPad(),eXuiServerAction_PauseServer,(void *)FALSE);
			}

			ui.PlayUISFX(eSFX_Back);
			navigateBack();
			if(!ProfileManager.IsFullVersion())
			{
				ui.ShowTrialTimer(true);
			}
		}
		break;
	case ACTION_MENU_OK:
	case ACTION_MENU_UP:
	case ACTION_MENU_DOWN:
		if(pressed)
		{
			sendInputToMovie(key, repeat, pressed, released);
		}
		break;

#if TO_BE_IMPLEMENTED
	case VK_PAD_X:
		// Change device
		if(bIsisPrimaryHost)
		{	
			// we need a function to deal with the return from this - if it changes, we need to update the pause menu and tooltips
			// Fix for #12531 - TCR 001: BAS Game Stability: When a player selects to change a storage 
			// device, and repeatedly backs out of the SD screen, disconnects from LIVE, and then selects a SD, the title crashes.
			m_bIgnoreInput=true;

			StorageManager.SetSaveDevice(&UIScene_PauseMenu::DeviceSelectReturned,this,true);
		}
		rfHandled = TRUE;
		break;
#endif

	case ACTION_MENU_Y:
		{
		}
		break;
	case ACTION_MENU_RIGHT_SCROLL:
		if( bDisplayBanTip )
		{
			UINT uiIDA[2];
			uiIDA[0]=IDS_CONFIRM_CANCEL;
			uiIDA[1]=IDS_CONFIRM_OK;
			ui.RequestMessageBox(IDS_ACTION_BAN_LEVEL_TITLE, IDS_ACTION_BAN_LEVEL_DESCRIPTION, uiIDA, 2, iPad,&UIScene_PauseMenu::BanGameDialogReturned,this, app.GetStringTable(), NULL, 0, false);

			//rfHandled = TRUE;
		}
		break;
	}
}

void UIScene_PauseMenu::handlePress(F64 controlId, F64 childId)
{
	if(m_bIgnoreInput) return;

	switch((int)controlId)
	{
	case BUTTON_PAUSE_RESUMEGAME:
		if( m_iPad == ProfileManager.GetPrimaryPad() && g_NetworkManager.IsLocalGame() )
		{
			app.SetXuiServerAction(ProfileManager.GetPrimaryPad(),eXuiServerAction_PauseServer,(void *)FALSE);
		}
		navigateBack();
		break;
	case BUTTON_PAUSE_ACHIEVEMENTS:
		{
			Minecraft *pMinecraft = Minecraft::GetInstance();
			if (pMinecraft != NULL)
				pMinecraft->setScreen(new AchievementScreen(pMinecraft->stats[0]));
		}
		XShowAchievementsUI( m_iPad );
		break;

	case BUTTON_PAUSE_HELPANDOPTIONS:
		ui.NavigateToScene(m_iPad,eUIScene_HelpAndOptionsMenu);	
		break;
	case BUTTON_PAUSE_SAVEGAME:
		PerformActionSaveGame();
		break;
	case BUTTON_PAUSE_EXITGAME:
		{
			Minecraft *pMinecraft = Minecraft::GetInstance();
			// Check if it's the trial version
			if(ProfileManager.IsFullVersion())
			{	
				UINT uiIDA[3];

				// is it the primary player exiting?
				if(m_iPad==ProfileManager.GetPrimaryPad())
				{
					int playTime = -1;
					if( pMinecraft->localplayers[m_iPad] != NULL )
					{
						playTime = (int)pMinecraft->localplayers[m_iPad]->getSessionTimer();
					}

					if(StorageManager.GetSaveDisabled())
					{
						uiIDA[0]=IDS_CONFIRM_CANCEL;
						uiIDA[1]=IDS_CONFIRM_OK;
						ui.RequestMessageBox(IDS_EXIT_GAME, IDS_CONFIRM_EXIT_GAME_PROGRESS_LOST, uiIDA, 2, m_iPad,&IUIScene_PauseMenu::ExitGameDialogReturned,this, app.GetStringTable(), NULL, 0, false);
					}
					else
					{
						if( g_NetworkManager.IsHost() )
						{	
							uiIDA[0]=IDS_CONFIRM_CANCEL;
							uiIDA[1]=IDS_EXIT_GAME_SAVE;
							uiIDA[2]=IDS_EXIT_GAME_NO_SAVE;

							if(g_NetworkManager.GetPlayerCount()>1)
							{
								ui.RequestMessageBox(IDS_EXIT_GAME, IDS_CONFIRM_EXIT_GAME_CONFIRM_DISCONNECT_SAVE, uiIDA, 3, m_iPad,&UIScene_PauseMenu::ExitGameSaveDialogReturned,this, app.GetStringTable(), NULL, 0, false);
							}
							else
							{
								ui.RequestMessageBox(IDS_EXIT_GAME, IDS_CONFIRM_EXIT_GAME, uiIDA, 3, m_iPad,&UIScene_PauseMenu::ExitGameSaveDialogReturned,this, app.GetStringTable(), NULL, 0, false);
							}
						}
						else
						{
							uiIDA[0]=IDS_CONFIRM_CANCEL;
							uiIDA[1]=IDS_CONFIRM_OK;

							ui.RequestMessageBox(IDS_EXIT_GAME, IDS_CONFIRM_EXIT_GAME, uiIDA, 2, m_iPad,&IUIScene_PauseMenu::ExitGameDialogReturned,this, app.GetStringTable(), NULL, 0, false);
						}
					}
				}
				else
				{
					int playTime = -1;
					if( pMinecraft->localplayers[m_iPad] != NULL )
					{
						playTime = (int)pMinecraft->localplayers[m_iPad]->getSessionTimer();
					}

					TelemetryManager->RecordLevelExit(m_iPad, eSen_LevelExitStatus_Exited);


					// just exit the player
					app.SetAction(m_iPad,eAppAction_ExitPlayer);
				}		
			}
			else
			{
				// is it the primary player exiting?
				if(m_iPad==ProfileManager.GetPrimaryPad())
				{
					int playTime = -1;
					if( pMinecraft->localplayers[m_iPad] != NULL )
					{
						playTime = (int)pMinecraft->localplayers[m_iPad]->getSessionTimer();
					}	

					// adjust the trial time played
					ui.ReduceTrialTimerValue();

					// exit the level
					UINT uiIDA[2];
					uiIDA[0]=IDS_CONFIRM_CANCEL;
					uiIDA[1]=IDS_CONFIRM_OK;
					ui.RequestMessageBox(IDS_EXIT_GAME, IDS_CONFIRM_EXIT_GAME_PROGRESS_LOST, uiIDA, 2, m_iPad,&IUIScene_PauseMenu::ExitGameDialogReturned, dynamic_cast<IUIScene_PauseMenu*>(this), app.GetStringTable(), NULL, 0, false);

				}
				else
				{
					int playTime = -1;
					if( pMinecraft->localplayers[m_iPad] != NULL )
					{
						playTime = (int)pMinecraft->localplayers[m_iPad]->getSessionTimer();
					}

					TelemetryManager->RecordLevelExit(m_iPad, eSen_LevelExitStatus_Exited);

					// just exit the player
					app.SetAction(m_iPad,eAppAction_ExitPlayer);
				}
			}
		}
		break;
	}
}

void UIScene_PauseMenu::PerformActionSaveGame()
{
	// is the player trying to save in the trial version?
	if(!ProfileManager.IsFullVersion())
	{

		// Unlock the full version?
		if(!ProfileManager.IsSignedInLive(m_iPad))
		{
		}
		else
		{
			UINT uiIDA[2];
			uiIDA[0]=IDS_CONFIRM_OK;
			uiIDA[1]=IDS_CONFIRM_CANCEL;
			ui.RequestMessageBox(IDS_UNLOCK_TITLE, IDS_UNLOCK_TOSAVE_TEXT, uiIDA, 2,m_iPad,&UIScene_PauseMenu::UnlockFullSaveReturned,this,app.GetStringTable(), NULL, 0, false);
		}

		return;
	}

	// 4J-PB - Is the player trying to save but they are using a trial texturepack ?
	if(!Minecraft::GetInstance()->skins->isUsingDefaultSkin())
	{
		TexturePack *tPack = Minecraft::GetInstance()->skins->getSelected();
		DLCTexturePack *pDLCTexPack=(DLCTexturePack *)tPack;

		m_pDLCPack=pDLCTexPack->getDLCInfoParentPack();//tPack->getDLCPack();

		if(!m_pDLCPack->hasPurchasedFile( DLCManager::e_DLCType_Texture, L"" ))
		{					
			// upsell
			UINT uiIDA[2];
			uiIDA[0]=IDS_CONFIRM_OK;
			uiIDA[1]=IDS_CONFIRM_CANCEL;

			// Give the player a warning about the trial version of the texture pack
			{
				ui.RequestMessageBox(IDS_WARNING_DLC_TRIALTEXTUREPACK_TITLE, IDS_WARNING_DLC_TRIALTEXTUREPACK_TEXT, uiIDA, 2, m_iPad,&UIScene_PauseMenu::WarningTrialTexturePackReturned,this,app.GetStringTable(), NULL, 0, false);
			}

			return;					
		}
		else
		{
			m_bTrialTexturePack = false;
		}
	}

	// does the save exist?
	bool bSaveExists;
	CStorageManager::ESaveGameState result=StorageManager.DoesSaveExist(&bSaveExists);

	{
		if(!m_savesDisabled)
		{
			UINT uiIDA[2];
			uiIDA[0]=IDS_CANCEL;
			uiIDA[1]=IDS_CONFIRM_OK;
			ui.RequestMessageBox(IDS_TITLE_DISABLE_AUTOSAVE, IDS_CONFIRM_DISABLE_AUTOSAVE, uiIDA, 2, m_iPad,&IUIScene_PauseMenu::DisableAutosaveDialogReturned,this, app.GetStringTable(), NULL, 0, false);
		}
		else
			// we need to ask if they are sure they want to overwrite the existing game
			if(bSaveExists)
			{
				UINT uiIDA[2];
				uiIDA[0]=IDS_CONFIRM_CANCEL;
				uiIDA[1]=IDS_CONFIRM_OK;
				ui.RequestMessageBox(IDS_TITLE_SAVE_GAME, IDS_CONFIRM_SAVE_GAME, uiIDA, 2, m_iPad,&IUIScene_PauseMenu::SaveGameDialogReturned,this, app.GetStringTable(), NULL, 0, false);
			}
			else
			{
				// flag a app action of save game
				app.SetAction(m_iPad,eAppAction_SaveGame);
			}
	}
}

void UIScene_PauseMenu::ShowScene(bool show)
{
	app.DebugPrintf("UIScene_PauseMenu::ShowScene is not implemented\n");
}

void UIScene_PauseMenu::HandleDLCInstalled()
{
	// mounted DLC may have changed
	if(app.StartInstallDLCProcess(m_iPad)==false)
	{
		// not doing a mount, so re-enable input
		//m_bIgnoreInput=false;
		app.DebugPrintf("UIScene_PauseMenu::HandleDLCInstalled - m_bIgnoreInput false\n");
	}
	else
	{
		// 4J-PB - Somehow, on th edisc build, we get in here, but don't call HandleDLCMountingComplete, so input locks up
		//m_bIgnoreInput=true;
		app.DebugPrintf("UIScene_PauseMenu::HandleDLCInstalled - m_bIgnoreInput true\n");
	}
	// this will send a CustomMessage_DLCMountingComplete when done
}


void UIScene_PauseMenu::HandleDLCMountingComplete()
{	
	// check if we should display the save option

	//m_bIgnoreInput=false;
	app.DebugPrintf("UIScene_PauseMenu::HandleDLCMountingComplete - m_bIgnoreInput false \n");

	// 	if(ProfileManager.IsFullVersion())
	// 	{
	// 		bool bIsisPrimaryHost=g_NetworkManager.IsHost() && (ProfileManager.GetPrimaryPad()==m_iPad);
	// 
	// 		if(bIsisPrimaryHost)
	// 		{
	// 			m_buttons[BUTTON_PAUSE_SAVEGAME].setEnable(true);
	// 		}
	// 	}
}

int UIScene_PauseMenu::UnlockFullSaveReturned(void *pParam,int iPad,CStorageManager::EMessageResult result)
{
	UIScene_PauseMenu* pClass = (UIScene_PauseMenu*)pParam;
	Minecraft *pMinecraft=Minecraft::GetInstance();

	if(result==CStorageManager::EMessage_ResultAccept)
	{
		if(ProfileManager.IsSignedInLive(pMinecraft->player->GetXboxPad()))
		{
			// 4J-PB - need to check this user can access the store
			{
				ProfileManager.DisplayFullVersionPurchase(false,pMinecraft->player->GetXboxPad(),eSen_UpsellID_Full_Version_Of_Game);
			}
		}
	}
	else
	{
		//SentientManager.RecordUpsellResponded(iPad, eSen_UpsellID_Full_Version_Of_Game, app.m_dwOfferID, eSen_UpsellOutcome_Declined);
	}

	return 0;
}

int UIScene_PauseMenu::SaveGame_SignInReturned(void *pParam,bool bContinue, int iPad)
{
	UIScene_PauseMenu* pClass = (UIScene_PauseMenu*)pParam;

	if(bContinue==true)
	{
		pClass->PerformActionSaveGame();
	}

	return 0;
}

int UIScene_PauseMenu::BanGameDialogReturned(void *pParam,int iPad,CStorageManager::EMessageResult result)
{
	// results switched for this dialog
	if(result==CStorageManager::EMessage_ResultDecline) 
	{
		app.SetAction(iPad,eAppAction_BanLevel);
	}
	return 0;
}

int UIScene_PauseMenu::WarningTrialTexturePackReturned(void *pParam,int iPad,CStorageManager::EMessageResult result)
{
	return 0;
}

int UIScene_PauseMenu::BuyTexturePack_SignInReturned(void *pParam,bool bContinue, int iPad)
{
	return 0;
}

int UIScene_PauseMenu::ViewInvites_SignInReturned(void *pParam,bool bContinue, int iPad)
{
	return 0;
}


int UIScene_PauseMenu::ExitGameSaveDialogReturned(void *pParam,int iPad,CStorageManager::EMessageResult result)
{
	UIScene_PauseMenu *pClass = (UIScene_PauseMenu *)pParam;
	// Exit with or without saving
	// Decline means save in this dialog
	if(result==CStorageManager::EMessage_ResultDecline || result==CStorageManager::EMessage_ResultThirdOption) 
	{
		if( result==CStorageManager::EMessage_ResultDecline ) // Save
		{
			// 4J-PB - Is the player trying to save but they are using a trial texturepack ?
			if(!Minecraft::GetInstance()->skins->isUsingDefaultSkin())
			{
				TexturePack *tPack = Minecraft::GetInstance()->skins->getSelected();
				DLCTexturePack *pDLCTexPack=(DLCTexturePack *)tPack;

				DLCPack *pDLCPack=pDLCTexPack->getDLCInfoParentPack();//tPack->getDLCPack();
				if(!pDLCPack->hasPurchasedFile( DLCManager::e_DLCType_Texture, L"" ))
				{					

					UINT uiIDA[2];
					uiIDA[0]=IDS_CONFIRM_OK;
					uiIDA[1]=IDS_CONFIRM_CANCEL;

					// Give the player a warning about the trial version of the texture pack
					ui.RequestMessageBox(IDS_WARNING_DLC_TRIALTEXTUREPACK_TITLE, IDS_WARNING_DLC_TRIALTEXTUREPACK_TEXT, uiIDA, 2, ProfileManager.GetPrimaryPad() ,&UIScene_PauseMenu::WarningTrialTexturePackReturned, dynamic_cast<IUIScene_PauseMenu*>(pClass),app.GetStringTable(), NULL, 0, false);

					return S_OK;					
				}
			}

			// does the save exist?
			bool bSaveExists;
			StorageManager.DoesSaveExist(&bSaveExists);
			// 4J-PB - we check if the save exists inside the libs
			// we need to ask if they are sure they want to overwrite the existing game
			if(bSaveExists)
			{
				UINT uiIDA[2];
				uiIDA[0]=IDS_CONFIRM_CANCEL;
				uiIDA[1]=IDS_CONFIRM_OK;
				ui.RequestMessageBox(IDS_TITLE_SAVE_GAME, IDS_CONFIRM_SAVE_GAME, uiIDA, 2, ProfileManager.GetPrimaryPad(),&IUIScene_PauseMenu::ExitGameAndSaveReturned, dynamic_cast<IUIScene_PauseMenu*>(pClass), app.GetStringTable(), NULL, 0, false);
				return 0;
			}
			else
			{
				StorageManager.SetSaveDisabled(false);
				MinecraftServer::getInstance()->setSaveOnExit( true );
			}
		}
		else
		{
			// been a few requests for a confirm on exit without saving
			UINT uiIDA[2];
			uiIDA[0]=IDS_CONFIRM_CANCEL;
			uiIDA[1]=IDS_CONFIRM_OK;
			ui.RequestMessageBox(IDS_TITLE_DECLINE_SAVE_GAME, IDS_CONFIRM_DECLINE_SAVE_GAME, uiIDA, 2, ProfileManager.GetPrimaryPad(),&IUIScene_PauseMenu::ExitGameDeclineSaveReturned, dynamic_cast<IUIScene_PauseMenu*>(pClass), app.GetStringTable(), NULL, 0, false);
			return 0;
		}

		app.SetAction(iPad,eAppAction_ExitWorld);
	}
	return 0;
}


void UIScene_PauseMenu::SetIgnoreInput(bool ignoreInput)
{
	m_bIgnoreInput = ignoreInput;
}

void UIScene_PauseMenu::HandleDLCLicenseChange()
{	
}
