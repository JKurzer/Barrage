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
		
		virtual void SphereCast();
		FBarrageKey CreateSimPrimitive(FBShapeParams& Definition);
		TSharedPtr<FBlet> GetShapeRef(FBarrageKey Existing);
		
		TStatId GetStatId() const override;
		TSharedPtr<FWorldSimOwner> JoltGameSim;

	protected:
		//in case we want the actor to directly manage the lifecycle rather than the barrage
		//ecs, which I think is unlikely but I want to design so that the semantic is possible to add.
		TPair<FBarrageKey, TSharedPtr<FBlet>> BindActor(FBShapeParams& Definition);
	};