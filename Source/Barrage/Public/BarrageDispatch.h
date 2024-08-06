#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Containers/TripleBuffer.h"
#include "FBarrageKey.h"
#include "FBarragePrimitive.h"
#include "BarrageDispatch.generated.h"

	class FWorldSimOwner;

	UCLASS()
	class BARRAGE_API UBarrageDispatch : public UTickableWorldSubsystem
	{
		GENERATED_BODY()
		static inline constexpr float TickRateInDelta = 1.0 /120.0;
	public:
		UBarrageDispatch();
		virtual ~UBarrageDispatch() override;
		virtual void Initialize(FSubsystemCollectionBase& Collection) override;
		virtual void OnWorldBeginPlay(UWorld& InWorld) override;
		virtual void Deinitialize() override;

		//we allow you to start with radius A and end with radius B for your sphere casts. this is useful for faking certain kinds of hit behavior.
		//This is actually implemented by shooting a few spheres, so don't use it unless you need it. -1 turns this off. I refuse to do negaspheres.
		virtual void SphereCast(double Radius, FVector3d CastFrom, double FinalRadius = -1, uint64_t timestamp = 0);
		FBarrageKey CreateSimPrimitive(FBShapeParams& Definition);
		TSharedPtr<FBlet> GetShapeRef(FBarrageKey Existing);
		

		virtual TStatId GetStatId() const override;
		TSharedPtr<FWorldSimOwner> JoltGameSim;
		//ONLY call this from a thread OTHER than gamethread, or you will experience untold sorrow.
		void StepWorld();
		//in case we want the actor to directly manage the lifecycle rather than the barrage
		//ecs, which I think is unlikely but I want to design so that the semantic is possible to add.
		TPair<FBarrageKey, TSharedPtr<FBlet>> BindActor(FBShapeParams& Definition);
	};