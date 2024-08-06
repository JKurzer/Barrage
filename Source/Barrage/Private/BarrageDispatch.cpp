#include "BarrageDispatch.h"

#include "FWorldSimOwner.h"


UBarrageDispatch::UBarrageDispatch()
{
}

	UBarrageDispatch::~UBarrageDispatch() {
	}

	void UBarrageDispatch::Initialize(FSubsystemCollectionBase& Collection)
	{
		Super::Initialize(Collection);
	}

	void UBarrageDispatch::OnWorldBeginPlay(UWorld& InWorld)
	{
		Super::OnWorldBeginPlay(InWorld);
		JoltGameSim = MakeShareable(new FWorldSimOwner(TickRateInDelta));
	}

	void UBarrageDispatch::Deinitialize()
	{
		Super::Deinitialize();

	}

void UBarrageDispatch::SphereCast(double Radius, FVector3d CastFrom, double FinalRadius, uint64_t timestamp)
{
}

FBarrageKey UBarrageDispatch::CreateSimPrimitive(FBShapeParams& Definition)
{
	
	return FBarrageKey();
}

TSharedPtr<FBlet> UBarrageDispatch::GetShapeRef(FBarrageKey Existing)
{
	return TSharedPtr<FBlet>();
}


TStatId UBarrageDispatch::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBarrageDispatch, STATGROUP_Tickables);
}

void UBarrageDispatch::StepWorld()
{
	JoltGameSim->StepSimulation();
}
