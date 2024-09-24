#pragma once

//unity build isn't fond of this, but we really want to completely contain these types and also prevent any collisions.
//there's other ways to do this, but the correct way is a namespace so far as I know.
// see: https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine?application_version=5.4#namespaces


#include "BarrageDispatch.h"
#include "Containers/CircularQueue.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "CoordinateUtils.h"
#include "FBShapeParams.h"
#include "FBarrageKey.h"
#include "FBPhysicsInput.h"

#include "SkeletonTypes.h"
#include "IsolatedJoltIncludes.h"
#include "PhysicsEngine/BodySetup.h"

// All Jolt symbols are in the JPH namespace

// Callback for traces, connect this to your own trace function if you have one
static void TraceImpl(const char* inFMT, ...)
{
	// Format the message
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), inFMT, list);
	va_end(list);
}


// We're also using STL classes in this example
#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine)
{
	// Breakpoint
	return true;
};


#endif // JPH_ENABLE_ASSERTS


// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
namespace Layers
{
	static constexpr JPH::ObjectLayer NON_MOVING = 0;
	static constexpr JPH::ObjectLayer MOVING = 1;
	static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};


namespace JOLT
{
class FBCharacterBase
{
public:
	virtual ~FBCharacterBase() = default;
	
	Ref<Shape> InnerStandingShape;
	JPH::RVec3 mInitialPosition = JPH::RVec3::sZero();
	float mHeightStanding = 1.35f;
	float mRadiusStanding = 0.3f;
	JPH::CharacterVirtualSettings mCharacterSettings;
	CharacterVirtual::ExtendedUpdateSettings mUpdateSettings;
	// Accumulated during IngestUpdate
	Vec3 mVelocityUpdate = Vec3::sZero();
	Vec3 mForcesUpdate = Vec3::sZero();
	JPH::Quat mCapsuleRotationUpdate = JPH::Quat::sIdentity();
	Ref<CharacterVirtual> mCharacter = Ref<CharacterVirtual>();
	float mDeltaTime = 0.01; //set this yourself or have a bad time.

	// Calculated effective velocity after a step
	Vec3 mEffectiveVelocity = Vec3::sZero();
	virtual void IngestUpdate(FBPhysicsInput& input) = 0;
	virtual void StepCharacter() = 0;
	
	TSharedPtr<JPH::PhysicsSystem, ESPMode::ThreadSafe> World;
protected:
	friend class FWorldSimOwner;
	TWeakPtr<FWorldSimOwner> Machine;
};

class FWorldSimOwner
{
	
	// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.



	
public:
	//members are destructed first in, last out.
	//https://stackoverflow.com/questions/2254263/order-of-member-constructor-and-destructor-calls
	//BodyId is actually a freaking 4byte struct, so it's _worse_ potentially to have a pointer to it than just copy it.
	TSharedPtr<TMap<FBarrageKey, BodyID>> BarrageToJoltMapping;
	TSharedPtr<TMap<FBarrageKey, TSharedPtr<FBCharacterBase>>> CharacterToJoltMapping;

	const uint AllocationArenaSize = 100 * 1024 * 1024;
	TSharedPtr<TempAllocatorImpl> Allocator;
	// List of active characters in the scene so they can collide
	//https://github.com/jrouwe/JoltPhysics/blob/e3ed3b1d33f3a0e7195fbac8b45b30f0a5c8a55b/Jolt/Physics/Character/CharacterVirtual.h#L143
	//note this is the only extant character collision handler, and it is not very efficient since it doesn't use a BSP.
	//This will need to go as soon as we have more than three characters or so.
	CharacterVsCharacterCollisionSimple CharacterVsCharacterCollisionSimple;
	// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
	// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
	// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
	// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
	// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.


	/// Class that determines if two object layers can collide
	class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
		{
			switch (inObject1)
			{
			case Layers::NON_MOVING:
				return inObject2 == Layers::MOVING; // Non moving only collides with moving
			case Layers::MOVING:
				return true; // Moving collides with everything
			default:
				JPH_ASSERT(false);
				return false;
			}
		}
	};
	
	class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
	{
	public:
		BPLayerInterfaceImpl()
		{
			// Create a mapping table from object to broad phase layer
			mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
			mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
		}

		virtual uint GetNumBroadPhaseLayers() const override
		{
			return BroadPhaseLayers::NUM_LAYERS;
		}

		virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
		{
			JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
			return mObjectToBroadPhase[inLayer];
		}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
		{
			switch (static_cast<BroadPhaseLayer::Type>(inLayer))
			{
			case static_cast<BroadPhaseLayer::Type>(BroadPhaseLayers::NON_MOVING): return "NON_MOVING";
			case static_cast<BroadPhaseLayer::Type>(BroadPhaseLayers::MOVING): return "MOVING";
			default: JPH_ASSERT(false);
				return "INVALID";
			}
		}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

	private:
		BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
	};

	/// Class that determines if an object layer can collide with a broadphase layer
	class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
		{
			switch (inLayer1)
			{
			case Layers::NON_MOVING:
				return inLayer2 == BroadPhaseLayers::MOVING;
			case Layers::MOVING:
				return true;
			default:
				JPH_ASSERT(false);
				return false;
			}
		}
	};

	// An example contact listener
	class MyContactListener : public ContactListener
	{
	public:
		// See: ContactListener
		virtual ValidateResult OnContactValidate(const Body& inBody1, const Body& inBody2, RVec3Arg inBaseOffset,
		                                         const CollideShapeResult& inCollisionResult) override
		{
			// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
			return ValidateResult::AcceptAllContactsForThisBodyPair;
		}

		virtual void OnContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold,
		                            ContactSettings& ioSettings) override
		{
		}

		virtual void OnContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold,
		                                ContactSettings& ioSettings) override
		{
		}

		virtual void OnContactRemoved(const SubShapeIDPair& inSubShapePair) override
		{
		}
	};

	// An example activation listener
	class MyBodyActivationListener : public BodyActivationListener
	{
	public:
		virtual void OnBodyActivated(const BodyID& inBodyID, uint64 inBodyUserData) override
		{
		}

		virtual void OnBodyDeactivated(const BodyID& inBodyID, uint64 inBodyUserData) override
		{
		}
	};

public:

	using ThreadFeed = TCircularQueue<FBPhysicsInput>;

	struct FeedMap
	{

		std::thread::id That = std::thread::id();
		TSharedPtr<ThreadFeed> Queue = nullptr;

		FeedMap()
		{
			That = std::thread::id();
			Queue = nullptr;
		}

		FeedMap(std::thread::id MappedThread, uint16 MaxQueueDepth)
		{
			That = MappedThread;
			Queue = MakeShareable(new ThreadFeed(MaxQueueDepth));
		}
	};

	FeedMap ThreadAcc[ALLOWED_THREADS_FOR_BARRAGE_PHYSICS];

	TSharedPtr<JobSystemThreadPool> job_system;
	// Create mapping table from object layer to broadphase layer
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	BPLayerInterfaceImpl broad_phase_layer_interface;

	// Create class that filters object vs broadphase layers
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;

	// Create class that filters object vs object layers
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	ObjectLayerPairFilterImpl object_vs_object_layer_filter;


	// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	TSharedPtr<MyContactListener> contact_listener;

	// A body activation listener gets notified when bodies activate and go to sleep
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	TSharedPtr<MyBodyActivationListener> body_activation_listener;

	float DeltaTime = 0.01; //You should set this or pass it in.

	//this is actually a member of the physics system
	BodyInterface* body_interface;

	// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const uint cMaxBodies = 65536;

	// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
	const uint cNumBodyMutexes = 0;

	// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
	// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
	// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const uint cMaxBodyPairs = 65536;

	// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
	// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
	// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
	const uint cMaxContactConstraints = 16384;


	//do not move this up. see C++ standard ~ 12.6.2
	TSharedPtr<PhysicsSystem> physics_system;
	FWorldSimOwner(float cDeltaTime);

	FBarrageKey SphereCast(double Radius, double Distance, FVector3d CastFrom, FVector3d Direction, JPH::BodyID& CastingBody);
	
	//we could use type indirection or inheritance, but the fact of the matter is that this is much easier
	//to understand and vastly vastly faster. it's also easier to optimize out allocations, and it's very
	//very easy to read for people who are probably already drowning in new types.
	//finally, it allows FBShapeParams to be a POD and so we can reason about it really easily.
	FBarrageKey CreatePrimitive(FBBoxParams& ToCreate, uint16 Layer);
	FBarrageKey CreatePrimitive(FBCharParams& ToCreate, uint16 Layer);
	FBarrageKey CreatePrimitive(FBSphereParams& ToCreate, uint16 Layer);
	FBarrageKey CreatePrimitive(FBCapParams& ToCreate, uint16 Layer);

	FBLet LoadComplexStaticMesh(FBMeshParams& Definition,
		const UStaticMeshComponent* StaticMeshComponent,
		FSkeletonKey Outkey);

	//This'll be trouble.
	//https://www.youtube.com/watch?v=KKC3VePrBOY&lc=Ugw9YRxHjcywQKH5LO54AaABAg
	void StepSimulation();

	//Broad Phase is the first pass in the engine's cycle, and the optimization used to accelerate it breaks down as objects are added. As a result, when you have time after adding objects,
	//you should call optimize broad phase. You should also batch object creation whenever possible, but we don't support that well yet.
	//Generally, as we add and remove objects, we'll want to perform this, but we really don't want to run it every tick. We can either use trigger logic or a cadenced ticklite
	void OptimizeBroadPhase();

	void FinalizeReleasePrimitive(FBarrageKey BarrageKey)
	{
		//TODO return owned Joltstuff to pool or dealloc
		auto bID = BarrageToJoltMapping->Find(BarrageKey);
		if (bID)
		{
			body_interface->RemoveBody(*bID);
			body_interface->DestroyBody(*bID);
		}
	}
	FBarrageKey GenerateBarrageKeyFromBodyId(const BodyID& Input) const;
	~FWorldSimOwner();
	bool UpdateCharacter(FBPhysicsInput& Update);
	bool UpdateCharacters(TSharedPtr<TArray<FBPhysicsInput>> Array);
};

}