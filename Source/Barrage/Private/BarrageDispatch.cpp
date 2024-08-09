#include "BarrageDispatch.h"
#include "FWorldSimOwner.h"




UBarrageDispatch::UBarrageDispatch()
{
}

UBarrageDispatch::~UBarrageDispatch()
{
	//now that all primitives are destructed
	GlobalBarrage = nullptr;
}

void UBarrageDispatch::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	for (auto& x : Tombs)
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
	for (auto& x : Tombs)
	{
		x = nullptr;
	}
}

//TODO: these will all need reworked for determinism.
FVector3d UBarrageDispatch::ToJoltCoordinates(FVector3d In)
{
	return FVector3d(In.X/100.0, In.Z/100.0, In.Y/100.0); //reverse is 0,2,1
}

FVector3d UBarrageDispatch::FromJoltCoordinates(FVector3d In)
{
	return FVector3d(In.X*100.0, In.Z*100.0, In.Y*100.0); // this looks _wrong_.
}

//These should be fine.
FQuat4d UBarrageDispatch::ToJoltRotation(FQuat4d In)
{
	return FQuat4d(-In.X, -In.Z, -In.Y, In.W);
}
FQuat4d UBarrageDispatch::FromJoltRotation(FQuat4d In)
{
	return FQuat4d(-In.X, -In.Z, -In.Y, In.W); 
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

FBLet UBarrageDispatch::LoadStaticMesh(FBShapeParams& Definition, uint64 Outkey)
{
	return FBLet();
}

//unlike our other ecs components in artillery, barrage dispatch does not maintain the mappings directly.
//this is because we may have _many_ jolt sims running if we choose to do deterministic rollback in certain ways.
//This is a copy by value return on purpose, as we want the ref count to rise.
FBLet UBarrageDispatch::GetShapeRef(FBarrageKey Existing)
{
	//SharedPTR's def val is nullptr. this will return nullptr as soon as entomb succeeds.
	//if entomb gets sliced, the tombstone check will fail as long as it is performed within 27 milliseconds of this call.
	//as a result, one of two states will arise: you get a null pointer, or you get a valid shared pointer
	//which will hold the asset open until you're done, but the tombstone markings will be set, letting you know
	//that this thoroughfare? it leads into the region of peril.
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

FBShapeParams FBarrageBounder::GenerateBoxBounds(double pointx, double pointy, double pointz, double xHalfEx,
	double yHalfEx, double zHalfEx)
{
	return FBShapeParams();
}

FBShapeParams FBarrageBounder::GenerateSphereBounds(double pointx, double pointy, double pointz, double radius)
{
	return FBShapeParams();
}

FBShapeParams FBarrageBounder::GenerateCapsuleBounds(UE::Geometry::FCapsule3d Capsule)
{
	return FBShapeParams();
}
