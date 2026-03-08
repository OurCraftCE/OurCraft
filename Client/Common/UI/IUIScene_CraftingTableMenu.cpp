#include "stdafx.h"
#include "IUIScene_CraftingTableMenu.h"
#include "..\..\..\World\Container\CraftingMenu.h"

IUIScene_AbstractContainerMenu::ESceneSection IUIScene_CraftingTableMenu::GetSectionAndSlotInDirection( ESceneSection eSection, ETapState eTapDirection, int *piTargetX, int *piTargetY )
{
	ESceneSection newSection = eSection;

	switch( eSection )
	{
		case eSectionCraftTableGrid:
			if(eTapDirection == eTapStateDown)
				newSection = eSectionCraftTableInventory;
			else if(eTapDirection == eTapStateUp)
				newSection = eSectionCraftTableUsing;
			else if(eTapDirection == eTapStateRight)
				newSection = eSectionCraftTableResult;
			break;
		case eSectionCraftTableResult:
			if(eTapDirection == eTapStateDown)
				newSection = eSectionCraftTableInventory;
			else if(eTapDirection == eTapStateUp)
				newSection = eSectionCraftTableUsing;
			else if(eTapDirection == eTapStateLeft)
				newSection = eSectionCraftTableGrid;
			break;
		case eSectionCraftTableInventory:
			if(eTapDirection == eTapStateDown)
				newSection = eSectionCraftTableUsing;
			else if(eTapDirection == eTapStateUp)
				newSection = eSectionCraftTableGrid;
			break;
		case eSectionCraftTableUsing:
			if(eTapDirection == eTapStateDown)
				newSection = eSectionCraftTableGrid;
			else if(eTapDirection == eTapStateUp)
				newSection = eSectionCraftTableInventory;
			break;
		default:
			assert( false );
			break;
	}

	updateSlotPosition(eSection, newSection, eTapDirection, piTargetX, piTargetY, 0);
	return newSection;
}

int IUIScene_CraftingTableMenu::getSectionStartOffset(ESceneSection eSection)
{
	int offset = 0;
	switch( eSection )
	{
		case eSectionCraftTableGrid:
			offset = CraftingMenu::CRAFT_SLOT_START;
			break;
		case eSectionCraftTableResult:
			offset = CraftingMenu::RESULT_SLOT;
			break;
		case eSectionCraftTableInventory:
			offset = CraftingMenu::INV_SLOT_START;
			break;
		case eSectionCraftTableUsing:
			offset = CraftingMenu::USE_ROW_SLOT_START;
			break;
		default:
			assert( false );
			break;
	}
	return offset;
}
