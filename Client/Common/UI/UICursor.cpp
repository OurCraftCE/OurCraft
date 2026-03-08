#include "stdafx.h"
#include "UICursor.h"
#include "../../Input/KeyboardMouseInput.h"
#include "../../Render/Tesselator.h"
#include "..\..\OurCraft.h"
#include "../../UI/ScreenSizeCalculator.h"
#include "../../UI/Gui.h"

UICursor::ECursorStyle UICursor::s_style = eCursor_Arrow;
bool UICursor::s_initialized = false;

void UICursor::Init()
{
	s_style = eCursor_Arrow;
	s_initialized = true;
}

void UICursor::Shutdown()
{
	s_initialized = false;
}

bool UICursor::IsVisible()
{
	// Show cursor when KBM is active, mouse is not grabbed, and window is focused
	if (!g_KBMInput.IsKBMActive()) return false;
	if (g_KBMInput.IsMouseGrabbed()) return false;
	if (!g_KBMInput.IsWindowFocused()) return false;
	return true;
}

void UICursor::SetStyle(ECursorStyle style)
{
	s_style = style;
}

UICursor::ECursorStyle UICursor::GetStyle()
{
	return s_style;
}

void UICursor::Render()
{
	if (!s_initialized) return;
	if (!IsVisible()) return;

	Minecraft *mc = Minecraft::GetInstance();
	if (!mc) return;

	float screenW = (float)mc->width_phys;
	float screenH = (float)mc->height_phys;
	if (screenW <= 0 || screenH <= 0) return;

	float mouseX = (float)g_KBMInput.GetMouseX();
	float mouseY = (float)g_KBMInput.GetMouseY();

	// Setup orthographic projection matching physical screen pixels
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0, (double)screenW, (double)screenH, 0.0, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);

	switch (s_style)
	{
	case eCursor_Crosshair:
		RenderCrosshairCursor(mouseX, mouseY, screenW, screenH);
		break;
	case eCursor_Arrow:
	case eCursor_Hand:
	default:
		RenderArrowCursor(mouseX, mouseY, screenW, screenH);
		break;
	}

	// Restore matrices
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	// Re-enable state that other code expects
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

void UICursor::DrawFilledRect(float x0, float y0, float x1, float y1,
							  unsigned int color, float screenW, float screenH)
{
	float a = ((color >> 24) & 0xff) / 255.0f;
	float r = ((color >> 16) & 0xff) / 255.0f;
	float g = ((color >> 8) & 0xff) / 255.0f;
	float b = ((color) & 0xff) / 255.0f;

	Tesselator *t = Tesselator::getInstance();
	glColor4f(r, g, b, a);
	t->begin();
	t->vertex(x0, y1, 0.0f);
	t->vertex(x1, y1, 0.0f);
	t->vertex(x1, y0, 0.0f);
	t->vertex(x0, y0, 0.0f);
	t->end();
}

void UICursor::DrawFilledTriangle(float x0, float y0, float x1, float y1,
								  float x2, float y2, unsigned int color,
								  float screenW, float screenH)
{
	float a = ((color >> 24) & 0xff) / 255.0f;
	float r = ((color >> 16) & 0xff) / 255.0f;
	float g = ((color >> 8) & 0xff) / 255.0f;
	float b = ((color) & 0xff) / 255.0f;

	// Render as a degenerate quad (duplicate last vertex)
	Tesselator *t = Tesselator::getInstance();
	glColor4f(r, g, b, a);
	t->begin();
	t->vertex(x0, y0, 0.0f);
	t->vertex(x1, y1, 0.0f);
	t->vertex(x2, y2, 0.0f);
	t->vertex(x2, y2, 0.0f); // degenerate
	t->end();
}

void UICursor::RenderArrowCursor(float x, float y, float screenW, float screenH)
{
	// Bedrock-style arrow cursor
	// Scale cursor size based on screen resolution
	float scale = screenH / 720.0f;
	if (scale < 1.0f) scale = 1.0f;

	float cursorW = 12.0f * scale;
	float cursorH = 19.0f * scale;
	float innerW = 8.0f * scale;

	// Black outline - main arrow body
	DrawFilledTriangle(x, y,
					   x, y + cursorH,
					   x + cursorW, y + cursorH * 0.6f,
					   0xff000000, screenW, screenH);

	// White fill - inner arrow
	float inset = 1.5f * scale;
	DrawFilledTriangle(x + inset, y + inset * 2.0f,
					   x + inset, y + cursorH - inset * 2.0f,
					   x + innerW, y + cursorH * 0.6f,
					   0xffffffff, screenW, screenH);

	// Arrow stem outline (black)
	float stemTop = y + cursorH * 0.45f;
	float stemBot = y + cursorH;
	float stemLeft = x + 1.0f * scale;
	float stemRight = x + 5.0f * scale;
	DrawFilledRect(stemLeft - 1.0f * scale, stemTop, stemRight + 1.0f * scale, stemBot,
				   0xff000000, screenW, screenH);

	// Arrow stem fill (white)
	DrawFilledRect(stemLeft, stemTop + 1.0f * scale, stemRight, stemBot - 1.0f * scale,
				   0xffffffff, screenW, screenH);
}

void UICursor::RenderCrosshairCursor(float x, float y, float screenW, float screenH)
{
	// Simple crosshair for container/inventory screens
	float scale = screenH / 720.0f;
	if (scale < 1.0f) scale = 1.0f;

	float size = 8.0f * scale;
	float thick = 1.5f * scale;

	// White crosshair with black outline
	// Horizontal bar outline
	DrawFilledRect(x - size - 1, y - thick - 1, x + size + 1, y + thick + 1,
				   0xff000000, screenW, screenH);
	// Vertical bar outline
	DrawFilledRect(x - thick - 1, y - size - 1, x + thick + 1, y + size + 1,
				   0xff000000, screenW, screenH);
	// Horizontal bar
	DrawFilledRect(x - size, y - thick, x + size, y + thick,
				   0xffffffff, screenW, screenH);
	// Vertical bar
	DrawFilledRect(x - thick, y - size, x + thick, y + size,
				   0xffffffff, screenW, screenH);
}
