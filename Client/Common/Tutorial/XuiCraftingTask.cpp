#include "stdafx.h"
#include "..\..\..\World\Items\ItemInstance.h"
#include "Tutorial.h"
#include "XuiCraftingTask.h"

bool XuiCraftingTask::isCompleted()
{
	// This doesn't seem to work
	//IUIScene_CraftingMenu *craftScene = reinterpret_cast<IUIScene_CraftingMenu *>(tutorial->getScene());
	return true;
}
