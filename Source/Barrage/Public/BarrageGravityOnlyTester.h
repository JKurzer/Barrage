// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CoreTypeKeys.h"
#include "FBarragePrimitive.h"
#include "Components/ActorComponent.h"
#include "BarrageGravityOnlyTester.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BARRAGE_API UBarrageGravityOnlyTester : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBarrageGravityOnlyTester();
	FBLet MyBarrageBody;
	ObjectKey MyObjectKey;
	bool IsReady = false;

	virtual void BeforeBeginPlay(ObjectKey TransformOwner);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
