#include "stdafx.h"
#include "UI.h"
#include "UIScene_DLCOffersMenu.h"
#include "..\..\..\World\Math\StringHelpers.h"
#include "..\DLC\DLCCatalog.h"
#include "..\DLC\DLCInstaller.h"


#define PLAYER_ONLINE_TIMER_ID 0
#define PLAYER_ONLINE_TIMER_TIME 100

UIScene_DLCOffersMenu::UIScene_DLCOffersMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	m_bProductInfoShown=false;
	DLCOffersParam *param=(DLCOffersParam *)initData;
	m_iProductInfoIndex=param->iType;
	m_iCurrentDLC=0;
	m_iTotalDLC=0;
	m_bAddAllDLCButtons=true;

	// Setup all the Iggy references we need for this scene
	initialiseMovie();

	m_bIsSD=!RenderManager.IsHiDef() && !RenderManager.IsWidescreen();

	m_labelOffers.init(app.GetString(IDS_DOWNLOADABLE_CONTENT_OFFERS));
	m_buttonListOffers.init(eControl_OffersList);
	m_labelHTMLSellText.init(" ");
	m_labelPriceTag.init(" ");
	TelemetryManager->RecordMenuShown(m_iPad, eUIScene_DLCOffersMenu, 0);

	m_bHasPurchased = false;
	m_bIsSelected = false;

	if(m_loadedResolution == eSceneResolution_1080)
	{
		m_labelXboxStore.init( app.GetString(IDS_XBOX_STORE) );
	}

	m_pNoImageFor_DLC = NULL;
	m_bDLCRequiredIsRetrieved=false;
	m_bIgnorePress=true;
	m_bSelectionChanged=true;

	// Local catalog is available immediately - no network wait needed
	PopulateCatalogOffers();
	m_bDLCRequiredIsRetrieved = true;
	m_bIgnorePress = false;
	m_bAddAllDLCButtons = false;
	m_Timer.setVisible(false);
}

UIScene_DLCOffersMenu::~UIScene_DLCOffersMenu()
{
	// Free cached thumbnail data
	for (auto it = m_thumbnailCache.begin(); it != m_thumbnailCache.end(); ++it)
	{
		delete[] it->second;
	}
	m_thumbnailCache.clear();
	m_thumbnailSizes.clear();
}

void UIScene_DLCOffersMenu::handleTimerComplete(int id)
{
}

int UIScene_DLCOffersMenu::ExitDLCOffersMenu(void *pParam,int iPad,CStorageManager::EMessageResult result)
{
	UIScene_DLCOffersMenu* pClass = (UIScene_DLCOffersMenu*)pParam;

	ui.NavigateToHomeMenu();

	return 0;
}

wstring UIScene_DLCOffersMenu::getMoviePath()
{
	return L"DLCOffersMenu";
}

void UIScene_DLCOffersMenu::updateTooltips()
{
	int iA = -1;
	if(m_bIsSelected)
	{
		if( !m_bHasPurchased )
		{
			iA = IDS_TOOLTIPS_INSTALL;
		}
		else
		{
			iA = IDS_TOOLTIPS_REINSTALL;
		}
	}
	ui.SetTooltips( m_iPad, iA,IDS_TOOLTIPS_BACK);
}

void UIScene_DLCOffersMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
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
		if(pressed)
		{
			if(m_iTotalDLC > 0)
			{
				if(m_iCurrentDLC > 0)
					m_iCurrentDLC--;

				m_bProductInfoShown = false;
			}
		}
		sendInputToMovie(key, repeat, pressed, released);
		break;

	case ACTION_MENU_DOWN:
		if(pressed)
		{
			if(m_iTotalDLC > 0)
			{
				if(m_iCurrentDLC < (m_iTotalDLC - 1))
					m_iCurrentDLC++;

				m_bProductInfoShown = false;
			}
		}
		sendInputToMovie(key, repeat, pressed, released);
		break;

	case ACTION_MENU_LEFT:
	case ACTION_MENU_RIGHT:
	case ACTION_MENU_OTHER_STICK_DOWN:
	case ACTION_MENU_OTHER_STICK_UP:
		sendInputToMovie(key, repeat, pressed, released);
		break;
	}
}

void UIScene_DLCOffersMenu::handlePress(F64 controlId, F64 childId)
{
	switch((int)controlId)
	{
	case eControl_OffersList:
		{
			int iIndex = (int)childId;
			// Install from local catalog instead of marketplace
			if (iIndex >= 0 && iIndex < (int)m_catalogEntryIds.size())
			{
				const wstring &packId = m_catalogEntryIds[iIndex];
				const DLCCatalogEntry *entry = app.m_dlcCatalog.FindEntry(packId);
				if (entry != NULL)
				{
					if (entry->installed)
					{
						app.DebugPrintf("DLC pack %ls is already installed\n", packId.c_str());
					}
					else
					{
						app.DebugPrintf("Installing DLC pack %ls\n", packId.c_str());
						if (app.InstallLocalDLCPack(packId))
						{
							// Refresh the list to show the tick
							PopulateCatalogOffers();
						}
					}
				}
			}
		}
		break;
	}
}

void UIScene_DLCOffersMenu::handleSelectionChanged(F64 selectedId)
{

}

void UIScene_DLCOffersMenu::handleFocusChange(F64 controlId, F64 childId)
{
	app.DebugPrintf("UIScene_DLCOffersMenu::handleFocusChange\n");
	m_bSelectionChanged=true;
}

void UIScene_DLCOffersMenu::tick()
{
	UIScene::tick();

	if(m_bSelectionChanged && m_bDLCRequiredIsRetrieved)
	{
		if(m_buttonListOffers.hasFocus() && (getControlChildFocus()>-1))
		{
			int iIndex = getControlChildFocus();
			if (iIndex >= 0 && iIndex < (int)m_catalogEntryIds.size())
			{
				const DLCCatalogEntry *entry = app.m_dlcCatalog.FindEntry(m_catalogEntryIds[iIndex]);
				if (entry != NULL)
				{
					UpdateDisplayFromCatalog(entry);
					m_bSelectionChanged=false;
				}
			}
		}
	}
}

void UIScene_DLCOffersMenu::PopulateCatalogOffers()
{
	m_buttonListOffers.clearList();
	m_catalogEntryIds.clear();
	int iCount = 0;

	// Map m_iProductInfoIndex to the content type filter
	eDLCContentType filterType = (eDLCContentType)m_iProductInfoIndex;

	int totalEntries = app.m_dlcCatalog.GetEntryCount();
	for (int i = 0; i < totalEntries; i++)
	{
		const DLCCatalogEntry *entry = app.m_dlcCatalog.GetEntry(i);
		if (entry == NULL) continue;

		// Filter by content type
		if (entry->GetContentType() != filterType)
			continue;

		wstring displayName = entry->name;

		// Remove "Minecraft " prefix if present
		if (displayName.compare(0, 10, L"Minecraft ") == 0)
		{
			displayName = displayName.substr(10);
		}

		m_buttonListOffers.addItem(displayName, entry->installed, iCount);
		m_catalogEntryIds.push_back(entry->id);

		iCount++;
	}

	m_iTotalDLC = iCount;

	if (iCount > 0)
	{
		// Display first entry
		const DLCCatalogEntry *first = app.m_dlcCatalog.FindEntry(m_catalogEntryIds[0]);
		if (first != NULL)
		{
			UpdateDisplayFromCatalog(first);
		}
	}
	else
	{
		wstring wstrTemp = app.GetString(IDS_NO_DLCOFFERS);
		m_labelHTMLSellText.setLabel(wstrTemp);
		m_labelPriceTag.setVisible(false);
	}
}

void UIScene_DLCOffersMenu::UpdateDisplayFromCatalog(const DLCCatalogEntry *entry)
{
	if (entry == NULL) return;

	// Set description text
	m_labelHTMLSellText.setLabel(entry->description);

	// Set price or installed status
	m_labelPriceTag.setVisible(true);
	if (entry->installed)
	{
		m_labelPriceTag.setLabel(L"Installed");
	}
	else
	{
		m_labelPriceTag.setLabel(entry->price);
	}

	// Try to load thumbnail image for the pack preview
	bool bImageLoaded = false;

	if (!entry->thumbnailPath.empty())
	{
		bImageLoaded = LoadThumbnailFromFile(entry->thumbnailPath);
	}

	// For skin packs, try to load a skin preview from the installed pack
	if (!bImageLoaded && entry->installed && entry->GetContentType() == e_DLC_SkinPack)
	{
		bImageLoaded = LoadSkinPreviewFromPack(entry->id);
	}

	// Update tooltip state
	m_bHasPurchased = entry->installed;
	m_bIsSelected = true;
	updateTooltips();
}

bool UIScene_DLCOffersMenu::LoadThumbnailFromFile(const wstring &filePath)
{
	// Check cache first
	auto it = m_thumbnailCache.find(filePath);
	if (it != m_thumbnailCache.end())
	{
		registerSubstitutionTexture(filePath.c_str(), it->second, m_thumbnailSizes[filePath], false);
		m_bitmapIconOfferImage.setTextureName(filePath.c_str());
		return true;
	}

	// Load from disk
	HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	DWORD dwSize = GetFileSize(hFile, NULL);
	if (dwSize == 0 || dwSize >= 4 * 1024 * 1024) // 4MB max
	{
		CloseHandle(hFile);
		return false;
	}

	BYTE *pData = new BYTE[dwSize];
	DWORD dwRead = 0;
	ReadFile(hFile, pData, dwSize, &dwRead, NULL);
	CloseHandle(hFile);

	if (dwRead != dwSize)
	{
		delete[] pData;
		return false;
	}

	// Cache the data (ownership transferred to cache)
	m_thumbnailCache[filePath] = pData;
	m_thumbnailSizes[filePath] = dwSize;

	registerSubstitutionTexture(filePath.c_str(), pData, dwSize, false);
	m_bitmapIconOfferImage.setTextureName(filePath.c_str());
	return true;
}

bool UIScene_DLCOffersMenu::LoadSkinPreviewFromPack(const wstring &packId)
{
	// Look for the first skin file in the installed pack to use as a preview
	DLCPack *pack = app.m_dlcManager.getPack(packId);
	if (pack == NULL)
		return false;

	// Get the first skin file from the pack
	if (pack->getSkinCount() == 0)
		return false;

	DLCSkinFile *skinFile = pack->getSkinFile((DWORD)0);
	if (skinFile == NULL)
		return false;

	// Build the skin texture path from the installed location
	wstring skinPath = L"dlc\\installed\\" + packId + L"\\" + skinFile->getPath();
	return LoadThumbnailFromFile(skinPath);
}

void UIScene_DLCOffersMenu::GetDLCInfo( int iOfferC, bool bUpdateOnly )
{
	// Replaced by PopulateCatalogOffers - kept for interface compatibility
	PopulateCatalogOffers();
}

int UIScene_DLCOffersMenu::OrderSortFunction(const void* a, const void* b)
{
	return ((SORTINDEXSTRUCT*)b)->uiSortIndex - ((SORTINDEXSTRUCT*)a)->uiSortIndex;
}

void UIScene_DLCOffersMenu::UpdateTooltips(MARKETPLACE_CONTENTOFFER_INFO& xOffer)
{
	// Legacy compatibility - now handled by UpdateDisplayFromCatalog
	m_bHasPurchased = xOffer.fUserHasPurchased;
	m_bIsSelected = true;
	updateTooltips();
}

bool UIScene_DLCOffersMenu::UpdateDisplay(MARKETPLACE_CONTENTOFFER_INFO& xOffer)
{
	// Legacy compatibility - kept for interface but no longer primary path
	return false;
}

void UIScene_DLCOffersMenu::HandleDLCLicenseChange()
{
	// Refresh from catalog
	PopulateCatalogOffers();
}
