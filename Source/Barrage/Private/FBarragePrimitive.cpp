#include "FBarragePrimitive.h"
#include "BarrageDispatch.h"
#include "CoordinateUtils.h"
//this is a long way to go to keep the types from leaking but I think it's probably worth it.

//don't add inline. don't do it!
FBarragePrimitive::~FBarragePrimitive()
{
	//THIS WILL NOT HAPPEN UNTIL THE TOMBSTONE HAS EXPIRED AND THE LAST SHARED POINTER TO THE PRIMITIVE IS RELEASED.
	//Only the CleanTombs function in dispatch actually releases the shared pointer on the dispatch side
	//but an actor might hold a shared pointer to the primitive that represents it after that primitive has been
	//popped out of this.
	if(GlobalBarrage != nullptr)
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
	if(IsNotNull(Target))
	{
		//GlobalBarrage
	}
}

//generally, this should be called from the same thread as update.
template <typename OutKeyDispatch>
bool FBarragePrimitive::TryPublishTransformFromJolt(FBLet Target)
{
	if(IsNotNull(Target))
	{
		if(GlobalBarrage)
		{

		}
		
	}
	return false;
}

FVector3d FBarragePrimitive::GetCentroidPossiblyStale(FBLet Target)
{
	//GlobalBarrage	
	if(IsNotNull(Target))
	{
		return FVector3d();
	}
	return FVector3d::ZeroVector;
}

bool FBarragePrimitive::IsNotNull(FBLet Target)
{
	return Target != nullptr && Target.IsValid() && Target->tombstone == 0;
}

void FBarragePrimitive::ApplyForce(FVector3d Force, FBLet Target)
{
	//globalbarrage
}