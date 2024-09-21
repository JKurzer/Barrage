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
	// you might see some use of jolt coding conventions when working in the barrage codebase.

	class FBCharacter : public FBCharacterBase
	{
	public:
		RVec3 GetPosition() const
		{
			return mCharacter->GetPosition();
		}
		// Create the character
		// Fails if the mProperties are not correctly set.
		BodyID Create(CharacterVsCharacterCollision* CVCColliderSystem);
		virtual void StepCharacter() override;

		//To prevent cheeky bullshit and maximize the value we get from the queuing we already do, this
		// should likely be called during step update OR during the locomotion step
		//it should definitely run on the busy worker.
		virtual void IngestUpdate(FBPhysicsInput& input) override;
	};
}
