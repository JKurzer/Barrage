#pragma once

#include "CoreMinimal.h"
#include "FWorldSimOwner.h"
#include "IsolatedJoltIncludes.h"

//based on
//https://github.com/jrouwe/JoltPhysics/blob/master/UnitTests/Physics/CharacterVirtualTests.cpp
//https://github.com/jrouwe/JoltPhysics/blob/master/Samples/Tests/Character/CharacterBaseTest.cpp
//https://github.com/jrouwe/JoltPhysics/blob/master/Samples/Tests/Character/CharacterTest.cpp
//https://github.com/jrouwe/JoltPhysics/blob/master/Samples/Tests/Character/CharacterVirtualTest.cpp
//and more generally: https://github.com/jrouwe/JoltPhysics/blob/master/UnitTests/Physics/
//Thanks again to jrouwe for such an excellent lib.
namespace JOLT
{
	// this will get hidden in the cpp prolly to some extent.
	// you might see some use of jolt coding conventions when working in the barrage codebase.

	class FBCharacter
	{
	public:
		JPH::RVec3 mInitialPosition = JPH::RVec3::sZero();
		float mHeightStanding = 1.35f;
		float mRadiusStanding = 0.3f;
		JPH::CharacterVirtualSettings mCharacterSettings;
		CharacterVirtual::ExtendedUpdateSettings mUpdateSettings;
		// Accumulated during IngestUpdate
		Vec3 mVelocityUpdate = Vec3::sZero();
		Vec3 mForcesUpdate = Vec3::sZero();
		JPH::Quat mCapsuleRotationUpdate = JPH::Quat::sIdentity();
		Ref<CharacterVirtual> mCharacter;
		float mDeltaTime;

		// Calculated effective velocity after a step
		Vec3 mEffectiveVelocity = Vec3::sZero();

	protected:
		friend class FWorldSimOwner;
		TWeakPtr<FWorldSimOwner> Machine;
		TSharedPtr<JPH::PhysicsSystem, ESPMode::ThreadSafe> World;

	public:
		RVec3 GetPosition() const
		{
			return mCharacter->GetPosition();
		}

		// Create the character
		void Create()
		{
			// Create capsule
			auto HoldOpen = Machine.Pin();
			if(HoldOpen)
			{
				World = HoldOpen->physics_system; // this MAY lead to a circ ref?  I think I've avoided it using the weak ptr to FWorldSimOwner
				//WorldSimOwner manages the lifecycle of the physics characters. we don't have a proper destructor in here yet, I'm just trying to get this UP for now.
				if(World)
				{
					Ref<Shape> capsule = new CapsuleShape(0.5f * mHeightStanding, mRadiusStanding);
					mCharacterSettings.mShape = RotatedTranslatedShapeSettings(
						Vec3(0, 0.5f * mHeightStanding + mRadiusStanding, 0), Quat::sIdentity(), capsule).Create().Get();

					// Configure supporting volume
					mCharacterSettings.mSupportingVolume = Plane(Vec3::sAxisY(), -mHeightStanding);
					// Accept contacts that touch the lower sphere of the capsule

					// Create character
					mCharacter = new CharacterVirtual(&mCharacterSettings, mInitialPosition, Quat::sIdentity(), 0, World.Get());
					//mCharacter->SetListener(this); 
					mCharacter->SetCharacterVsCharacterCollision(&HoldOpen->CharacterVsCharacterCollisionSimple); // see https://github.com/jrouwe/JoltPhysics/blob/e3ed3b1d33f3a0e7195fbac8b45b30f0a5c8a55b/UnitTests/Physics/CharacterVirtualTests.cpp#L759
					HoldOpen->CharacterToJoltMapping->Get()->Add(mCharacter->GetInnerBodyID(), this); //I am going to regret this somehow.
				}
			}
		}
		void StepCharacter()
		{
			// Determine new basic velocity
			Vec3 current_vertical_velocity = Vec3(0, mCharacter->GetLinearVelocity().GetY(), 0);
			Vec3 ground_velocity = mCharacter->GetGroundVelocity();
			Vec3 new_velocity = mForcesUpdate;

			//this ensures small forces won't knock us off the ground, vastly reducing jitter.
			if (mCharacter->GetGroundState() == CharacterVirtual::EGroundState::OnGround // If on ground
				&& (current_vertical_velocity.GetY() - ground_velocity.GetY()) < 0.3f)
			// And not moving away from ground
			{
				// Assume velocity of ground when on ground
				new_velocity += ground_velocity;
			}
			else
			{
				new_velocity += ground_velocity + current_vertical_velocity;
			}

			// Gravity
			new_velocity += World->GetGravity() * mDeltaTime;

			// Player input
			new_velocity += mVelocityUpdate;

			// Update character velocity
			mCharacter->SetLinearVelocity(new_velocity);

			RVec3 start_pos = GetPosition();

			// Update the character position
			TempAllocatorMalloc allocator;
			mCharacter->ExtendedUpdate(mDeltaTime,
			                           World->GetGravity(),
			                           mUpdateSettings,
			                           World->GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
			                           World->GetDefaultLayerFilter(Layers::MOVING),
			                           {},
			                           {},
			                           allocator);

			// Calculate effective velocity in this step
			mEffectiveVelocity = Vec3(GetPosition() - start_pos) / mDeltaTime;
		}


		//To prevent cheeky bullshit and maximize the value we get from the queuing we already do, this
		// should likely be called during step update OR during the locomotion step
		//it should definitely run on the busy worker.
		void IngestUpdate(FBPhysicsInput& input)
		{
			if (input.Action == PhysicsInputType::Rotation)
			{
				mCapsuleRotationUpdate = input.State;
			}
			else if (input.Action == PhysicsInputType::OtherForce)
			{
				mForcesUpdate += input.State.GetXYZ();
			}
			else if (input.Action == PhysicsInputType::Velocity || input.Action == PhysicsInputType::SelfMovement)
			{
				mVelocityUpdate += input.State.GetXYZ();
			}
			
		}
	};
}
