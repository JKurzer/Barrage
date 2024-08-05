#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Containers/TripleBuffer.h"
PRAGMA_PUSH_PLATFORM_DEFAULT_PACKING

#include "Jolt/Jolt.h"
#include "Jolt/RegisterTypes.h"
#include "Jolt/Core/Factory.h"
#include "Jolt/Physics/PhysicsSettings.h"
#include "Jolt/Physics/PhysicsSystem.h"
PRAGMA_POP_PLATFORM_DEFAULT_PACKING

#include "BarrageDispatch.generated.h"
namespace Barrage
{
	UCLASS()
	class BARRAGE_API UBarrageDispatch : public UTickableWorldSubsystem
	{
	
		GENERATED_BODY()
		// All Jolt symbols are in the JPH namespace
		using namespace JPH;

		// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
		using namespace JPH::literals;
	public:
		UBarrageDispatch();
		virtual ~UBarrageDispatch() override;
		virtual void Initialize(FSubsystemCollectionBase& Collection) override;
		virtual void OnWorldBeginPlay(UWorld& InWorld) override;
		virtual void Deinitialize() override;
	private:

		PhysicsSystem physics_system;
	};
}