#pragma once

#include "UIScene.h"

class UIScene_LanguageMenu : public UIScene
{
private:
	enum EControls
	{
		eControl_LanguageList,
	};

	struct LanguageEntry
	{
		unsigned char ucLangId;
		const wchar_t *pwchName;
	};

	static LanguageEntry s_languages[];
	static int s_languageCount;

	UIControl_ButtonList m_buttonListLanguages;
	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
		UI_MAP_ELEMENT( m_buttonListLanguages, "HowToList")
	UI_END_MAP_ELEMENTS_AND_NAMES()

	int m_iCurrentLangIndex;

public:
	UIScene_LanguageMenu(int iPad, void *initData, UILayer *parentLayer);

	virtual EUIScene getSceneType() { return eUIScene_LanguageMenu; }

	virtual void updateTooltips();
	virtual void updateComponents();
	virtual void handleReload();

protected:
	virtual wstring getMoviePath();

public:
	virtual void handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled);

protected:
	void handlePress(F64 controlId, F64 childId);

private:
	void populateList();
};
