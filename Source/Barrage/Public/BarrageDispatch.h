#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Containers/TripleBuffer.h"
#include "FBarrageKey.h"
#include "FBarragePrimitive.h"
#include "Containers/CircularQueue.h"
#include "Containers/Deque.h"
#include "BarrageDispatch.generated.h"


//Note the use of shared pointers. Due to tombstoning, FBlets must always be used by reference.
//this is actually why they're called FBLets, as they're rented (or let) shapes that are also thus both shapelets and shape-lets.
class FBLetter
{
public:
	//transform forces transparently from UE world space to jolt world space
	//then apply them directly to the "primitive"
	static void ApplyForce(FVector3d Force, FBLet Target);
	//transform the quaternion from the UE ref to the Jolt ref
	//then apply it to the "primitive"
	static void ApplyRotation(FQuat4d Rotator, FBLet Target);

	//as the barrage primitive contains both the in and out keys, that is sufficient to act as a full mapping
	//IFF you can supply the dispatch provider that owns the out key. this is done as a template arg
	template <typename OutKeyDispatch>
	static void PublishTransformFromJolt(FBLet Target);

	static FVector3d GetCentroidPossiblyStale(FBLet Target);
	//tombstoned primitives are treated as null even by live references, because while the primitive is valid
	//and operations against it can be performed safely, no new operations should be allowed to start.
	//the tombstone period is effectively a grace period due to the fact that we have quite a lot of different
	//timings in play. it should be largely unnecessary, but it's also a very useful semantic for any pooled
	//data and allows us to batch disposal nicely.
	static inline bool IsNotNull(FBLet Target);
};
class FBarrageBounder
{
	//convert from UE to Jolt without exposing the jolt types.
	static FBShapeParams GenerateBoxBounds(	double pointx, double pointy, double pointz, double xHalfEx, double yHalfEx, double zHalfEx);
	//transform the quaternion from the UE ref to the Jolt ref
	//then apply it to the "primitive"
	static FBShapeParams GenerateSphereBounds(double pointx, double pointy, double pointz, double radius);

	//as the barrage primitive contains both the in and out keys, that is sufficient to act as a full mapping
	//IFF you can supply the dispatch provider that owns the out key. this is done as a template arg
	static FBShapeParams GenerateCapsuleBounds(UE::Geometry::FCapsule3d Capsule);
};

class FWorldSimOwner;

UCLASS()
class BARRAGE_API UBarrageDispatch : public UTickableWorldSubsystem
{
	GENERATED_BODY()
	static inline constexpr float TickRateInDelta = 1.0 / 120.0;

public:
	UBarrageDispatch();
	virtual ~UBarrageDispatch() override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	static FVector3d ToJoltCoordinates(FVector3d In);
	static FVector3d FromJoltCoordinates(FVector3d In);
	static FQuat4d ToJoltRotation(FQuat4d In);
	static FQuat4d FromJoltRotation(FQuat4d In);
	virtual void SphereCast(double Radius, FVector3d CastFrom, uint64_t timestamp = 0);
	FBLet CreateSimPrimitive(FBShapeParams& Definition, uint64 Outkey);
	FBLet GetShapeRef(FBarrageKey Existing);
	void FinalizeReleasePrimitive(FBarrageKey BarrageKey);

	//any non-zero value is the same, effectively, as a nullity for the purposes of any new operation.
	//because we can't control certain aspects of timing and because we may need to roll back, we use tombstoning
	//instead of just reference counting and deleting - this is because cases arise where there MUST be an authoritative
	//single source answer to the alive/dead question for a rigid body, but we still want all the advantages of ref counting
	//and we want to be able to revert that decision for faster rollbacks or for pooling purposes.
	constexpr static uint32 TombstoneInitialMinimum = 9;

	uint32 SuggestTombstone(const FBLet& Target)
	{
		if (FBLetter::IsNotNull(Target))
		{
			Target->tombstone = TombstoneInitialMinimum + TombOffset;
			return Target->tombstone;
		}
		return 1;
		// one indicates that something has no remaining time to live, and is equivalent to finding a nullptr. we return if for tombstone suggestions against tombstoned or null data.
	}

	virtual TStatId GetStatId() const override;
	TSharedPtr<FWorldSimOwner> JoltGameSim;
	//ONLY call this from a thread OTHER than gamethread, or you will experience untold sorrow.
	void StepWorld();

protected:
	friend FWorldSimOwner;
	friend FBLetter;

private:
	TSharedPtr<TMap<FBarrageKey, FBLet>> MasterRecordForLifecycle;

	uint32 TombOffset = 0; //ticks up by one every world step.
	//this is a little hard to explain. so keys are inserted as 

	//clean tombs must only ever be called from step world which must only ever be called from one thread.
	//this reserves as little memory as possible, but it could be quite a lot (megs) of reserved memory if you expire
	//tons and tons of bodies in one frame. if that's a bottleneck for you, you may wish to shorten the tombstone promise
	//or optimize this for memory better. In general, Barrage and Artillery trade memory for speed and elegance.
	TSharedPtr<TArray<FBLet>> Tombs[TombstoneInitialMinimum + 1];

	void CleanTombs()
	{
		//free tomb at offset - TombstoneInitialMinimum, fulfilling our promised minimum.
		Tombs[(TombOffset - TombstoneInitialMinimum) % (TombstoneInitialMinimum + 1)]->Empty(); //roast 'em lmao.
		TombOffset = (TombOffset + 1) % (TombstoneInitialMinimum + 1);
	}

	void Entomb(FBLet NONREFERENCE)
	{
		//request removal here
		MasterRecordForLifecycle->Remove(NONREFERENCE->KeyIntoBarrage);
		// there doesn't seem to be a better way to do this idiomatically in the UE framework.
		//push into tomb here. because we shadow two back on the release, this is guaranteed to be safe,
		Tombs[TombOffset]->Push(NONREFERENCE);
		//DO NOT make K a reference parameter
	}
};

void FBLetter::ApplyRotation(FQuat4d Rotator, FBLet Target)
{
}

template <typename OutKeyDispatch>
void FBLetter::PublishTransformFromJolt(FBLet Target)
{
}

FVector3d FBLetter::GetCentroidPossiblyStale(FBLet Target)
{
	return FVector3d();
}

bool FBLetter::IsNotNull(FBLet Target)
{
	return Target != nullptr && Target.IsValid() && Target->tombstone == 0;
}

void FBLetter::ApplyForce(FVector3d Force, FBLet Target)
{
}
