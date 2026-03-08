#include "stdafx.h"
#include "..\..\..\World\Math\Mth.h"
#include "..\..\..\World\Math\StringHelpers.h"
#include "..\..\..\World\Math\Random.h"
#include "../../Game/User.h"
#include "..\..\MinecraftServer.h"
#include "UI.h"
#include "UIScene_MainMenu.h"
#include "../../Screens/AchievementScreen.h"
// Only need the button defines from XUI_MainMenu, not the XUI framework types
#define BUTTON_PLAYGAME			0
#define BUTTON_QUITGAME			1
#define BUTTON_ACHIEVEMENTS		2
#define BUTTON_HELPANDOPTIONS	3
#define BUTTON_UNLOCKFULLGAME	4
#define BUTTONS_MAX				BUTTON_UNLOCKFULLGAME + 1
#define MAIN_MENU_MAX_TEXT_SCALE 1.5f
// Stub for CScene_Main callbacks referenced in shared code (never actually called on PC)
struct CScene_Main {
	static int DeviceSelectReturned(void *, bool) { return 0; }
	static int CreateLoad_OfflineProfileReturned(void *, bool, int) { return 0; }
	static int AchievementsDeviceSelectReturned(void *, bool) { return 0; }
};

Random *UIScene_MainMenu::random = new Random();

UIScene_MainMenu::UIScene_MainMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	//m_ePatchCheckState=ePatchCheck_Idle;
	m_bRunGameChosen=false;
	m_bErrorDialogRunning=false;
	m_bWaitingForDLCInfo=false;


	// Setup all the Iggy references we need for this scene
	initialiseMovie();

	parentLayer->addComponent(iPad,eUIComponent_Panorama);
	parentLayer->addComponent(iPad,eUIComponent_Logo);

	m_eAction=eAction_None;
	m_bIgnorePress=false;


	m_buttons[(int)eControl_PlayGame].init(app.GetString(IDS_PLAY_GAME),eControl_PlayGame);

	if(!ProfileManager.IsFullVersion()) m_buttons[(int)eControl_PlayGame].setLabel(app.GetString(IDS_PLAY_TRIAL_GAME));
	app.SetReachedMainMenu();

	m_buttons[(int)eControl_QuitGame].init(app.GetString(IDS_EXIT_GAME),eControl_QuitGame);
	m_buttons[(int)eControl_Achievements].init(app.GetString(IDS_ACHIEVEMENTS),eControl_Achievements);
	m_buttons[(int)eControl_HelpAndOptions].init(app.GetString(IDS_HELP_AND_OPTIONS),eControl_HelpAndOptions);
	if(ProfileManager.IsFullVersion())
	{
		m_bTrialVersion=false;
		m_buttons[(int)eControl_UnlockOrDLC].init(app.GetString(IDS_DOWNLOADABLECONTENT),eControl_UnlockOrDLC);
	}
	else
	{
		m_bTrialVersion=true;
		m_buttons[(int)eControl_UnlockOrDLC].init(app.GetString(IDS_UNLOCK_FULL_GAME),eControl_UnlockOrDLC);
	}


	doHorizontalResizeCheck();

	m_splash = L"";

	wstring filename = L"splashes.txt";
	if( app.hasArchiveFile(filename) )
	{
		byteArray splashesArray = app.getArchiveFile(filename);
		ByteArrayInputStream bais(splashesArray);
		InputStreamReader isr( &bais );
		BufferedReader br( &isr );

		wstring line = L"";
		while ( !(line = br.readLine()).empty() )
		{
			line = trimString( line );
			if (line.length() > 0)
			{
				m_splashes.push_back(line);
			}
		}

		br.close();
	}

	m_bIgnorePress=false;
	m_bLoadTrialOnNetworkManagerReady = false;

	// 4J Stu - Clear out any loaded game rules
	app.setLevelGenerationOptions(NULL);

	// 4J Stu - Reset the leaving game flag so that we correctly handle signouts while in the menus
	g_NetworkManager.ResetLeavingGame();

#if TO_BE_IMPLEMENTED
	// Fix for #45154 - Frontend: DLC: Content can only be downloaded from the frontend if you have not joined/exited multiplayer
	XBackgroundDownloadSetMode(XBACKGROUND_DOWNLOAD_MODE_ALWAYS_ALLOW);
#endif
}

UIScene_MainMenu::~UIScene_MainMenu()
{
	m_parentLayer->removeComponent(eUIComponent_Panorama);
	m_parentLayer->removeComponent(eUIComponent_Logo);
}

void UIScene_MainMenu::updateTooltips()
{
	int iX = -1;
	int iA = -1;
	if(!m_bIgnorePress)
	{
		iA = IDS_TOOLTIPS_SELECT;

		iX = IDS_TOOLTIPS_CHOOSE_USER;
	}
	ui.SetTooltips( DEFAULT_XUI_MENU_USER, iA, -1, iX);
}

void UIScene_MainMenu::updateComponents()
{
	m_parentLayer->showComponent(m_iPad,eUIComponent_Panorama,true);
	m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,true);
}

void UIScene_MainMenu::handleGainFocus(bool navBack)
{
	UIScene::handleGainFocus(navBack);
	ui.ShowPlayerDisplayname(false);
	m_bIgnorePress=false;
	
	// 4J-JEV: This needs to come before SetLockedProfile(-1) as it wipes the XbLive contexts.
	if (!navBack)
	{
		for (int iPad = 0; iPad < MAX_LOCAL_PLAYERS; iPad++)
		{
			// For returning to menus after exiting a game.
			if (ProfileManager.IsSignedIn(iPad) )
			{
				ProfileManager.SetCurrentGameActivity(iPad, CONTEXT_PRESENCE_MENUS, false);
			}
		}
	}
	ProfileManager.SetLockedProfile(-1);

	m_bIgnorePress = false;
	updateTooltips();

	ProfileManager.ClearGameUsers();
		
	if(navBack && ProfileManager.IsFullVersion())
	{
		// Replace the Unlock Full Game with Downloadable Content
		m_buttons[(int)eControl_UnlockOrDLC].setLabel(app.GetString(IDS_DOWNLOADABLECONTENT));
	}

#if TO_BE_IMPLEMENTED
	// Fix for #45154 - Frontend: DLC: Content can only be downloaded from the frontend if you have not joined/exited multiplayer
	XBackgroundDownloadSetMode(XBACKGROUND_DOWNLOAD_MODE_ALWAYS_ALLOW);
	m_Timer.SetShow(FALSE);
#endif
	m_controlTimer.setVisible( false );

	// 4J-PB - remove the "hobo humping" message legal say we can't have, and the 1080p one for Vita
	int splashIndex = eSplashRandomStart + 1 + random->nextInt( (int)m_splashes.size() - (eSplashRandomStart + 1) );

	// Override splash text on certain dates
	SYSTEMTIME LocalSysTime;
	GetLocalTime( &LocalSysTime );
	if (LocalSysTime.wMonth == 11 && LocalSysTime.wDay == 9)
	{
		splashIndex = eSplashHappyBirthdayEx;
	}
	else if (LocalSysTime.wMonth == 6 && LocalSysTime.wDay == 1)
	{
		splashIndex = eSplashHappyBirthdayNotch;
	}
	else if (LocalSysTime.wMonth == 12 && LocalSysTime.wDay == 24) // the Java game shows this on Christmas Eve, so we will too
	{
		splashIndex = eSplashMerryXmas;
	}
	else if (LocalSysTime.wMonth == 1 && LocalSysTime.wDay == 1)
	{
		splashIndex = eSplashHappyNewYear;
	}
	//splashIndex = 47; // Very short string
	//splashIndex = 194; // Very long string
	//splashIndex = 295; // Coloured
	//splashIndex = 296; // Noise
	m_splash = m_splashes.at( splashIndex );
}

wstring UIScene_MainMenu::getMoviePath()
{
	return L"MainMenu";
}

void UIScene_MainMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	//app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d, down- %s, pressed- %s, released- %s\n", iPad, key, down?"TRUE":"FALSE", pressed?"TRUE":"FALSE", released?"TRUE":"FALSE");
	
	if(m_bIgnorePress) return;


	ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

	switch(key)
	{
	case ACTION_MENU_OK:
		if(pressed)
		{
			ProfileManager.SetPrimaryPad(iPad);
			ProfileManager.SetLockedProfile(-1);
			sendInputToMovie(key, repeat, pressed, released);
		}
		break;
	case ACTION_MENU_X:
		if(pressed)
		{
			m_bIgnorePress = true;
			ProfileManager.RequestSignInUI(false, false, false, false, false, ChooseUser_SignInReturned, this, iPad);
		}
		break;

	case ACTION_MENU_UP:
	case ACTION_MENU_DOWN:
		sendInputToMovie(key, repeat, pressed, released);
		break;
	}
}

void UIScene_MainMenu::handlePress(F64 controlId, F64 childId)
{
	int primaryPad = ProfileManager.GetPrimaryPad();
	
	int (*signInReturnedFunc) (LPVOID,const bool, const int iPad) = NULL;

	switch((int)controlId)
	{
	case eControl_PlayGame:
		m_eAction=eAction_RunGame;
		//CD - Added for audio
		ui.PlayUISFX(eSFX_Press);

		signInReturnedFunc = &UIScene_MainMenu::CreateLoad_SignInReturned;
		break;
	case eControl_QuitGame:
		ui.PlayUISFX(eSFX_Press);
		{
			UINT uiIDA[2];
			uiIDA[0]=IDS_CONFIRM_CANCEL;
			uiIDA[1]=IDS_CONFIRM_OK;
			ui.RequestMessageBox(IDS_EXIT_GAME, IDS_EXIT_GAME, uiIDA, 2, primaryPad, &UIScene_MainMenu::ExitGameReturned, this, app.GetStringTable());
		}
		return;
	case eControl_Achievements:
		//CD - Added for audio
		ui.PlayUISFX(eSFX_Press);

		m_eAction=eAction_RunAchievements;
		signInReturnedFunc = &UIScene_MainMenu::Achievements_SignInReturned;
		break;
	case eControl_HelpAndOptions:
		//CD - Added for audio
		ui.PlayUISFX(eSFX_Press);

		m_eAction=eAction_RunHelpAndOptions;
		signInReturnedFunc = &UIScene_MainMenu::HelpAndOptions_SignInReturned;
		break;
	case eControl_UnlockOrDLC:
		//CD - Added for audio
		ui.PlayUISFX(eSFX_Press);

		m_eAction=eAction_RunUnlockOrDLC;
		signInReturnedFunc = &UIScene_MainMenu::UnlockFullGame_SignInReturned;
		break;

	default:	__debugbreak();
	}
	
	bool confirmUser = false;

	// Note: if no sign in returned func, assume this isn't required
	if (signInReturnedFunc != NULL)
	{
		if(ProfileManager.IsSignedIn(primaryPad))
		{
			if (confirmUser)
			{
				ProfileManager.RequestSignInUI(false, false, true, false, true, signInReturnedFunc, this, primaryPad);
			}
			else
			{
				RunAction(primaryPad);
			}
		}
		else
		{
			// Ask user to sign in
			UINT uiIDA[2];
			uiIDA[0]=IDS_CONFIRM_OK;
			uiIDA[1]=IDS_CONFIRM_CANCEL;
			ui.RequestMessageBox(IDS_MUST_SIGN_IN_TITLE, IDS_MUST_SIGN_IN_TEXT, uiIDA, 2, primaryPad, &UIScene_MainMenu::MustSignInReturned, this, app.GetStringTable());
		}
	}
}

// Run current action
void UIScene_MainMenu::RunAction(int iPad)
{
	switch(m_eAction)
	{
	case eAction_RunGame:
		RunPlayGame(iPad);
		break;
	case eAction_RunAchievements:
		RunAchievements(iPad);
		break;
	case eAction_RunHelpAndOptions:
		RunHelpAndOptions(iPad);
		break;
	case eAction_RunUnlockOrDLC:
		RunUnlockOrDLC(iPad);
		break;
	}
}

void UIScene_MainMenu::customDraw(IggyCustomDrawCallbackRegion *region)
{
	if(wcscmp((wchar_t *)region->name,L"Splash")==0)
	{
		PIXBeginNamedEvent(0,"Custom draw splash");
		customDrawSplash(region);
		PIXEndNamedEvent();
	}
}

void UIScene_MainMenu::customDrawSplash(IggyCustomDrawCallbackRegion *region)
{
	Minecraft *pMinecraft = Minecraft::GetInstance();

	// 4J Stu - Move this to the ctor when the main menu is not the first scene we navigate to
	ScreenSizeCalculator ssc(pMinecraft->options, pMinecraft->width_phys, pMinecraft->height_phys);
	m_fScreenWidth=(float)pMinecraft->width_phys;
	m_fRawWidth=(float)ssc.rawWidth;
	m_fScreenHeight=(float)pMinecraft->height_phys;
	m_fRawHeight=(float)ssc.rawHeight;


	// Setup GDraw, normal game render states and matrices
	CustomDrawData *customDrawRegion = ui.setupCustomDraw(this,region);
	delete customDrawRegion;


	Font *font = pMinecraft->font;

	// build and render with the game call
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	glPushMatrix();

	float width = region->x1 - region->x0;
	float height = region->y1 - region->y0;
	float xo = width/2;
	float yo = height;

	glTranslatef(xo, yo, 0);

	glRotatef(-17, 0, 0, 1);
	float sss = 1.8f - Mth::abs(Mth::sin(System::currentTimeMillis() % 1000 / 1000.0f * PI * 2) * 0.1f);
	sss*=(m_fScreenWidth/m_fRawWidth);

	sss = sss * 100 / (font->width(m_splash) + 8 * 4);
	glScalef(sss, sss, sss);
	//drawCenteredString(font, splash, 0, -8, 0xffff00);
	font->drawShadow(m_splash, 0 - (font->width(m_splash)) / 2, -8, 0xffff00);
	glPopMatrix();

	glDisable(GL_RESCALE_NORMAL);

	glEnable(GL_DEPTH_TEST);


	// Finish GDraw and anything else that needs to be finalised
	ui.endCustomDraw(region);	
}

int UIScene_MainMenu::MustSignInReturned(void *pParam, int iPad, CStorageManager::EMessageResult result)
{
	UIScene_MainMenu* pClass = (UIScene_MainMenu*)pParam;

	if(result==CStorageManager::EMessage_ResultAccept) 
	{
		// we need to specify local game here to display local and LIVE profiles in the list
		switch(pClass->m_eAction)
		{
		case eAction_RunGame:			ProfileManager.RequestSignInUI(false,  true, false, false, true, &UIScene_MainMenu::CreateLoad_SignInReturned,		pClass,	iPad );	break;
		case eAction_RunHelpAndOptions:	ProfileManager.RequestSignInUI(false, false,  true, false, true, &UIScene_MainMenu::HelpAndOptions_SignInReturned,	pClass,	iPad );	break;
		case eAction_RunAchievements:	ProfileManager.RequestSignInUI(false, false,  true, false, true, &UIScene_MainMenu::Achievements_SignInReturned,	pClass,	iPad );	break;
		case eAction_RunUnlockOrDLC:	ProfileManager.RequestSignInUI(false, false,  true, false, true, &UIScene_MainMenu::UnlockFullGame_SignInReturned,	pClass,	iPad );	break;
		}
	}
	else
	{
		pClass->m_bIgnorePress=false;
		// unlock the profile
		ProfileManager.SetLockedProfile(-1);
		for(int i=0;i<XUSER_MAX_COUNT;i++)
		{
			// if the user is valid, we should set the presence
			if(ProfileManager.IsSignedIn(i))
			{
				ProfileManager.SetCurrentGameActivity(i, CONTEXT_PRESENCE_MENUS, false);
			}
		}
	}	

	return 0;
}

int UIScene_MainMenu::HelpAndOptions_SignInReturned(void *pParam,bool bContinue,int iPad)
{
	UIScene_MainMenu *pClass = (UIScene_MainMenu *)pParam;

	if(bContinue)
	{
		// 4J-JEV: Don't we only need to update rich-presence if the sign-in status changes.
		ProfileManager.SetCurrentGameActivity(iPad, CONTEXT_PRESENCE_MENUS, false);

#if TO_BE_IMPLEMENTED
 		if(app.GetTMSDLCInfoRead())
#endif
 		{
			ProfileManager.SetLockedProfile(ProfileManager.GetPrimaryPad());
			ui.ShowPlayerDisplayname(true);
 			ui.NavigateToScene(iPad,eUIScene_HelpAndOptionsMenu);
 		}
#if TO_BE_IMPLEMENTED
 		else
 		{
 			// Changing to async TMS calls
 			app.SetTMSAction(iPad,eTMSAction_TMSPP_RetrieveFiles_HelpAndOptions);
 
 			// block all input
 			pClass->m_bIgnorePress=true;
 			// We want to hide everything in this scene and display a timer until we get a completion for the TMS files
 			for(int i=0;i<BUTTONS_MAX;i++)
 			{
 				pClass->m_Buttons[i].SetShow(FALSE);
 			}
 
 			pClass->updateTooltips();
 
 			pClass->m_Timer.SetShow(TRUE);
 		}
#endif
	}
	else
	{
		pClass->m_bIgnorePress=false;
		// unlock the profile
		ProfileManager.SetLockedProfile(-1);
		for(int i=0;i<XUSER_MAX_COUNT;i++)
		{
			// if the user is valid, we should set the presence
			if(ProfileManager.IsSignedIn(i))
			{
				ProfileManager.SetCurrentGameActivity(i,CONTEXT_PRESENCE_MENUS,false);
			}
		}
	}	

	return 0;
}

int UIScene_MainMenu::ChooseUser_SignInReturned(void *pParam, bool bContinue, int iPad)
{
	UIScene_MainMenu* pClass = (UIScene_MainMenu*)pParam;
	pClass->m_bIgnorePress = false;

	return 0;
}

int UIScene_MainMenu::CreateLoad_SignInReturned(void *pParam, bool bContinue, int iPad)
{
	UIScene_MainMenu* pClass = (UIScene_MainMenu*)pParam;
	
	if(bContinue)
	{
		// 4J-JEV: We only need to update rich-presence if the sign-in status changes.
		ProfileManager.SetCurrentGameActivity(iPad, CONTEXT_PRESENCE_MENUS, false);

		UINT uiIDA[1] = { IDS_OK };

		if(ProfileManager.IsGuest(ProfileManager.GetPrimaryPad()))
		{
			pClass->m_bIgnorePress=false;
			ui.RequestMessageBox(IDS_PRO_GUESTPROFILE_TITLE, IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
		}
		else
		{
			ProfileManager.SetLockedProfile(ProfileManager.GetPrimaryPad());


			// change the minecraft player name
			Minecraft::GetInstance()->user->name = convStringToWstring( ProfileManager.GetGamertag(ProfileManager.GetPrimaryPad()));

			if(ProfileManager.IsFullVersion())
			{
				bool bSignedInLive = ProfileManager.IsSignedInLive(iPad);

				// Check if we're signed in to LIVE
				if(bSignedInLive)
				{
					// 4J-PB - Need to check for installed DLC
					if(!app.DLCInstallProcessCompleted()) app.StartInstallDLCProcess(iPad);

					if(ProfileManager.IsGuest(iPad))
					{
						pClass->m_bIgnorePress=false;
						ui.RequestMessageBox(IDS_PRO_GUESTPROFILE_TITLE, IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
					}
					else
					{
						// 4J Stu - Check if TMS files are already loaded
						if(app.GetTMSAction(iPad) == eTMSAction_Idle)
						{
							ui.NavigateToScene(ProfileManager.GetPrimaryPad(), eUIScene_LoadOrJoinMenu);
						}
						else
						{
							app.SetTMSAction(iPad,eTMSAction_TMSPP_RetrieveFiles_RunPlayGame);

							// block all input
							pClass->m_bIgnorePress=true;
							// We want to hide everything in this scene and display a timer until we get a completion for the TMS files
							// 					for(int i=0;i<eControl_Count;i++)
							// 					{
							// 						m_buttons[i].set(false);
							// 					}

							pClass->updateTooltips();

							pClass->m_controlTimer.setVisible( true );
						}
#if TO_BE_IMPLEMENTED
						// check if all the TMS files are loaded
						if(app.GetTMSDLCInfoRead() && app.GetTMSXUIDsFileRead() && app.GetBanListRead(iPad))
						{
							if(StorageManager.SetSaveDevice(&UIScene_MainMenu::DeviceSelectReturned,pClass)==true)
							{
								// save device already selected

								// ensure we've applied this player's settings
								app.ApplyGameSettingsChanged(ProfileManager.GetPrimaryPad());
								// check for DLC
								// start timer to track DLC check finished
								pClass->m_Timer.SetShow(TRUE);
								XuiSetTimer(pClass->m_hObj,DLC_INSTALLED_TIMER_ID,DLC_INSTALLED_TIMER_TIME);
								//app.NavigateToScene(ProfileManager.GetPrimaryPad(),eUIScene_MultiGameJoinLoad);
							}
						}
						else
						{
							// Changing to async TMS calls
							app.SetTMSAction(iPad,eTMSAction_TMSPP_RetrieveFiles_RunPlayGame);

							// block all input
							pClass->m_bIgnorePress=true;
							// We want to hide everything in this scene and display a timer until we get a completion for the TMS files
							for(int i=0;i<BUTTONS_MAX;i++)
							{
								pClass->m_Buttons[i].SetShow(FALSE);
							}

							updateTooltips();

							pClass->m_Timer.SetShow(TRUE);
						}
#else
						Minecraft *pMinecraft=Minecraft::GetInstance();
						pMinecraft->user->name = convStringToWstring( ProfileManager.GetGamertag(ProfileManager.GetPrimaryPad()));

						// ensure we've applied this player's settings
						app.ApplyGameSettingsChanged(iPad);

						ui.ShowPlayerDisplayname(true);
						ui.NavigateToScene(ProfileManager.GetPrimaryPad(), eUIScene_LoadOrJoinMenu);
#endif
					}
				}
				else
				{
#if TO_BE_IMPLEMENTED
					// offline
					ProfileManager.DisplayOfflineProfile(&CScene_Main::CreateLoad_OfflineProfileReturned,pClass, ProfileManager.GetPrimaryPad() );
#else
					app.DebugPrintf("Offline Profile returned not implemented\n");		
					ui.ShowPlayerDisplayname(true);
					ui.NavigateToScene(ProfileManager.GetPrimaryPad(), eUIScene_LoadOrJoinMenu);
#endif
				}
			}
			else
			{
				// 4J-PB - if this is the trial game, we can't have any networking
				// Can't apply the player's settings here - they haven't come back from the QuerySignInStatud call above yet.
				// Need to let them action in the main loop when they come in
				// ensure we've applied this player's settings
				//app.ApplyGameSettingsChanged(iPad);


				{
					// go straight in to the trial level
					LoadTrial();
				}
			}
		}
	}
	else
	{
		pClass->m_bIgnorePress=false;

		// unlock the profile
		ProfileManager.SetLockedProfile(-1);
		for(int i=0;i<XUSER_MAX_COUNT;i++)
		{
			// if the user is valid, we should set the presence
			if(ProfileManager.IsSignedIn(i))
			{
				ProfileManager.SetCurrentGameActivity(i,CONTEXT_PRESENCE_MENUS,false);
			}
		}

	}	
	return 0;
}

int UIScene_MainMenu::Achievements_SignInReturned(void *pParam,bool bContinue,int iPad)
{
	UIScene_MainMenu *pClass = (UIScene_MainMenu *)pParam;

	if (bContinue)
	{
		pClass->m_bIgnorePress=false;
		// 4J-JEV: We only need to update rich-presence if the sign-in status changes.
		ProfileManager.SetCurrentGameActivity(iPad, CONTEXT_PRESENCE_MENUS, false);

		XShowAchievementsUI( ProfileManager.GetPrimaryPad() );
	}
	else
	{
		pClass->m_bIgnorePress=false;
		// unlock the profile
		ProfileManager.SetLockedProfile(-1);
		for(int i=0;i<XUSER_MAX_COUNT;i++)
		{
			// if the user is valid, we should set the presence
			if(ProfileManager.IsSignedIn(i))
			{
				ProfileManager.SetCurrentGameActivity(i,CONTEXT_PRESENCE_MENUS,false);
			}
		}

	}	
	return 0;
}

int UIScene_MainMenu::UnlockFullGame_SignInReturned(void *pParam,bool bContinue,int iPad)
{
	UIScene_MainMenu* pClass = (UIScene_MainMenu*)pParam;

	if (bContinue)
	{
		// 4J-JEV: We only need to update rich-presence if the sign-in status changes.
		ProfileManager.SetCurrentGameActivity(iPad, CONTEXT_PRESENCE_MENUS, false);

		pClass->RunUnlockOrDLC(iPad);
	}
	else
	{
		pClass->m_bIgnorePress=false;
		// unlock the profile
		ProfileManager.SetLockedProfile(-1);
		for(int i=0;i<XUSER_MAX_COUNT;i++)
		{
			// if the user is valid, we should set the presence
			if(ProfileManager.IsSignedIn(i))
			{
				ProfileManager.SetCurrentGameActivity(i,CONTEXT_PRESENCE_MENUS,false);
			}
		}

	}	

	return 0;
}

int UIScene_MainMenu::ExitGameReturned(void *pParam,int iPad,CStorageManager::EMessageResult result)
{
	//UIScene_MainMenu* pClass = (UIScene_MainMenu*)pParam;

	// buttons reversed on this
	if(result==CStorageManager::EMessage_ResultDecline) 
	{
		//XLaunchNewImage(XLAUNCH_KEYWORD_DASH_ARCADE, 0);
		app.ExitGame();
	}

	return 0;
}


void UIScene_MainMenu::RunPlayGame(int iPad)
{
	Minecraft *pMinecraft=Minecraft::GetInstance();

	// clear the remembered signed in users so their profiles get read again
	app.ClearSignInChangeUsersMask();

	app.ReleaseSaveThumbnail();

	// PC: skip all sign-in/TMS/ban list checks - always fully connected
	ProfileManager.SetLockedProfile(iPad);
	ProfileManager.QuerySigninStatus();
	if(!app.DLCInstallProcessCompleted()) app.StartInstallDLCProcess(iPad);
	pMinecraft->user->name = convStringToWstring( ProfileManager.GetGamertag(ProfileManager.GetPrimaryPad()));
	app.ApplyGameSettingsChanged(iPad);
	ui.NavigateToScene(ProfileManager.GetPrimaryPad(), eUIScene_LoadOrJoinMenu);
}

void UIScene_MainMenu::RunUnlockOrDLC(int iPad)
{
	UINT uiIDA[1];
	uiIDA[0]=IDS_OK;

	// Check if this means downloadable content
	if(ProfileManager.IsFullVersion())
	{
		// downloadable content
		if(ProfileManager.IsSignedInLive(iPad))
		{
			if(ProfileManager.IsGuest(iPad))
			{
				m_bIgnorePress=false;
				ui.RequestMessageBox(IDS_PRO_GUESTPROFILE_TITLE, IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
			}
			else
			{

				// If the player was signed in before selecting play, we'll not have read the profile yet, so query the sign-in status to get this to happen
				ProfileManager.QuerySigninStatus();

				if(app.GetTMSDLCInfoRead())
				{
					bool bContentRestricted=false;
					if(bContentRestricted)
					{
						m_bIgnorePress=false;
						// you can't see the store
						UINT uiIDA[1];
						uiIDA[0]=IDS_CONFIRM_OK;
						ui.RequestMessageBox(IDS_ONLINE_SERVICE_TITLE, IDS_CONTENT_RESTRICTION, uiIDA, 1, ProfileManager.GetPrimaryPad(),NULL,this, app.GetStringTable());
					}
					else
					{
						ProfileManager.SetLockedProfile(iPad);
						ui.ShowPlayerDisplayname(true);
						ui.NavigateToScene(ProfileManager.GetPrimaryPad(),eUIScene_DLCMainMenu);
					}
				}
				else
				{
					// Changing to async TMS calls
					app.SetTMSAction(iPad,eTMSAction_TMSPP_RetrieveFiles_DLCMain);

					// block all input
					m_bIgnorePress=true;
					// We want to hide everything in this scene and display a timer until we get a completion for the TMS files
// 					for(int i=0;i<BUTTONS_MAX;i++)
// 					{
// 						m_Buttons[i].SetShow(FALSE);
// 					}

					updateTooltips();

					m_controlTimer.setVisible( true );
					m_bWaitingForDLCInfo=true;
				}

				// read the DLC info from TMS
				/*app.ReadDLCFileFromTMS(iPad);*/

				// We want to navigate to the DLC scene, but block input until we get the DLC file in from TMS
				// Don't navigate - we might have an uplink disconnect
				//app.NavigateToScene(ProfileManager.GetPrimaryPad(),eUIScene_DLCMainMenu);

			}
		}
		else
		{
			UINT uiIDA[1];
			uiIDA[0]=IDS_OK;
			ui.RequestMessageBox(IDS_PRO_NOTONLINE_TITLE, IDS_PRO_XBOXLIVE_NOTIFICATION, uiIDA, 1);
		}
	}
	else
	{
		// guests can't buy the game
		if(ProfileManager.IsGuest(iPad))
		{
			m_bIgnorePress=false;
			ui.RequestMessageBox(IDS_UNLOCK_TITLE, IDS_UNLOCK_GUEST_TEXT, uiIDA, 1,iPad);
		}
		else if(!ProfileManager.IsSignedInLive(iPad))
		{
			UINT uiIDA[1];
			uiIDA[0]=IDS_OK;
			ui.RequestMessageBox(IDS_PRO_NOTONLINE_TITLE, IDS_PRO_XBOXLIVE_NOTIFICATION, uiIDA, 1);

		}
		else
		{
			// If the player was signed in before selecting play, we'll not have read the profile yet, so query the sign-in status to get this to happen
			ProfileManager.QuerySigninStatus();

			// check that the commerce is in the right state to be able to display the full version purchase - if the user is fast with the trial version, it can still be retrieving the product list
			TelemetryManager->RecordUpsellPresented(iPad, eSen_UpsellID_Full_Version_Of_Game, app.m_dwOfferID);
			ProfileManager.DisplayFullVersionPurchase(false,iPad,eSen_UpsellID_Full_Version_Of_Game);
		}
	}
}

void UIScene_MainMenu::tick()
{
	UIScene::tick();


	if(m_bWaitingForDLCInfo)
	{	
		if(app.GetTMSDLCInfoRead())
		{		
			m_bWaitingForDLCInfo=false;
			ProfileManager.SetLockedProfile(m_iPad);
			ui.NavigateToScene(ProfileManager.GetPrimaryPad(),eUIScene_DLCMainMenu);		
		}
	}

	if(g_NetworkManager.ShouldMessageForFullSession())
	{
		UINT uiIDA[1];
		uiIDA[0]=IDS_CONFIRM_OK;
		ui.RequestMessageBox( IDS_CONNECTION_FAILED, IDS_IN_PARTY_SESSION_FULL, uiIDA,1,ProfileManager.GetPrimaryPad(),NULL,NULL, app.GetStringTable());
	}

}

void UIScene_MainMenu::RunAchievements(int iPad)
{
	{
		Minecraft *pMinecraft = Minecraft::GetInstance();
		pMinecraft->setScreen(new AchievementScreen(pMinecraft->stats[0]));
	}
	UINT uiIDA[1];
	uiIDA[0]=IDS_OK;

	// guests can't look at achievements
	if(ProfileManager.IsGuest(iPad))
	{
		ui.RequestMessageBox(IDS_PRO_GUESTPROFILE_TITLE, IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
	}
	else
	{
		XShowAchievementsUI( iPad );
	}
}

void UIScene_MainMenu::RunHelpAndOptions(int iPad)
{
	if(ProfileManager.IsGuest(iPad))
	{
		UINT uiIDA[1];
		uiIDA[0]=IDS_OK;
		ui.RequestMessageBox(IDS_PRO_GUESTPROFILE_TITLE, IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
	}
	else
	{
		// If the player was signed in before selecting play, we'll not have read the profile yet, so query the sign-in status to get this to happen
		ProfileManager.QuerySigninStatus();

#if TO_BE_IMPLEMENTED
		// 4J-PB - You can be offline and still can go into help and options
		if(app.GetTMSDLCInfoRead() || !ProfileManager.IsSignedInLive(iPad))
#endif
		{
			ProfileManager.SetLockedProfile(iPad);
			ui.ShowPlayerDisplayname(true);
			ui.NavigateToScene(iPad,eUIScene_HelpAndOptionsMenu);
		}
#if TO_BE_IMPLEMENTED
		else
		{
			// Changing to async TMS calls
			app.SetTMSAction(iPad,eTMSAction_TMSPP_RetrieveFiles_HelpAndOptions);

			// block all input
			m_bIgnorePress=true;
			// We want to hide everything in this scene and display a timer until we get a completion for the TMS files
			for(int i=0;i<BUTTONS_MAX;i++)
			{
				m_Buttons[i].SetShow(FALSE);
			}

			updateTooltips();

			m_Timer.SetShow(TRUE);
		}
#endif
	}
}

void UIScene_MainMenu::LoadTrial(void)
{		
	app.SetTutorialMode( true );

	// clear out the app's terrain features list
	app.ClearTerrainFeaturePosition();

	StorageManager.ResetSaveData();

	// Need to set the mode as trial
	ProfileManager.StartTrialGame();

	// No saving in the trial
	StorageManager.SetSaveDisabled(true);

	// Set the global flag, so that we don't disable saving again once the save is complete
	app.SetGameHostOption(eGameHostOption_DisableSaving, 1);

	StorageManager.SetSaveTitle(L"Tutorial");

	// Reset the autosave time
	app.SetAutosaveTimerTime();

	// not online for the trial game
	g_NetworkManager.HostGame(0,false,true,MINECRAFT_NET_MAX_PLAYERS,0);

	g_NetworkManager.FakeLocalPlayerJoined();

	NetworkGameInitData *param = new NetworkGameInitData();
	param->seed = 0;
	param->saveData = NULL;
	param->settings = app.GetGameHostOption( eGameHostOption_Tutorial ) | app.GetGameHostOption(eGameHostOption_DisableSaving);

	vector<LevelGenerationOptions *> *generators = app.getLevelGenerators();
	param->levelGen = generators->at(0);

	LoadingInputParams *loadingParams = new LoadingInputParams();
	loadingParams->func = &CGameNetworkManager::RunNetworkGameThreadProc;
	loadingParams->lpParam = (LPVOID)param;

	UIFullscreenProgressCompletionData *completionData = new UIFullscreenProgressCompletionData();
	completionData->bShowBackground=TRUE;
	completionData->bShowLogo=TRUE;
	completionData->type = e_ProgressCompletion_CloseAllPlayersUIScenes;
	completionData->iPad = ProfileManager.GetPrimaryPad();
	loadingParams->completionData = completionData;

	ui.ShowTrialTimer(true);
	
	ui.ShowPlayerDisplayname(true);
	ui.NavigateToScene(ProfileManager.GetPrimaryPad(),eUIScene_FullscreenProgress, loadingParams);
}

void UIScene_MainMenu::handleUnlockFullVersion()
{
	m_buttons[(int)eControl_UnlockOrDLC].setLabel(app.GetString(IDS_DOWNLOADABLECONTENT),true);
}


