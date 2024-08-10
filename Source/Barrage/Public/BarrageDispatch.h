#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Containers/TripleBuffer.h"
#include "FBarrageKey.h"

#include "CapsuleTypes.h"
#include "FBarragePrimitive.h"
#include "Containers/CircularQueue.h"
#include "Containers/Deque.h"
#include "BarrageDispatch.generated.h"



class FBarrageBounder
{
	friend class FBBoxParams;
	friend class FBSphereParams;
	friend class FBCapParams;
	//convert from UE to Jolt without exposing the jolt types or coordinates.
	static FBBoxParams GenerateBoxBounds(FVector3d point, double xDiam, double yDiam, double zDiam);
	static FBSphereParams GenerateSphereBounds(FVector3d point, double radius);
	static FBCapParams GenerateCapsuleBounds(UE::Geometry::FCapsule3d Capsule);
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


	virtual void SphereCast(double Radius, FVector3d CastFrom, uint64_t timestamp = 0);
	//and viola [sic] actually pretty elegant even without type polymorphism by using overloading polymorphism.
	FBLet CreatePrimitive(FBBoxParams& Definition, uint64 Outkey, uint16 Layer);
	FBLet CreatePrimitive(FBSphereParams& Definition, uint64 OutKey, uint16 Layer);
	FBLet CreatePrimitive(FBCapParams& Definition, uint64 OutKey, uint16 Layer);
	FBLet LoadComplexStaticMesh(FBMeshParams& Definition, const UStaticMeshComponent* StaticMeshComponent, uint64 Outkey, FBarrageKey& InKey);
	FBLet GetShapeRef(FBarrageKey Existing) const;
	void FinalizeReleasePrimitive(FBarrageKey BarrageKey);

	//any non-zero value is the same, effectively, as a nullity for the purposes of any new operation.
	//because we can't control certain aspects of timing and because we may need to roll back, we use tombstoning
	//instead of just reference counting and deleting - this is because cases arise where there MUST be an authoritative
	//single source answer to the alive/dead question for a rigid body, but we still want all the advantages of ref counting
	//and we want to be able to revert that decision for faster rollbacks or for pooling purposes.
	constexpr static uint32 TombstoneInitialMinimum = 9;

	uint32 SuggestTombstone(const FBLet& Target)
	{
		if (FBarragePrimitive::IsNotNull(Target))
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

private:
	TSharedPtr<TMap<FBarrageKey, FBLet>> BodyLifecycleOwner;
	FBLet ManagePointers(uint64 OutKey, FBarrageKey temp, FBarragePrimitive::FBShape form);
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
		BodyLifecycleOwner->Remove(NONREFERENCE->KeyIntoBarrage);
		// there doesn't seem to be a better way to do this idiomatically in the UE framework.
		//push into tomb here. because we shadow two back on the release, this is guaranteed to be safe,
		Tombs[TombOffset]->Push(NONREFERENCE);
		//DO NOT make K a reference parameter
	}
};


