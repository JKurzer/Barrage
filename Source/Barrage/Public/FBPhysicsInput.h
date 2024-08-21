#pragma once
#include "BarrageDispatch.h"

PRAGMA_PUSH_PLATFORM_DEFAULT_PACKING
#include "Jolt/Jolt.h"
#include "Jolt/Math/Quat.h"
#include "mimalloc.h"
PRAGMA_POP_PLATFORM_DEFAULT_PACKING

enum PhysicsInputType
{
	SelfMovement,
	Velocity,
	OtherForce,
	Rotation,
		
};


//this struct must be size 48.
struct FBPhysicsInput
{
		FBLet Target;
		uint64 Sequence;//unused, likely needed for determinism.
		PhysicsInputType Action;
		uint32 metadata;
		JPH::Quat State;
	
	explicit FBPhysicsInput()
	{
	//don't initialize anything. just trust me on this.	
	}

	FBPhysicsInput(FBLet Affected, int Seq, PhysicsInputType PhysicsInput, JPH::Quat Quat)
	{
		State = Quat;
		Target  = Affected;
		Sequence = Seq;
		Action = PhysicsInput;
	};

};
