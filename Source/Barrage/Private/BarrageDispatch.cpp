#include "BarrageDispatch.h"
#include "FWorldSimOwner.h"

	UBarrageDispatch::UBarrageDispatch()
	{
		
		
		
	}

	UBarrageDispatch::~UBarrageDispatch() {
	//now that all primitives are destructed
	GlobalBarrage = nullptr;
	}

	void UBarrageDispatch::Initialize(FSubsystemCollectionBase& Collection)
	{
		Super::Initialize(Collection);
		for(auto& x : Tombs)
		{
			x = MakeShareable(new TArray<FBLet>());
		}
	GlobalBarrage = this;
	}

	void UBarrageDispatch::OnWorldBeginPlay(UWorld& InWorld)
	{
		Super::OnWorldBeginPlay(InWorld);
		JoltGameSim = MakeShareable(new FWorldSimOwner(TickRateInDelta));
		MasterRecordForLifecycle = MakeShareable(new TMap<FBarrageKey, FBLet>());
	}

	void UBarrageDispatch::Deinitialize()
	{
		Super::Deinitialize();
		MasterRecordForLifecycle = nullptr;
		for(auto& x : Tombs)
		{
			x = nullptr;
		}

	}

void UBarrageDispatch::SphereCast(double Radius, FVector3d CastFrom, uint64_t timestamp)
{
}

//this method exists to allow us to hide the types for JPH by including them in the CPP rather than the .h
// which is why the bounder and letter generate structs consumed by dispatch.
FBLet UBarrageDispatch::CreateSimPrimitive(FBShapeParams& Definition, uint64 OutKey)
{
	auto temp = JoltGameSim->CreatePrimitive(Definition);
	FBLet indirect = MakeShareable(new FBarragePrimitive(temp, OutKey));
	indirect->Me = Definition.MyShape;
	MasterRecordForLifecycle->Add(indirect->KeyIntoBarrage, indirect);
	return indirect;
}

//unlike our other ecs components in artillery, barrage dispatch does not maintain the mappings directly.
//this is because we may have _many_ jolt sims running if we choose to do deterministic rollback in certain ways.
//This is a copy by value return on purpose, as we want the ref count to rise.
FBLet UBarrageDispatch::GetShapeRef(FBarrageKey Existing)
{
		//SharedPTR's def val is nullptr. this will return nullptr as soon as entomb succeeds.
		return MasterRecordForLifecycle->FindRef(Existing);

}

void UBarrageDispatch::FinalizeReleasePrimitive(FBarrageKey BarrageKey)
{
	//TODO return owned Joltstuff to pool or dealloc	
}


TStatId UBarrageDispatch::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBarrageDispatch, STATGROUP_Tickables);
}

void UBarrageDispatch::StepWorld()
{
	JoltGameSim->StepSimulation();
	//maintain tombstones
	CleanTombs();
}
