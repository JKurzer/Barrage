#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Containers/TripleBuffer.h"

#include "BarrageDispatch.generated.h"

	UCLASS()
	class BARRAGE_API UBarrageDispatch : public UTickableWorldSubsystem
	{
		GENERATED_BODY()

	public:
		UBarrageDispatch();
		virtual ~UBarrageDispatch() override;
		virtual void Initialize(FSubsystemCollectionBase& Collection) override;
		virtual void OnWorldBeginPlay(UWorld& InWorld) override;
		virtual void Deinitialize() override;
		TStatId GetStatId() const override;
	};