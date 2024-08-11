#pragma once
#include "BarrageDispatch.h"

PRAGMA_PUSH_PLATFORM_DEFAULT_PACKING
#include "Jolt/Jolt.h"
#include "Jolt/Math/Quat.h"
PRAGMA_POP_PLATFORM_DEFAULT_PACKING

enum PhysicsInputType
{
	SelfMovement,
	Velocity,
	OtherForce,
	Rotation,
		
};

struct FBPhysicsInput
{
		FBLet Target;
		uint32 Sequence;//unused, likely needed for determinism.
		PhysicsInputType Action;
		JPH::Quat State;
};