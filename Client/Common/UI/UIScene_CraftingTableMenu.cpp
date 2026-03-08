#include "stdafx.h"
#include "UI.h"
#include "UIScene_CraftingTableMenu.h"

#include "..\..\..\World\FwdHeaders/net.minecraft.world.inventory.h"
#include "..\..\..\World\FwdHeaders/net.minecraft.world.item.h"
#include "..\..\..\World\FwdHeaders/net.minecraft.world.item.crafting.h"
#include "../../Game/LocalPlayer.h"
#include "..\..\OurCraft.h"
#include "..\Tutorial\Tutorial.h"
#include "..\Tutorial\TutorialMode.h"
#include "..\Tutorial\TutorialEnum.h"

UIScene_CraftingTableMenu::UIScene_CraftingTableMenu(int iPad, void *_initData, UILayer *parentLayer) : UIScene_AbstractContainerMenu(iPad, parentLayer)
{
	CraftingPanelScreenInput *initData = (CraftingPanelScreenInput *)_initData;

	// Setup all the Iggy references we need for this scene
	initialiseMovie();

	Minecraft *pMinecraft = Minecraft::GetInstance();
	if( pMinecraft->localgameModes[iPad] != NULL )
	{
		TutorialMode *gameMode = (TutorialMode *)pMinecraft->localgameModes[initData->iPad];
		m_previousTutorialState = gameMode->getTutorial()->getCurrentState();
		gameMode->getTutorial()->changeTutorialState(e_Tutorial_State_3x3Crafting_Menu, this);
	}

	CraftingMenu* menu = new CraftingMenu(initData->player->inventory, initData->player->level, initData->x, initData->y, initData->z);

	Initialize( initData->iPad, menu, true, CraftingMenu::INV_SLOT_START, eSectionCraftTableUsing, eSectionCraftTableMax);

	m_slotListCraftGrid.addSlots(CraftingMenu::CRAFT_SLOT_START, CraftingMenu::CRAFT_SLOT_END - CraftingMenu::CRAFT_SLOT_START);
	m_slotListCraftResult.addSlots(CraftingMenu::RESULT_SLOT, 1);

	if(initData) delete initData;
}

wstring UIScene_CraftingTableMenu::getMoviePath()
{
	if(app.GetLocalPlayerCount() > 1)
	{
		return L"ChestMenuSplit";
	}
	else
	{
		return L"ChestMenu";
	}
}

void UIScene_CraftingTableMenu::handleReload()
{
	Initialize( m_iPad, m_menu, true, CraftingMenu::INV_SLOT_START, eSectionCraftTableUsing, eSectionCraftTableMax );

	m_slotListCraftGrid.addSlots(CraftingMenu::CRAFT_SLOT_START, CraftingMenu::CRAFT_SLOT_END - CraftingMenu::CRAFT_SLOT_START);
	m_slotListCraftResult.addSlots(CraftingMenu::RESULT_SLOT, 1);
}

int UIScene_CraftingTableMenu::getSectionColumns(ESceneSection eSection)
{
	int cols = 0;
	switch( eSection )
	{
		case eSectionCraftTableGrid:
			cols = 3;
			break;
		case eSectionCraftTableResult:
			cols = 1;
			break;
		case eSectionCraftTableInventory:
			cols = 9;
			break;
		case eSectionCraftTableUsing:
			cols = 9;
			break;
		default:
			assert( false );
			break;
	}
	return cols;
}

int UIScene_CraftingTableMenu::getSectionRows(ESceneSection eSection)
{
	int rows = 0;
	switch( eSection )
	{
		case eSectionCraftTableGrid:
			rows = 3;
			break;
		case eSectionCraftTableResult:
			rows = 1;
			break;
		case eSectionCraftTableInventory:
			rows = 3;
			break;
		case eSectionCraftTableUsing:
			rows = 1;
			break;
		default:
			assert( false );
			break;
	}
	return rows;
}

void UIScene_CraftingTableMenu::GetPositionOfSection( ESceneSection eSection, UIVec2D* pPosition )
{
	switch( eSection )
	{
	case eSectionCraftTableGrid:
		pPosition->x = m_slotListCraftGrid.getXPos();
		pPosition->y = m_slotListCraftGrid.getYPos();
		break;
	case eSectionCraftTableResult:
		pPosition->x = m_slotListCraftResult.getXPos();
		pPosition->y = m_slotListCraftResult.getYPos();
		break;
	case eSectionCraftTableInventory:
		pPosition->x = m_slotListInventory.getXPos();
		pPosition->y = m_slotListInventory.getYPos();
		break;
	case eSectionCraftTableUsing:
		pPosition->x = m_slotListHotbar.getXPos();
		pPosition->y = m_slotListHotbar.getYPos();
		break;
	default:
		assert( false );
		break;
	}
}

void UIScene_CraftingTableMenu::GetItemScreenData( ESceneSection eSection, int iItemIndex, UIVec2D* pPosition, UIVec2D* pSize )
{
	UIVec2D sectionSize;

	switch( eSection )
	{
	case eSectionCraftTableGrid:
		sectionSize.x = m_slotListCraftGrid.getWidth();
		sectionSize.y = m_slotListCraftGrid.getHeight();
		break;
	case eSectionCraftTableResult:
		sectionSize.x = m_slotListCraftResult.getWidth();
		sectionSize.y = m_slotListCraftResult.getHeight();
		break;
	case eSectionCraftTableInventory:
		sectionSize.x = m_slotListInventory.getWidth();
		sectionSize.y = m_slotListInventory.getHeight();
		break;
	case eSectionCraftTableUsing:
		sectionSize.x = m_slotListHotbar.getWidth();
		sectionSize.y = m_slotListHotbar.getHeight();
		break;
	default:
		assert( false );
		break;
	}

	int rows = getSectionRows(eSection);
	int cols = getSectionColumns(eSection);

	pSize->x = sectionSize.x/cols;
	pSize->y = sectionSize.y/rows;

	int itemCol = iItemIndex % cols;
	int itemRow = iItemIndex/cols;

	pPosition->x = itemCol * pSize->x;
	pPosition->y = itemRow * pSize->y;
}

void UIScene_CraftingTableMenu::setSectionSelectedSlot(ESceneSection eSection, int x, int y)
{
	int cols = getSectionColumns(eSection);

	int index = (y * cols) + x;

	UIControl_SlotList *slotList = NULL;
	switch( eSection )
	{
	case eSectionCraftTableGrid:
		slotList = &m_slotListCraftGrid;
		break;
	case eSectionCraftTableResult:
		slotList = &m_slotListCraftResult;
		break;
	case eSectionCraftTableInventory:
		slotList = &m_slotListInventory;
		break;
	case eSectionCraftTableUsing:
		slotList = &m_slotListHotbar;
		break;
	default:
		assert( false );
		return;
	}

	slotList->setHighlightSlot(index);
}

UIControl *UIScene_CraftingTableMenu::getSection(ESceneSection eSection)
{
	UIControl *control = NULL;
	switch( eSection )
	{
	case eSectionCraftTableGrid:
		control = &m_slotListCraftGrid;
		break;
	case eSectionCraftTableResult:
		control = &m_slotListCraftResult;
		break;
	case eSectionCraftTableInventory:
		control = &m_slotListInventory;
		break;
	case eSectionCraftTableUsing:
		control = &m_slotListHotbar;
		break;
	default:
		assert( false );
		break;
	}
	return control;
}
