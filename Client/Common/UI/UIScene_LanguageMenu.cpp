#include "stdafx.h"
#include "UI.h"
#include "UIScene_LanguageMenu.h"
#include "..\..\OurCraft.h"

// Languages in the UI
UIScene_LanguageMenu::LanguageEntry UIScene_LanguageMenu::s_languages[] =
{
	{ eMCLang_enUS,  L"English" },
	{ eMCLang_enGB,  L"English (UK)" },
	{ eMCLang_deDE,  L"Deutsch" },
	{ eMCLang_frFR,  L"Fran\x00E7" L"ais" },
	{ eMCLang_esES,  L"Espa\x00F1" L"ol" },
	{ eMCLang_esMX,  L"Espa\x00F1" L"ol (M\x00E9xico)" },
	{ eMCLang_itIT,  L"Italiano" },
	{ eMCLang_ptPT,  L"Portugu\x00EA" L"s" },
	{ eMCLang_ptBR,  L"Portugu\x00EA" L"s (Brasil)" },
	{ eMCLang_ruRU,  L"\x0420\x0443\x0441\x0441\x043A\x0438\x0439" },
	{ eMCLang_jaJP,  L"\x65E5\x672C\x8A9E" },
	{ eMCLang_koKR,  L"\xD55C\xAD6D\xC5B4" },
	{ eMCLang_zhCHS, L"\x7B80\x4F53\x4E2D\x6587" },
	{ eMCLang_zhCHT, L"\x7E41\x9AD4\x4E2D\x6587" },
	{ eMCLang_nlNL,  L"Nederlands" },
	{ eMCLang_plPL,  L"Polski" },
	{ eMCLang_svSV,  L"Svenska" },
	{ eMCLang_noNO,  L"Norsk" },
	{ eMCLang_daDA,  L"Dansk" },
	{ eMCLang_fiFI,  L"Suomi" },
	{ eMCLang_trTR,  L"T\x00FC" L"rk\x00E7" L"e" },
	{ eMCLang_elEL,  L"\x0395\x03BB\x03BB\x03B7\x03BD\x03B9\x03BA\x03AC" },
	{ eMCLang_csCZ,  L"\x010C" L"e\x0161" L"tina" },
	{ eMCLang_skSK,  L"Sloven\x010D" L"ina" },
};

int UIScene_LanguageMenu::s_languageCount = sizeof(s_languages) / sizeof(s_languages[0]);

UIScene_LanguageMenu::UIScene_LanguageMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	initialiseMovie();

	m_buttonListLanguages.init(eControl_LanguageList);

	m_iCurrentLangIndex = 0;

	populateList();
}

void UIScene_LanguageMenu::populateList()
{
	m_buttonListLanguages.clearList();

	unsigned char ucCurrentLang = app.GetMinecraftLanguage(m_iPad);
	m_iCurrentLangIndex = 0;

	for (int i = 0; i < s_languageCount; ++i)
	{
		m_buttonListLanguages.addItem(s_languages[i].pwchName, i);

		if (s_languages[i].ucLangId == ucCurrentLang)
		{
			m_iCurrentLangIndex = i;
		}
	}

	m_buttonListLanguages.setCurrentSelection(m_iCurrentLangIndex);
}

wstring UIScene_LanguageMenu::getMoviePath()
{
	if (app.GetLocalPlayerCount() > 1)
	{
		return L"HowToPlayMenuSplit";
	}
	else
	{
		return L"HowToPlayMenu";
	}
}

void UIScene_LanguageMenu::updateTooltips()
{
	ui.SetTooltips(m_iPad, IDS_TOOLTIPS_SELECT, IDS_TOOLTIPS_BACK);
}

void UIScene_LanguageMenu::updateComponents()
{
	bool bNotInGame = (Minecraft::GetInstance()->level == NULL);
	if (bNotInGame)
	{
		m_parentLayer->showComponent(m_iPad, eUIComponent_Panorama, true);
		m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, true);
	}
	else
	{
		m_parentLayer->showComponent(m_iPad, eUIComponent_Panorama, false);

		if (app.GetLocalPlayerCount() == 1) m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, true);
		else m_parentLayer->showComponent(m_iPad, eUIComponent_Logo, false);
	}
}

void UIScene_LanguageMenu::handleReload()
{
	populateList();
}

void UIScene_LanguageMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

	switch (key)
	{
	case ACTION_MENU_CANCEL:
		if (pressed && !repeat)
		{
			navigateBack();
		}
		break;
	case ACTION_MENU_OK:
		sendInputToMovie(key, repeat, pressed, released);
		break;
	case ACTION_MENU_UP:
	case ACTION_MENU_DOWN:
	case ACTION_MENU_PAGEUP:
	case ACTION_MENU_PAGEDOWN:
		sendInputToMovie(key, repeat, pressed, released);
		break;
	}
}

void UIScene_LanguageMenu::handlePress(F64 controlId, F64 childId)
{
	if ((int)controlId == eControl_LanguageList)
	{
		ui.PlayUISFX(eSFX_Press);

		int selectedIndex = (int)childId;
		if (selectedIndex >= 0 && selectedIndex < s_languageCount)
		{
			unsigned char ucNewLang = s_languages[selectedIndex].ucLangId;
			unsigned char ucOldLang = app.GetMinecraftLanguage(m_iPad);

			if (ucNewLang != ucOldLang)
			{
				// the language choice
				app.SetMinecraftLanguage(m_iPad, ucNewLang);

				// Reload the string table with the new language
				app.loadStringTable();

				bool bInGame = (Minecraft::GetInstance()->level != NULL);
				if (!bInGame)
				{
					// Not in game - close all scenes and open a fresh main menu
					// so all labels are created with the new language strings
					ui.CloseUIScenes(m_iPad);
					ui.NavigateToScene(m_iPad, eUIScene_MainMenu);
				}
				else
				{
					// In game - just go back, menus will reload on next open
					navigateBack();
				}
			}
			else
			{
				// Same language selected, just go back
				navigateBack();
			}
		}
	}
}
