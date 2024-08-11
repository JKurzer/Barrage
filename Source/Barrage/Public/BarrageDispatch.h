#pragma once

#include <thread>

#include "CoreMinimal.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "Subsystems/WorldSubsystem.h"
#include "Containers/TripleBuffer.h"
#include "FBarrageKey.h"

#include "Chaos/Particles.h"
#include "CapsuleTypes.h"
#include "FBarragePrimitive.h"
#include "Containers/CircularQueue.h"
#include "Containers/Deque.h"
#include "BarrageDispatch.generated.h"


struct FBPhysicsInput;

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

#define ALLOWED_THREADS_FOR_BARRAGE_PHYSICS 64
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
	static inline constexpr float TickRateInDelta = 1.0 / 120.0;
	
public:
	
	typedef TCircularQueue<FBPhysicsInput> ThreadFeed;
	
	struct FeedMap
	{
		std::thread::id That = std::thread::id();
		TSharedPtr<ThreadFeed> Queue = nullptr;
	};
	struct TransformUpdate
	{
		uint64 ObjectKey;
		uint64 sequence;
		FTransform3d update; // this alignment looks wrong. Like outright wrong.
	};
	uint8 ThreadAccTicker = 0;
	typedef TCircularQueue<TransformUpdate> TransformUpdatesForGameThread;
	TSharedPtr<TransformUpdatesForGameThread> GameTransformPump;
	FeedMap ThreadAcc[ALLOWED_THREADS_FOR_BARRAGE_PHYSICS];
	 //this value indicates you have none.
	mutable FCriticalSection GrowOnlyAccLock;

	// Why would I do it this way? It's fast and easy to debug, and we will probably need to force a thread
	// order for determinism. this ensures there's a call point where we can institute that.
	void GrantFeed()
	{
	FScopeLock GrantFeedLock(&GrowOnlyAccLock);
		ThreadAcc[ThreadAccTicker].That = std::this_thread::get_id();
		MyBARRAGEIndex = ThreadAccTicker;
		//TODO: expand if we need for rollback powers. could be sliiiick
		ThreadAcc[ThreadAccTicker].Queue = MakeShareable(new ThreadFeed(1024));
		++ThreadAccTicker;
	}
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

	//StackUp should be called before stepworld and from the same thread. anything can be done between them.
	void StackUp();
	//ONLY call this from a thread OTHER than gamethread, or you will experience untold sorrow.
	template<typename TimeKeeping>
	void StepWorld();

protected:
	friend FWorldSimOwner;

private:
	TSharedPtr<TMap<FBarrageKey, FBLet>> JoltBodyLifecycleOwnerMapping;
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
		JoltBodyLifecycleOwnerMapping->Remove(NONREFERENCE->KeyIntoBarrage);
		// there doesn't seem to be a better way to do this idiomatically in the UE framework.
		//push into tomb here. because we shadow two back on the release, this is guaranteed to be safe?
		Tombs[TombOffset]->Push(NONREFERENCE);
		//DO NOT make that a reference parameter
	}
};


