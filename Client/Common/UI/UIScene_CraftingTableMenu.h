#pragma once

#include "UIScene_AbstractContainerMenu.h"
#include "IUIScene_CraftingTableMenu.h"

class CraftingMenu;

class UIScene_CraftingTableMenu : public UIScene_AbstractContainerMenu, public IUIScene_CraftingTableMenu
{
public:
	UIScene_CraftingTableMenu(int iPad, void *initData, UILayer *parentLayer);

	virtual EUIScene getSceneType() { return eUIScene_CraftingTableMenu; }

protected:
	UIControl_SlotList m_slotListCraftGrid;
	UIControl_SlotList m_slotListCraftResult;
	UIControl_Label m_labelCrafting;

	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene_AbstractContainerMenu)
		UI_BEGIN_MAP_CHILD_ELEMENTS( m_controlMainPanel )
			UI_MAP_ELEMENT( m_slotListCraftGrid, "craftGrid")
			UI_MAP_ELEMENT( m_slotListCraftResult, "craftResult")
			UI_MAP_ELEMENT( m_labelCrafting, "craftingLabel")
		UI_END_MAP_CHILD_ELEMENTS()
	UI_END_MAP_ELEMENTS_AND_NAMES()

	virtual wstring getMoviePath();
	virtual void handleReload();

	virtual int getSectionColumns(ESceneSection eSection);
	virtual int getSectionRows(ESceneSection eSection);
	virtual void GetPositionOfSection( ESceneSection eSection, UIVec2D* pPosition );
	virtual void GetItemScreenData( ESceneSection eSection, int iItemIndex, UIVec2D* pPosition, UIVec2D* pSize );
	virtual void handleSectionClick(ESceneSection eSection) {}
	virtual void setSectionSelectedSlot(ESceneSection eSection, int x, int y);

	virtual UIControl *getSection(ESceneSection eSection);
};
