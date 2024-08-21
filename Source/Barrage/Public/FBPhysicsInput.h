#pragma once
#include "IsolatedJoltIncludes.h"


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
	
	explicit FBPhysicsInput(): Sequence(0), Action(), metadata(0), State()
		{
			//don't initialize anything. just trust me on this.	
		}

		FBPhysicsInput(FBLet Affected, int Seq, PhysicsInputType PhysicsInput, JPH::Quat Quat)
	{
		State = Quat;
		Target  = Affected;
		Sequence = Seq;
		Action = PhysicsInput;
		metadata = 0;
	};

};
