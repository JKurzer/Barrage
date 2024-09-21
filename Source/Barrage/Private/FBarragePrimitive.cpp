﻿#include "FBarragePrimitive.h"
#include "BarrageDispatch.h"
#include "FBPhysicsInput.h"
#include "CoordinateUtils.h"
#include "FWorldSimOwner.h"
//this is a long way to go to keep the types from leaking but I think it's probably worth it.

//don't add inline. don't do it!
FBarragePrimitive::~FBarragePrimitive()
{
	//THIS WILL NOT HAPPEN UNTIL THE TOMBSTONE HAS EXPIRED AND THE LAST SHARED POINTER TO THE PRIMITIVE IS RELEASED.
	//Only the CleanTombs function in dispatch actually releases the shared pointer on the dispatch side
	//but an actor might hold a shared pointer to the primitive that represents it after that primitive has been
	//popped out of this.
	if (GlobalBarrage != nullptr && Me != Character) //TODO: This prevented the double free but we may now not free at all?
	{
		GlobalBarrage->FinalizeReleasePrimitive(KeyIntoBarrage);
	}
}

//-----------------
//the copy ops on call are intentional. they create a hold-open ref that allows us to be a little safer in many circumstances.
//once this thing is totally cleaned up, we can see about removing them and refactoring this.
//-----------------

void FBarragePrimitive::ApplyRotation(FQuat4d Rotator, FBLet Target)
{
	if (IsNotNull(Target))
	{
		if (GlobalBarrage)
		{
			auto HoldOpenGameSim = GlobalBarrage->JoltGameSim;
			if (HoldOpenGameSim && MyBARRAGEIndex < ALLOWED_THREADS_FOR_BARRAGE_PHYSICS)
			{
				HoldOpenGameSim->ThreadAcc[MyBARRAGEIndex].Queue->Enqueue(
					FBPhysicsInput(Target, 0, PhysicsInputType::Rotation,
					               CoordinateUtils::ToBarrageRotation(Rotator)
					)
				);
			}
		}
	}
}

//generally, this should be called from the same thread as update.
bool FBarragePrimitive::TryUpdateTransformFromJolt(FBLet Target, uint64 Time)
{
	if (IsNotNull(Target))
	{
		if (GlobalBarrage)
		{
			auto GameSimHoldOpen = GlobalBarrage->JoltGameSim;
			if (GameSimHoldOpen)
			{
				auto bID = GameSimHoldOpen->BarrageToJoltMapping->Find(Target->KeyIntoBarrage);
				if (bID)
				{
					//atm, characters cannot be inactive.
					if (GameSimHoldOpen->body_interface->IsActive(*bID) || Target->Me == FBShape::Character)
					{
						auto HoldOpen = GlobalBarrage->GameTransformPump;

						//TODO: @Eliza, can we figure out if updating the transforms in place is threadsafe? that'd be vastly preferable
						//TODO: figure out how to make this less.... horrid.
						if (HoldOpen)
						{
							HoldOpen->Enqueue(TransformUpdate(
								Target->KeyOutOfBarrage,
								Time,
								CoordinateUtils::FromJoltRotation(GameSimHoldOpen->body_interface->GetRotation(*bID)),
								CoordinateUtils::FromJoltCoordinates(
									GameSimHoldOpen->body_interface->GetPosition(*bID)),
									0
							));
						}
					}
					return true;
				}
			}
		}
	}
	return false;
}

FVector3f FBarragePrimitive::GetCentroidPossiblyStale(FBLet Target)
{
	//GlobalBarrage	
	if (IsNotNull(Target))
	{
		if (GlobalBarrage)
		{
			auto GameSimHoldOpen = GlobalBarrage->JoltGameSim;
			if (GameSimHoldOpen)
			{
				auto bID = GameSimHoldOpen->BarrageToJoltMapping->Find(Target->KeyIntoBarrage);
				if (bID)
				{
					if (GameSimHoldOpen->body_interface->IsActive(*bID))
					{
						return CoordinateUtils::FromJoltCoordinates(GameSimHoldOpen->body_interface->GetCenterOfMassPosition(*bID));
					}
				}
			}
		}
	}
	return FVector3f::ZeroVector;
}

FVector3f FBarragePrimitive::GetVelocityDirection(FBLet Target)
{
	if (IsNotNull(Target))
	{
		if (GlobalBarrage)
		{
			auto HoldOpenGameSim = GlobalBarrage->JoltGameSim;
			if (HoldOpenGameSim
				&& IsNotNull(Target)
				&& MyBARRAGEIndex < ALLOWED_THREADS_FOR_BARRAGE_PHYSICS)
			{
				auto bID = HoldOpenGameSim->BarrageToJoltMapping->Find(Target->KeyIntoBarrage);
				if(bID && Target->Me != FBShape::Character)
				{
					auto velo = HoldOpenGameSim->body_interface->GetLinearVelocity(*bID);
					return CoordinateUtils::FromJoltCoordinates(velo);
				}
				if(bID && Target->Me == FBShape::Character)
				{
					auto CharacterActual = HoldOpenGameSim->CharacterToJoltMapping->Find(Target->KeyIntoBarrage);
					if(CharacterActual && *CharacterActual)
					{
						return CoordinateUtils::FromJoltCoordinates(CharacterActual->Get()->mEffectiveVelocity);
					}
				}
			}
		}
	}
	return FVector3f();
}

void FBarragePrimitive::ApplyForce(FVector3d Force, FBLet Target)
{
	if (IsNotNull(Target))
	{
		if (GlobalBarrage)
		{
			
			auto HoldOpenGameSim = GlobalBarrage->JoltGameSim;
			if (HoldOpenGameSim && MyBARRAGEIndex < ALLOWED_THREADS_FOR_BARRAGE_PHYSICS)
			{
				HoldOpenGameSim->ThreadAcc[MyBARRAGEIndex].Queue->Enqueue(
					FBPhysicsInput(Target, 0, PhysicsInputType::OtherForce,
					               CoordinateUtils::ToBarrageForce(Force)
					)
				);
			}
		}
	}
}
