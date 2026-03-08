#include "stdafx.h"
#include "UI.h"
#include "UIScene.h"

#include "../../Render/Lighting.h"
#include "../../Game/LocalPlayer.h"
#include "../../Render/ItemRenderer.h"
#include "..\..\..\World\FwdHeaders/net.minecraft.world.item.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include "..\..\Platform\RmlUIManager.h"

UIScene::UIScene(int iPad, UILayer *parentLayer)
{
	m_parentLayer = parentLayer;
	m_iPad = iPad;
	swf = nullptr;
	m_document = nullptr;
	m_pItemRenderer = nullptr;

	bHasFocus = false;
	m_hasTickedOnce = false;
	m_bFocussedOnce = false;
	m_bVisible = true;
	m_bCanHandleInput = false;
	m_bIsReloading = false;

	m_iFocusControl = -1;
	m_iFocusChild = 0;
	m_lastOpacity = 1.0f;
	m_bUpdateOpacity = false;

	m_backScene = nullptr;

	m_cacheSlotRenders = false;
	m_needsCacheRendered = true;
	m_expectedCachedSlotCount = 0;
	m_callbackUniqueId = 0;
}

UIScene::~UIScene()
{
	if (m_document)
	{
		m_document->RemoveEventListener("click", this, true);
		m_document->Close();
		m_document = nullptr;
	}

	for (AUTO_VAR(it, m_registeredTextures.begin()); it != m_registeredTextures.end(); ++it)
		ui.unregisterSubstitutionTexture(it->first, it->second);

	if (m_callbackUniqueId != 0)
		ui.UnregisterCallbackId(m_callbackUniqueId);

	if (m_pItemRenderer != nullptr) delete m_pItemRenderer;
}

// ---- Document management ----

void UIScene::loadDocument()
{
	Rml::Context *ctx = RmlUIManager::Get().GetContext();
	if (!ctx) return;

	wstring wpath = getMoviePath();
	// Convert to UTF-8 for RmlUi
	string path(wpath.begin(), wpath.end());
	path += ".rml";

	m_document = ctx->LoadDocument(path);
	if (m_document)
	{
		m_document->AddEventListener("click",  this, true);
		m_document->AddEventListener("change", this, true);
		m_document->Show();
		m_movieWidth  = (int)m_document->GetClientWidth();
		m_movieHeight = (int)m_document->GetClientHeight();
		m_renderWidth  = m_movieWidth;
		m_renderHeight = m_movieHeight;
	}
}

void UIScene::loadMovie()
{
	// Iggy path gone — load RmlUi document instead
	loadDocument();
	m_loadedResolution = eSceneResolution_1080;
}

void UIScene::destroyMovie()
{
	if (m_document)
	{
		m_document->RemoveEventListener("click", this, true);
		m_document->Close();
		m_document = nullptr;
	}
	m_controls.clear();
	m_fastNames.clear();
}

void UIScene::reloadMovie(bool force)
{
	if (!force && (stealsFocus() && (getSceneType() != eUIScene_FullscreenProgress && !bHasFocus))) return;

	m_bIsReloading = true;
	if (m_document)
	{
		m_document->RemoveEventListener("click", this, true);
		m_document->Close();
		m_document = nullptr;
		m_controls.clear();
		m_fastNames.clear();
	}

	initialiseMovie();
	handlePreReload();
	for (AUTO_VAR(it, m_controls.begin()); it != m_controls.end(); ++it)
		(*it)->ReInit();
	handleReload();

	m_needsCacheRendered = true;
	m_bIsReloading = false;
}

bool UIScene::needsReloaded()
{
	return !m_document && (!stealsFocus() || bHasFocus);
}

bool UIScene::hasMovie()
{
	return m_document != nullptr;
}

void UIScene::initialiseMovie()
{
	loadMovie();
	mapElementsAndNames();
	updateSafeZone();
	m_bUpdateOpacity = true;
}

bool UIScene::mapElementsAndNames()
{
	m_rootPath = nullptr; // Iggy root gone
	// Fast-names no longer needed — RmlUi uses string IDs directly
	m_funcRemoveObject = registerFastName(L"RemoveObject");
	m_funcSlideLeft    = registerFastName(L"SlideLeft");
	m_funcSlideRight   = registerFastName(L"SlideRight");
	m_funcSetSafeZone  = registerFastName(L"SetSafeZone");
	m_funcSetAlpha     = registerFastName(L"SetAlpha");
	m_funcSetFocus     = registerFastName(L"SetFocus");
	m_funcHorizontalResizeCheck = registerFastName(L"DoHorizontalResizeCheck");
	return true;
}

IggyName UIScene::registerFastName(const wstring &name)
{
	// Iggy fast-name registration is a no-op — return default
	IggyName var = {};
	return var;
}

// ---- Tick ----

void UIScene::tick()
{
	if (m_bIsReloading) return;
	if (m_hasTickedOnce) m_bCanHandleInput = true;

	tickTimers();
	for (AUTO_VAR(it, m_controls.begin()); it != m_controls.end(); ++it)
		(*it)->tick();

	m_hasTickedOnce = true;
}

// ---- Render ----

void UIScene::render(S32 width, S32 height, C4JRender::eViewportType viewport)
{
	// RmlUIManager handles rendering of all documents — nothing to do here
}

// ---- RmlUi EventListener ----

void UIScene::ProcessEvent(Rml::Event &event)
{
	Rml::Element *target = event.GetTargetElement();
	if (!target) return;

	if (event.GetId() == Rml::EventId::Click)
	{
		int controlId = target->GetAttribute<int>("data-control-id", -1);
		int childId   = target->GetAttribute<int>("data-child-id", 0);
		if (controlId >= 0)
			handlePress((F64)controlId, (F64)childId);
	}
	else if (event.GetId() == Rml::EventId::Change)
	{
		int controlId = target->GetAttribute<int>("data-control-id", -1);
		if (controlId >= 0)
		{
			Rml::String value = target->GetAttribute<Rml::String>("value", "");
			// route to appropriate handler based on element type
			handleSelectionChanged((F64)controlId);
		}
	}
}

// ---- Iggy external callback (legacy — only called if Iggy ever fires, which it doesn't) ----

void UIScene::externalCallback(IggyExternalFunctionCallUTF16 *call)
{
	if (!call) return;

	const wchar_t *fn = (const wchar_t *)call->function_name.string;

	if (wcscmp(fn, L"handlePress") == 0 && call->num_arguments == 2)
		handlePress(call->arguments[0].number, call->arguments[1].number);
	else if (wcscmp(fn, L"handleFocusChange") == 0 && call->num_arguments == 2)
		_handleFocusChange(call->arguments[0].number, call->arguments[1].number);
	else if (wcscmp(fn, L"handleInitFocus") == 0 && call->num_arguments == 2)
		_handleInitFocus(call->arguments[0].number, call->arguments[1].number);
	else if (wcscmp(fn, L"handleCheckboxToggled") == 0 && call->num_arguments == 2)
		handleCheckboxToggled(call->arguments[0].number, call->arguments[1].boolval);
	else if (wcscmp(fn, L"handleSliderMove") == 0 && call->num_arguments == 2)
		handleSliderMove(call->arguments[0].number, call->arguments[1].number);
	else if (wcscmp(fn, L"handleAnimationEnd") == 0)
		handleAnimationEnd();
	else if (wcscmp(fn, L"handleSelectionChanged") == 0 && call->num_arguments == 1)
		handleSelectionChanged(call->arguments[0].number);
	else if (wcscmp(fn, L"handleRequestMoreData") == 0)
	{
		if (call->num_arguments == 0) handleRequestMoreData(0, false);
		else if (call->num_arguments == 2) handleRequestMoreData(call->arguments[0].number, call->arguments[1].boolval);
	}
	else if (wcscmp(fn, L"handleTouchBoxRebuild") == 0)
		handleTouchBoxRebuild();
}

// ---- Safe zone (no-op when no Iggy movie) ----

F64 UIScene::getSafeZoneHalfHeight()
{
	float height = ui.getScreenHeight();
	float pct = (!RenderManager.IsHiDef() && RenderManager.IsWidescreen()) ? 0.075f : 0.05f;
	return height * pct;
}

F64 UIScene::getSafeZoneHalfWidth()
{
	float width = ui.getScreenWidth();
	float pct = 0.075f;
	return width * pct;
}

void UIScene::updateSafeZone()
{
	if (!m_document) return;

	F64 safeTop = 0, safeBottom = 0, safeLeft = 0, safeRight = 0;
	switch (m_parentLayer->getViewport())
	{
	case C4JRender::VIEWPORT_TYPE_SPLIT_TOP:         safeTop    = getSafeZoneHalfHeight(); break;
	case C4JRender::VIEWPORT_TYPE_SPLIT_BOTTOM:      safeBottom = getSafeZoneHalfHeight(); break;
	case C4JRender::VIEWPORT_TYPE_SPLIT_LEFT:        safeLeft   = getSafeZoneHalfWidth();  break;
	case C4JRender::VIEWPORT_TYPE_SPLIT_RIGHT:       safeRight  = getSafeZoneHalfWidth();  break;
	case C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
		safeTop = getSafeZoneHalfHeight(); safeLeft = getSafeZoneHalfWidth(); break;
	case C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
		safeTop = getSafeZoneHalfHeight(); safeRight = getSafeZoneHalfWidth(); break;
	case C4JRender::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
		safeBottom = getSafeZoneHalfHeight(); safeLeft = getSafeZoneHalfWidth(); break;
	case C4JRender::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
		safeBottom = getSafeZoneHalfHeight(); safeRight = getSafeZoneHalfWidth(); break;
	default:
		safeTop = safeBottom = getSafeZoneHalfHeight();
		safeLeft = safeRight = getSafeZoneHalfWidth();
		break;
	}

	// Apply as CSS padding on the document body
	Rml::Element *body = m_document->GetChild(0);
	if (body)
	{
		body->SetProperty("padding-top",    Rml::ToString((int)safeTop)    + "px");
		body->SetProperty("padding-bottom", Rml::ToString((int)safeBottom) + "px");
		body->SetProperty("padding-left",   Rml::ToString((int)safeLeft)   + "px");
		body->SetProperty("padding-right",  Rml::ToString((int)safeRight)  + "px");
	}
}

void UIScene::setSafeZone(S32 safeTop, S32 safeBottom, S32 safeLeft, S32 safeRight)
{
	// no-op: updateSafeZone() applies directly via CSS
}

// ---- Focus ----

void UIScene::gainFocus()
{
	if (!bHasFocus && stealsFocus())
	{
		bHasFocus = true;
		if (app.GetGameStarted() && needsReloaded())
			reloadMovie();

		updateTooltips();
		updateComponents();
		handleGainFocus(m_bFocussedOnce);
		if (bHasFocus) m_bFocussedOnce = true;
	}
	else if (bHasFocus && stealsFocus())
	{
		updateTooltips();
	}
}

void UIScene::loseFocus()
{
	if (bHasFocus)
	{
		bHasFocus = false;
		handleLoseFocus();
	}
}

void UIScene::handleGainFocus(bool navBack)
{
	InputManager.SetEnabledGtcButtons(this->getDefaultGtcButtons());
}

void UIScene::updateTooltips()
{
	ui.SetTooltips(m_iPad, -1);
}

// ---- Input ----

void UIScene::sendInputToMovie(int key, bool repeat, bool pressed, bool released)
{
	// Input is now handled directly in UIScene subclasses via handleInput()
}

bool UIScene::allowRepeat(int key)
{
	switch (key)
	{
	case ACTION_MENU_OK:
	case ACTION_MENU_CANCEL:
	case ACTION_MENU_A:
	case ACTION_MENU_B:
	case ACTION_MENU_X:
	case ACTION_MENU_Y:
		return false;
	}
	return true;
}

int UIScene::convertGameActionToIggyKeycode(int action) { return -1; }

// ---- Opacity / visibility ----

void UIScene::setOpacity(float percent)
{
	if (percent == m_lastOpacity && !m_bUpdateOpacity) return;
	m_lastOpacity = percent;
	m_bUpdateOpacity = false;

	if (m_document)
		m_document->SetProperty("opacity", Rml::ToString(percent));
}

void UIScene::setVisible(bool visible)
{
	m_bVisible = visible;
	if (m_document)
		m_document->SetProperty("visibility", visible ? "visible" : "hidden");
}

// ---- Navigation ----

void UIScene::navigateBack()
{
	ui.PlayUISFX(eSFX_Back);
	ui.NavigateBack(m_iPad);
	if (ui.GetTopScene(0))
		InputManager.SetEnabledGtcButtons(ui.GetTopScene(0)->getDefaultGtcButtons());
}

// ---- Controls (slide/remove) ----

UIControl *UIScene::GetMainPanel() { return nullptr; }

void UIScene::removeControl(UIControl_Base *control, bool centreScene)
{
	if (!m_document) return;
	Rml::Element *el = m_document->GetElementById(control->getControlName());
	if (el) el->SetProperty("display", "none");
}

void UIScene::slideLeft()  {}
void UIScene::slideRight() {}
void UIScene::doHorizontalResizeCheck() {}

void UIScene::customDraw(IggyCustomDrawCallbackRegion *region)
{
	app.DebugPrintf("Handling custom draw for scene with no override!\n");
}

// ---- Focus tracking ----

void UIScene::_handleFocusChange(F64 controlId, F64 childId)
{
	m_iFocusControl = (int)controlId;
	m_iFocusChild = (int)childId;
	handleFocusChange(controlId, childId);
	ui.PlayUISFX(eSFX_Focus);
}

void UIScene::_handleInitFocus(F64 controlId, F64 childId)
{
	m_iFocusControl = (int)controlId;
	m_iFocusChild = (int)childId;
	handleFocusChange(controlId, childId);
}

bool UIScene::controlHasFocus(int iControlId) { return m_iFocusControl == iControlId; }
bool UIScene::controlHasFocus(UIControl_Base *control) { return controlHasFocus(control->getId()); }
int  UIScene::getControlFocus()      { return m_iFocusControl; }
int  UIScene::getControlChildFocus() { return m_iFocusChild; }

void UIScene::setBackScene(UIScene *scene) { m_backScene = scene; }
UIScene *UIScene::getBackScene() { return m_backScene; }

// ---- Timers ----

void UIScene::addTimer(int id, int ms)
{
	int now = System::currentTimeMillis();
	TimerInfo info;
	info.running = true;
	info.duration = ms;
	info.targetTime = now + ms;
	m_timers[id] = info;
}

void UIScene::killTimer(int id)
{
	AUTO_VAR(it, m_timers.find(id));
	if (it != m_timers.end()) it->second.running = false;
}

void UIScene::tickTimers()
{
	int now = System::currentTimeMillis();
	for (AUTO_VAR(it, m_timers.begin()); it != m_timers.end();)
	{
		if (!it->second.running) { it = m_timers.erase(it); continue; }
		if (now > it->second.targetTime)
		{
			handleTimerComplete(it->first);
			it->second.targetTime = it->second.duration + now;
		}
		++it;
	}
}

// ---- Texture substitution ----

void UIScene::registerSubstitutionTexture(const wstring &textureName, PBYTE pbData, DWORD dwLength, bool deleteData)
{
	m_registeredTextures[textureName] = deleteData;
	ui.registerSubstitutionTexture(textureName, pbData, dwLength);
}

bool UIScene::hasRegisteredSubstitutionTexture(const wstring &textureName)
{
	return m_registeredTextures.find(textureName) != m_registeredTextures.end();
}

// ---- Callback IDs ----

size_t UIScene::GetCallbackUniqueId()
{
	if (m_callbackUniqueId == 0)
		m_callbackUniqueId = ui.RegisterForCallbackId(this);
	return m_callbackUniqueId;
}

bool UIScene::isReadyToDelete() { return true; }

// ---- Item slot custom draw (unchanged) ----

void UIScene::customDrawSlotControl(IggyCustomDrawCallbackRegion *region, int iPad, shared_ptr<ItemInstance> item, float fAlpha, bool isFoil, bool bDecorations)
{
	if (!item) return;

	if (m_cacheSlotRenders)
	{
		if ((m_cachedSlotDraw.size() + 1) == (size_t)m_expectedCachedSlotCount)
		{
			Minecraft *pMinecraft = Minecraft::GetInstance();
			shared_ptr<MultiplayerLocalPlayer> oldPlayer = pMinecraft->player;
			if (iPad >= 0 && iPad < XUSER_MAX_COUNT) pMinecraft->player = pMinecraft->localplayers[iPad];

			CustomDrawData *customDrawRegion = ui.calculateCustomDraw(region);
			ui.beginIggyCustomDraw4J(region, customDrawRegion);
			ui.setupCustomDrawGameState();

			int list = m_parentLayer->m_parentGroup->getCommandBufferList();
			m_needsCacheRendered = true;

			ui.setupCustomDrawMatrices(this, customDrawRegion);
			_customDrawSlotControl(customDrawRegion, iPad, item, fAlpha, isFoil, bDecorations, true);
			delete customDrawRegion;

			for (AUTO_VAR(it, m_cachedSlotDraw.begin()); it != m_cachedSlotDraw.end(); ++it)
			{
				CachedSlotDrawData *d = *it;
				ui.setupCustomDrawMatrices(this, d->customDrawRegion);
				_customDrawSlotControl(d->customDrawRegion, iPad, d->item, d->fAlpha, d->isFoil, d->bDecorations, true);
				delete d->customDrawRegion;
				delete d;
			}
			RenderManager.CBuffEnd();
			m_cachedSlotDraw.clear();
			RenderManager.CBuffCall(list);
			ui.endCustomDraw(region);
			pMinecraft->player = oldPlayer;
		}
		else
		{
			CachedSlotDrawData *d = new CachedSlotDrawData();
			d->item = item; d->fAlpha = fAlpha; d->isFoil = isFoil; d->bDecorations = bDecorations;
			d->customDrawRegion = ui.calculateCustomDraw(region);
			m_cachedSlotDraw.push_back(d);
		}
	}
	else
	{
		CustomDrawData *customDrawRegion = ui.setupCustomDraw(this, region);
		Minecraft *pMinecraft = Minecraft::GetInstance();
		shared_ptr<MultiplayerLocalPlayer> oldPlayer = pMinecraft->player;
		if (iPad >= 0 && iPad < XUSER_MAX_COUNT) pMinecraft->player = pMinecraft->localplayers[iPad];
		_customDrawSlotControl(customDrawRegion, iPad, item, fAlpha, isFoil, bDecorations, false);
		delete customDrawRegion;
		pMinecraft->player = oldPlayer;
		ui.endCustomDraw(region);
	}
}

void UIScene::_customDrawSlotControl(CustomDrawData *region, int iPad, shared_ptr<ItemInstance> item, float fAlpha, bool isFoil, bool bDecorations, bool usingCommandBuffer)
{
	Minecraft *pMinecraft = Minecraft::GetInstance();
	float bwidth = region->x1 - region->x0;
	float bheight = region->y1 - region->y0;
	float x = region->x0, y = region->y0;
	float scaleX = bwidth / 16.0f, scaleY = bheight / 16.0f;

	glEnable(GL_RESCALE_NORMAL);
	glPushMatrix();
	glRotatef(120, 1, 0, 0);
	Lighting::turnOn();
	glPopMatrix();

	float pop = item->popTime;
	if (pop > 0)
	{
		glPushMatrix();
		float squeeze = 1 + pop / (float)Inventory::POP_TIME_DURATION;
		glTranslatef(x + 8 * scaleX, y + 12 * scaleY, 0);
		glScalef(1 / squeeze, (squeeze + 1) / 2, 1);
		glTranslatef(-(x + 8 * scaleX), -(y + 12 * scaleY), 0);
	}

	if (m_pItemRenderer == nullptr) m_pItemRenderer = new ItemRenderer();
	m_pItemRenderer->renderAndDecorateItem(pMinecraft->font, pMinecraft->textures, item, x, y, scaleX, scaleY, fAlpha, isFoil, false, !usingCommandBuffer);

	if (pop > 0) glPopMatrix();

	if (bDecorations)
	{
		if (scaleX != 1.0f || scaleY != 1.0f)
		{
			glPushMatrix();
			glScalef(scaleX, scaleY, 1.0f);
			m_pItemRenderer->renderGuiItemDecorations(pMinecraft->font, pMinecraft->textures, item, (int)(0.5f + x / scaleX), (int)(0.5f + y / scaleY), fAlpha);
			glPopMatrix();
		}
		else
		{
			m_pItemRenderer->renderGuiItemDecorations(pMinecraft->font, pMinecraft->textures, item, (int)x, (int)y, fAlpha);
		}
	}

	Lighting::turnOff();
	glDisable(GL_RESCALE_NORMAL);
}
