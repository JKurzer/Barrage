// Fill out your copyright notice in the Description page of Project Settings.


#include "BarrageGravityOnlyTester.h"

#include "BarrageDispatch.h"
#include "FWorldSimOwner.h"
#include "KeyCarry.h"

// Sets default values for this component's properties
UBarrageGravityOnlyTester::UBarrageGravityOnlyTester()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	MyObjectKey = 0;
	
}

void UBarrageGravityOnlyTester::BeforeBeginPlay(ObjectKey TransformOwner)
{
	MyObjectKey = TransformOwner;
	IsReady = true;
}

// Called when the game starts
void UBarrageGravityOnlyTester::BeginPlay()
{
	Super::BeginPlay();
	if(!IsReady)
	{
		MyObjectKey = SKELETON::Key(this);
		IsReady = true;
	}
	if(!IsDefaultSubobject() && MyObjectKey != 0)
	{
		auto Physics =  GetWorld()->GetSubsystem<UBarrageDispatch>();
		auto params = FBarrageBounder::GenerateBoxBounds(GetOwner()->GetActorLocation(), 2, 2 ,2);
		MyBarrageBody = Physics->CreatePrimitive(params, MyObjectKey, Layers::MOVING);
	}
}


// Called every frame
void UBarrageGravityOnlyTester::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if(MyBarrageBody && FBarragePrimitive::IsNotNull(MyBarrageBody))
	{
		//nothing is required here. weird. real weird.
	}
	// ...
}

