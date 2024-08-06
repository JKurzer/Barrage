#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Containers/TripleBuffer.h"
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
		TStatId GetStatId() const override;
		TSharedPtr<FWorldSimOwner> JoltGameSim;
	};