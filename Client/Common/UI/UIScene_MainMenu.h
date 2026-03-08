#pragma once

#include "UIScene.h"

class UIScene_MainMenu : public UIScene
{
private:
	enum EControls
	{
		eControl_PlayGame,
		eControl_QuitGame,
		eControl_Achievements,
		eControl_HelpAndOptions,
		eControl_UnlockOrDLC,
		eControl_Count,
	};

// 	enum EPatchCheck
// 	{
// 		ePatchCheck_Idle,
// 		ePatchCheck_Init,
// 		ePatchCheck_Running,
// 	};
// #endif

	UIControl_Button m_buttons[eControl_Count];
	UIControl m_controlTimer;
	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
		UI_MAP_ELEMENT( m_buttons[(int)eControl_PlayGame], "Button1")
		UI_MAP_ELEMENT( m_buttons[(int)eControl_QuitGame], "Button2")
		UI_MAP_ELEMENT( m_buttons[(int)eControl_Achievements], "Button3")
		UI_MAP_ELEMENT( m_buttons[(int)eControl_HelpAndOptions], "Button4")
		UI_MAP_ELEMENT( m_buttons[(int)eControl_UnlockOrDLC], "Button5")
		UI_MAP_ELEMENT( m_controlTimer, "Timer")
	UI_END_MAP_ELEMENTS_AND_NAMES()
	
	static Random *random;
	bool m_bIgnorePress;
	bool m_bRunGameChosen;
	bool m_bErrorDialogRunning;
	bool m_bTrialVersion;
	bool m_bLoadTrialOnNetworkManagerReady;

	bool m_bWaitingForDLCInfo;
	
	float m_fScreenWidth,m_fScreenHeight;
	float m_fRawWidth,m_fRawHeight;
	vector<wstring> m_splashes;
	wstring m_splash;
	enum eSplashIndexes
	{
		eSplashHappyBirthdayEx = 0,
		eSplashHappyBirthdayNotch,
		eSplashMerryXmas,
		eSplashHappyNewYear,

		// The start index in the splashes vector from which we can select a random splash
		eSplashRandomStart,
	};

	enum eActions
	{
		eAction_None=0,
		eAction_RunGame,
		eAction_QuitGame,
		eAction_RunAchievements,
		eAction_RunHelpAndOptions,
		eAction_RunUnlockOrDLC,

	};
	eActions m_eAction;
public:
	UIScene_MainMenu(int iPad, void *initData, UILayer *parentLayer);
	virtual ~UIScene_MainMenu();

	// Returns true if this scene has focus for the pad passed in
	virtual bool hasFocus(int iPad) { return bHasFocus; }
	
	virtual void updateTooltips();
	virtual void updateComponents();

	virtual EUIScene getSceneType() { return eUIScene_MainMenu;}

	virtual void customDraw(IggyCustomDrawCallbackRegion *region);
protected:
	void customDrawSplash(IggyCustomDrawCallbackRegion *region);


	virtual wstring getMoviePath();

public:
	virtual void tick();
	// INPUT
	virtual void handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled);

	virtual void handleUnlockFullVersion();

protected:
	void handlePress(F64 controlId, F64 childId);

	void handleGainFocus(bool navBack);

	virtual long long getDefaultGtcButtons() { return 0; }

private:
	void RunPlayGame(int iPad);
	void RunUnlockOrDLC(int iPad);
	void RunAchievements(int iPad);
	void RunHelpAndOptions(int iPad);

	void RunAction(int iPad);

	static void LoadTrial();

	static int ChooseUser_SignInReturned(void *pParam,bool bContinue, int iPad);
	static int CreateLoad_SignInReturned(void *pParam,bool bContinue, int iPad);
	static int HelpAndOptions_SignInReturned(void *pParam,bool bContinue,int iPad);
	static int Achievements_SignInReturned(void *pParam,bool bContinue,int iPad);
	static int MustSignInReturned(void *pParam,int iPad,CStorageManager::EMessageResult result);

	static int UnlockFullGame_SignInReturned(void *pParam,bool bContinue,int iPad);
	static int ExitGameReturned(void *pParam,int iPad,CStorageManager::EMessageResult result);


};
