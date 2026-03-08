#pragma once
#include "Input.h"
#include "KeyboardMouseInput.h"

// Keyboard & Mouse player input handler.
// The base Input::tick() already handles KBM input inline alongside gamepad.
// This subclass provides a standalone KBM-only input path that can be used
// when the game is running in KBM-exclusive mode.
class KeyboardMousePlayerInput : public Input
{
public:
	virtual void tick(LocalPlayer *player) override;
};
