#pragma once
#include "SkeletonTypes.h"

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FBarrageKey.h"
#include "Chaos/Particles.h"
#include "CapsuleTypes.h"
#include "FBarragePrimitive.h"
#include "FBPhysicsInput.h"
#include "Containers/CircularQueue.h"
#include "FBShapeParams.h"
#include "FBarrageMaps.h"
#include "BarrageContactEvent.h"
#include "BarrageDispatch.generated.h"

class BARRAGE_API FBarrageBounder
{
	
	friend class FBBoxParams;
	friend class FBSphereParams;
	friend class FBCapParams;
	//convert from UE to Jolt without exposing the jolt types or coordinates.
public:
	static FBBoxParams GenerateBoxBounds(FVector3d point , double xDiam, double yDiam, double zDiam, FVector3d OffsetCenterToMatchBoundedShape = FVector::Zero());
	static FBSphereParams GenerateSphereBounds(FVector3d point, double radius);
	static FBCapParams GenerateCapsuleBounds(UE::Geometry::FCapsule3d Capsule);
	static FBCharParams GenerateCharacterBounds(FVector3d point, double radius, double extent, double speed);
};

namespace JOLT
{
	struct BarrageContactEvent;
	class FWorldSimOwner;
}

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBarrageContactAdded, const BarrageContactEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBarrageContactPersisted, const BarrageContactEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBarrageContactRemoved, const BarrageContactEvent&);

constexpr int ALLOWED_THREADS_FOR_BARRAGE_PHYSICS =64;
//if we could make a promise about when threads are allocated, we could probably get rid of this
//since the accumulator is in the world subsystem and so gets cleared when the world spins down.
//that would mean that we could add all the threads, then copy the state from the volatile array to a
//fixed read-only hash table during begin play. this is already complicated though, and let's see if we like it
//before we invest more time in it. I had a migraine when I wrote this, by way of explanation --J
thread_local static uint32 MyBARRAGEIndex = ALLOWED_THREADS_FOR_BARRAGE_PHYSICS + 1;
UCLASS()
class BARRAGE_API UBarrageDispatch : public UTickableWorldSubsystem
{
	GENERATED_BODY()
	friend class JOLT::FWorldSimOwner;
	static inline constexpr float TickRateInDelta = 1.0 / 120.0;

public:


	uint8 ThreadAccTicker = 0;
	TSharedPtr<TransformUpdatesForGameThread> GameTransformPump;
	TSharedPtr<TCircularQueue<BarrageContactEvent>> ContactEventPump;
	 //this value indicates you have none.
	mutable FCriticalSection GrowOnlyAccLock;

	// Why would I do it this way? It's fast and easy to debug, and we will probably need to force a thread
	// order for determinism. this ensures there's a call point where we can institute that.
	void GrantFeed();
	UBarrageDispatch();
	
	virtual ~UBarrageDispatch() override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;


	virtual void SphereCast(FBarrageKey ShapeSource, double Radius, double Distance, FVector3d CastFrom, FVector3d Direction, TSharedPtr<FHitResult> OutHit, uint64_t timestamp = 0);
	//and viola [sic] actually pretty elegant even without type polymorphism by using overloading polymorphism.
	FBLet CreatePrimitive(FBBoxParams& Definition, FSkeletonKey Outkey, uint16 Layer, bool IsSensor = false);
	FBLet CreatePrimitive(FBCharParams& Definition, FSkeletonKey Outkey, uint16 Layer);
	FBLet CreatePrimitive(FBSphereParams& Definition, FSkeletonKey OutKey, uint16 Layer, bool IsSensor = false);
	FBLet CreatePrimitive(FBCapParams& Definition, FSkeletonKey OutKey, uint16 Layer, bool IsSensor = false);
	FBLet LoadComplexStaticMesh(FBMeshParams& Definition, const UStaticMeshComponent* StaticMeshComponent, FSkeletonKey Outkey);
	FBLet GetShapeRef(FBarrageKey Existing) const;
	FBLet GetShapeRef(FSkeletonKey Existing) const;
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
	TSharedPtr< JOLT::FWorldSimOwner> JoltGameSim;

	//StackUp should be called before stepworld and from the same thread. anything can be done between them.
	//Returns rather than applies the FBPhysicsInputs that affect Primitives of Types: Character
	//This list may expand. Failure to handle these will result in catastrophic bugs.
	void StackUp();
	bool UpdateCharacters(TSharedPtr<TArray<FBPhysicsInput>> CharacterInputs);
	bool UpdateCharacter(FBPhysicsInput& CharacterInput);
	//ONLY call this from a thread OTHER than gamethread, or you will experience untold sorrow.
	void StepWorld(uint64 Time);

	//TODO: oh dear I'm doing the same thing as the TransformQueue... Also probably want to check back on this.
	bool BroadcastContactEvents();
	
	FOnBarrageContactAdded OnBarrageContactAddedDelegate;
	void HandleContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold,
									ContactSettings& ioSettings);
	FOnBarrageContactPersisted OnBarrageContactPersistedDelegate;
	void HandleContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold,
									ContactSettings& ioSettings);
	FOnBarrageContactRemoved OnBarrageContactRemovedDelegate;
	void HandleContactRemoved(const SubShapeIDPair& inSubShapePair);

	FBarrageKey GenerateBarrageKeyFromBodyId(const JPH::BodyID& Input) const;
	FBarrageKey GenerateBarrageKeyFromBodyId(const uint32 RawIndexAndSequenceNumberInput) const;

	FBarrageKey GetBarrageKeyFromFHitResult(TSharedPtr<FHitResult> HitResult) const
	{
		check(HitResult.IsValid());
		return HitResult.Get()->MyItem != JPH::BodyID::cInvalidBodyID ? GenerateBarrageKeyFromBodyId(static_cast<uint32>(HitResult.Get()->MyItem)) : 0;
	}
	
protected:

private:
	TSharedPtr<KeyToFBLet> JoltBodyLifecycleMapping;
	
	TSharedPtr<KeyToKey> TranslationMapping;
	FBLet ManagePointers(FSkeletonKey OutKey, FBarrageKey temp, FBarragePrimitive::FBShape form);
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
		auto HoldOpen = Tombs;
		auto& Mausoleum = HoldOpen[(TombOffset - TombstoneInitialMinimum) % (TombstoneInitialMinimum + 1)];
		if(Mausoleum)
		{
			Mausoleum->Empty(); //roast 'em lmao.
		}
		TombOffset = (TombOffset + 1) % (TombstoneInitialMinimum + 1);
	}

	void inline Entomb(FBLet NONREFERENCE)
	{
		//request removal here
		JoltBodyLifecycleMapping->erase(NONREFERENCE->KeyIntoBarrage);
		TranslationMapping->erase(NONREFERENCE->KeyOutOfBarrage);
		// there doesn't seem to be a better way to do this idiomatically in the UE framework.
		//push into tomb here. because we shadow two back on the release, this is guaranteed to be safe?
		Tombs[TombOffset]->Push(NONREFERENCE);
		//DO NOT make that a reference parameter
	}
};

