#include "stdafx.h"
#include "Windows64_UIController.h"

ConsoleUIController ui;

void ConsoleUIController::init(ID3D11Device *dev, ID3D11DeviceContext *ctx, ID3D11RenderTargetView* pRenderTargetView, ID3D11DepthStencilView* pDepthStencilView, S32 w, S32 h)
{
	m_pRenderTargetView = pRenderTargetView;
	m_pDepthStencilView = pDepthStencilView;
	preInit(w, h);
	postInit();
}

void ConsoleUIController::render()
{
	renderScenes();
}

CustomDrawData *ConsoleUIController::setupCustomDraw(UIScene *scene, IggyCustomDrawCallbackRegion *region)
{
	return nullptr;
}

CustomDrawData *ConsoleUIController::calculateCustomDraw(IggyCustomDrawCallbackRegion *region)
{
	return nullptr;
}

void ConsoleUIController::endCustomDraw(IggyCustomDrawCallbackRegion *region)
{
}

void ConsoleUIController::setTileOrigin(S32 xPos, S32 yPos)
{
}

void ConsoleUIController::beginIggyCustomDraw4J(IggyCustomDrawCallbackRegion *region, CustomDrawData *customDrawRegion)
{
}

GDrawTexture *ConsoleUIController::getSubstitutionTexture(int textureId)
{
	return nullptr;
}

void ConsoleUIController::destroySubstitutionTexture(void *destroyCallBackData, GDrawTexture *handle)
{
}

void ConsoleUIController::shutdown()
{
}
