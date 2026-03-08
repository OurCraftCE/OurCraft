#include "stdafx.h"
#include "KeyboardMousePlayerInput.h"
#include "../OurCraft.h"
#include "../Game/GameMode.h"
#include "..\World\FwdHeaders/net.minecraft.world.entity.player.h"
#include "../Game/LocalPlayer.h"
#include "../Game/Options.h"
#include "KeyboardMouseInput.h"

extern KeyboardMouseInput g_KBMInput;

void KeyboardMousePlayerInput::tick(LocalPlayer *player)
{
	Minecraft *pMinecraft = Minecraft::GetInstance();
	int iPad = player->GetXboxPad();

	xa = 0.0f;
	ya = 0.0f;

	// Movement from WASD
	if (pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_LEFT) || pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_RIGHT))
		xa = g_KBMInput.GetMoveX();

	if (pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_FORWARD) || pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_BACKWARD))
		ya = g_KBMInput.GetMoveY();

	// Sneaking (Left Shift - hold)
	if (!player->abilities.flying)
	{
		if (pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_SNEAK_TOGGLE))
			sneaking = g_KBMInput.IsKeyDown(KeyboardMouseInput::KEY_SNEAK);
	}

	if (sneaking)
	{
		xa *= 0.3f;
		ya *= 0.3f;
	}

	// Sprinting (Left Ctrl + forward)
	if (!player->abilities.flying)
	{
		bool ctrlHeld = g_KBMInput.IsKeyDown(KeyboardMouseInput::KEY_SPRINT);
		bool movingForward = (ya > 0.0f);
		sprinting = (ctrlHeld && movingForward);
	}
	else
	{
		sprinting = false;
	}

	// Mouse look
	float mouseSensitivity = ((float)app.GetGameSettings(iPad, eGameSetting_Sensitivity_InGame)) / 100.0f;
	float mouseLookScale = 5.0f;
	float turnX = g_KBMInput.GetLookX(mouseSensitivity * mouseLookScale);
	float turnY = g_KBMInput.GetLookY(mouseSensitivity * mouseLookScale);

	if (app.GetGameSettings(iPad, eGameSetting_ControlInvertLook))
	{
		turnY = -turnY;
	}

	player->interpolateTurn(turnX, turnY);

	// Jumping
	bool kbJump = g_KBMInput.IsKeyDown(KeyboardMouseInput::KEY_JUMP);
	if (kbJump && pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_JUMP))
		jumping = true;
	else
		jumping = false;

	// ESC to ungrab mouse
	if (g_KBMInput.IsKeyPressed(VK_ESCAPE) && g_KBMInput.IsMouseGrabbed())
	{
		g_KBMInput.SetMouseGrabbed(false);
	}
}
