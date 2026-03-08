#pragma once

#include "UIScene.h"

struct DLCCatalogEntry;

class UIScene_DLCOffersMenu : public UIScene
{
private:
	enum EControls
	{
		eControl_OffersList,
	};

	bool m_bIsSD;
	bool m_bHasPurchased;
	bool m_bIsSelected;

	UIControl_DLCList m_buttonListOffers;
	UIControl_Label m_labelOffers, m_labelPriceTag, m_labelXboxStore;
	UIControl_HTMLLabel m_labelHTMLSellText;
	UIControl_BitmapIcon m_bitmapIconOfferImage;
	UIControl m_Timer;
	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
		UI_MAP_ELEMENT( m_buttonListOffers, "OffersList")
		UI_MAP_ELEMENT( m_labelOffers, "OffersList_Title")
		UI_MAP_ELEMENT( m_labelPriceTag, "PriceTag")
		UI_MAP_ELEMENT( m_labelHTMLSellText, "HTMLSellText")
		UI_MAP_ELEMENT( m_bitmapIconOfferImage, "DLCIcon" )
		UI_MAP_ELEMENT( m_Timer, "Timer")

		// XboxLabel not present in Windows SWF
	UI_END_MAP_ELEMENTS_AND_NAMES()
public:
	UIScene_DLCOffersMenu(int iPad, void *initData, UILayer *parentLayer);
	~UIScene_DLCOffersMenu();
	static int ExitDLCOffersMenu(void *pParam,int iPad,CStorageManager::EMessageResult result);

	virtual EUIScene getSceneType() { return eUIScene_DLCOffersMenu;}
	virtual void tick();
	virtual void updateTooltips();

protected:
	virtual wstring getMoviePath();

public:
	// INPUT
	virtual void handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled);

	virtual void handlePress(F64 controlId, F64 childId);
	virtual void handleSelectionChanged(F64 selectedId);
	virtual void handleFocusChange(F64 controlId, F64 childId);
	virtual void handleTimerComplete(int id);

	virtual void HandleDLCLicenseChange();

private:
	// Populate offer list from local DLCCatalog
	void PopulateCatalogOffers();
	void UpdateDisplayFromCatalog(const DLCCatalogEntry *entry);

	// Legacy methods kept for interface compatibility
	void GetDLCInfo( int iOfferC, bool bUpdateOnly=false );
	void UpdateTooltips(MARKETPLACE_CONTENTOFFER_INFO& xOffer);
	bool UpdateDisplay(MARKETPLACE_CONTENTOFFER_INFO& xOffer);

	static int OrderSortFunction(const void* a, const void* b);

	bool m_bIgnorePress;
	bool m_bDLCRequiredIsRetrieved;
	DLC_INFO *m_pNoImageFor_DLC;

	typedef struct
	{
		unsigned int uiContentIndex;
		unsigned int uiSortIndex;
	}
	SORTINDEXSTRUCT;

	// Load a thumbnail image from file into memory and register as substitution texture
	bool LoadThumbnailFromFile(const wstring &filePath);
	// Load first skin thumbnail from an installed DLC pack for preview
	bool LoadSkinPreviewFromPack(const wstring &packId);

	// Catalog entry IDs mapped to list indices
	vector<wstring> m_catalogEntryIds;
	vector <wstring>m_vIconRetrieval;
	bool m_bSelectionChanged;

	// Thumbnail image cache to avoid reloading
	unordered_map<wstring, BYTE *> m_thumbnailCache;
	unordered_map<wstring, DWORD> m_thumbnailSizes;

	bool m_bProductInfoShown;
	int m_iProductInfoIndex;
	int m_iCurrentDLC;
	int m_iTotalDLC;
	bool m_bAddAllDLCButtons;
};
