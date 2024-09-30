#pragma once

#include "EPhysicsLayer.generated.h"

// TODO: how do we enforce that the blueprint enum and the Jolt enum stay consistent?
UENUM(BlueprintType)
enum class EPhysicsLayer : uint8
{
	NON_MOVING UMETA(DisplayName="Static (Non-Moving)"),
	MOVING UMETA(DisplayName="Moving Entity"),
	HITBOX UMETA(DisplayName="Hitbox"),
	PROJECTILE UMETA(DisplayName="Projectile"),
	CAST_QUERY UMETA(DisplayName="Cast Query"),
	DEBRIS UMETA(DisplayName="Debris")
};

namespace Layers
{
	// Layer that objects can be in, determines which other objects it can collide with
	// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
	// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
	// but only if you do collision testing).
	enum JoltPhysicsLayer
	{
		NON_MOVING,
		MOVING,
		HITBOX,
		PROJECTILE,
		CAST_QUERY,
		DEBRIS,
		NUM_LAYERS
	};
}