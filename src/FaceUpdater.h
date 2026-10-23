#pragma once

// Applies the face tracking mappings to the player's face from the game thread, by hooking
// PlayerCharacter::Update. Updates run every Settings::faceUpdateInterval frames.
namespace FaceUpdater
{
	void Install();
}
