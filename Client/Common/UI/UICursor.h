#pragma once

// Software cursor renderer for Bedrock-style mouse interaction.
// Renders an arrow/crosshair cursor on top of all UI when KBM is active
// and the mouse is not grabbed (i.e. player is in a menu).

class UICursor
{
public:
	static void Init();
	static void Shutdown();

	// Call at the very end of the frame, after all UI is rendered.
	// Draws the cursor at the current mouse position when appropriate.
	static void Render();

	// Returns true if the software cursor should be visible right now
	static bool IsVisible();

	// Cursor style
	enum ECursorStyle
	{
		eCursor_Arrow,
		eCursor_Crosshair,
		eCursor_Hand,
	};

	static void SetStyle(ECursorStyle style);
	static ECursorStyle GetStyle();

private:
	static ECursorStyle s_style;
	static bool s_initialized;

	static void RenderArrowCursor(float x, float y, float screenW, float screenH);
	static void RenderCrosshairCursor(float x, float y, float screenW, float screenH);

	// Draw a filled triangle (for arrow cursor)
	static void DrawFilledTriangle(float x0, float y0, float x1, float y1, float x2, float y2,
								   unsigned int color, float screenW, float screenH);
	// Draw a filled rect in screen coords
	static void DrawFilledRect(float x0, float y0, float x1, float y1,
							   unsigned int color, float screenW, float screenH);
};
