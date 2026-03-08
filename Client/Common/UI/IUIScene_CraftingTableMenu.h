#pragma once

#include "IUIScene_AbstractContainerMenu.h"

class CraftingMenu;

class IUIScene_CraftingTableMenu : public virtual IUIScene_AbstractContainerMenu
{
protected:
	virtual ESceneSection GetSectionAndSlotInDirection( ESceneSection eSection, ETapState eTapDirection, int *piTargetX, int *piTargetY );
	int getSectionStartOffset(ESceneSection eSection);
};
