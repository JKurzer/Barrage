#include "BarrageDispatch.h"


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
	}

	void UBarrageDispatch::Deinitialize()
	{
		Super::Deinitialize();

	}

TStatId UBarrageDispatch::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBarrageDispatch, STATGROUP_Tickables);
}