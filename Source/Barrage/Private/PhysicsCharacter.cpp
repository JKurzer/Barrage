﻿#include "PhysicsCharacter.h"

JPH::BodyID JOLT::FBCharacter::Create(CharacterVsCharacterCollision* CVCColliderSystem)
{
	BodyID ret = BodyID();
	// Create capsule
	if(World) 
	{
		//WorldSimOwner manages the lifecycle of the physics characters. we don't have a proper destructor in here yet, I'm just trying to get this UP for now.
		if(World)
		{
			mCharacterSettings.mMass = 1000;
			Ref<Shape> capsule = new CapsuleShape(0.5f * mHeightStanding, mRadiusStanding);
			Ref<Shape> capsuleB = new CapsuleShape(0.5f * mHeightStanding, mRadiusStanding);
			mCharacterSettings.mShape = RotatedTranslatedShapeSettings(
				Vec3(0, 0.5f * mHeightStanding + mRadiusStanding, 0), Quat::sIdentity(), capsule).Create().Get();
			// Configure supporting volume
			mCharacterSettings.mSupportingVolume = Plane(Vec3::sAxisY(), -mHeightStanding);
			mForcesUpdate = Vec3::sZero();
			// Accept contacts that touch the lower sphere of the capsule
			// If you want to create character WITH innerbodyshape - don't try to reduce, reuse, or recycle here.
			InnerStandingShape =  RotatedTranslatedShapeSettings(
				Vec3(0, 0.5f * mHeightStanding + mRadiusStanding, 0), Quat::sIdentity(), capsuleB).Create().Get();

			//TODO: 12/10/24
			//uncomment this when we update jolt and inner body management is simplified.
			//mCharacterSettings.mInnerBodyShape = InnerStandingShape;
			//mCharacterSettings.mInnerBodyLayer = Layers::MOVING;

			mCharacter = new CharacterVirtual(&mCharacterSettings, mInitialPosition, Quat::sIdentity(), 0, World.Get());
			//mCharacter->SetListener(this);
			mCharacter->SetCharacterVsCharacterCollision(CVCColliderSystem); // see https://github.com/jrouwe/JoltPhysics/blob/e3ed3b1d33f3a0e7195fbac8b45b30f0a5c8a55b/UnitTests/Physics/CharacterVirtualTests.cpp#L759
			mEffectiveVelocity = Vec3::sZero();
			ret = mCharacter->GetInnerBodyID(); //I am going to regret this somehow. Update: I did.
		}
	}
	return ret;
}

void JOLT::FBCharacter::StepCharacter()
{
	// Determine new basic velocity
	Vec3 MyVelo = mCharacter->GetLinearVelocity();
	Vec3 current_vertical_velocity = Vec3(0, MyVelo.GetY(), 0);
	Vec3 current_planar_velocity = Vec3(MyVelo.GetX(), 0, MyVelo.GetZ());
	Vec3 ground_velocity = mCharacter->GetGroundVelocity();
	Vec3 new_velocity = mForcesUpdate;
	mForcesUpdate = Vec3::sZero();
	//the original design used ground velocity but because of our queuing design, we actually may often be in a state where
	//things are known good but we don't have a ground velocity yet.
	
	//this ensures small forces won't knock us off the ground, vastly reducing jitter.
	if (mCharacter->GetGroundState() == CharacterVirtual::EGroundState::OnGround // If on ground
		&& (current_vertical_velocity.GetY() - ground_velocity.GetY()) < 0.3f)
	// And not moving away from ground
	{
		
		if(ground_velocity.IsNearZero())
		{//during initial settling, we want to maintain velocity.
			new_velocity += current_planar_velocity;
		}
		else
		{// Assume ground velocity when on ground for multiple ticks.
			new_velocity += ground_velocity;
		}
	}
	else
	{
		//allow total air control.
		new_velocity += current_planar_velocity + current_vertical_velocity;
	}

	// Gravity
	if(World)
	{
		new_velocity += World->GetGravity() * mDeltaTime;
	}
	// Player input
	//todo: I think this is wrong now.
	new_velocity += mVelocityUpdate;
	// this doesn't actually keep you exactly to the max speed, instead it just keeps you to a ratio of max speed in each component.
	auto clamped = (new_velocity.Normalized() * mMaxSpeed).Abs(); 
	clamped = clamped.sClamp(new_velocity, -clamped, clamped);
	// Update character velocity
	mCharacter->SetLinearVelocity(clamped);

	RVec3 start_pos = GetPosition();

	// Update the character position
	TempAllocatorMalloc allocator;
	mCharacter->ExtendedUpdate(mDeltaTime,
	                           World->GetGravity(),
	                           mUpdateSettings,
	                           World->GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
	                           World->GetDefaultLayerFilter(Layers::MOVING),
	                           IgnoreSingleBodyFilter(mCharacter->GetInnerBodyID()),
	                           {},
	                           allocator);

	// Calculate effective velocity in this step
	mEffectiveVelocity = Vec3(GetPosition() - start_pos) / mDeltaTime;
}

void JOLT::FBCharacter::IngestUpdate(FBPhysicsInput& input)
{
	if (input.Action == PhysicsInputType::Rotation)
	{
		mCapsuleRotationUpdate = input.State;
	}
	else if (input.Action == PhysicsInputType::OtherForce)
	{
		// TODO: IDK this is a kludge for now since we removed the 100.0 divide by in CoordinateUtils::ToBarrageForce
		mForcesUpdate += input.State.GetXYZ() / 100.0;
	}
	else if (input.Action == PhysicsInputType::Velocity || input.Action == PhysicsInputType::SelfMovement)
	{
		mVelocityUpdate += input.State.GetXYZ();
	}
			
}
