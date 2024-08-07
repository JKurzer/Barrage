#pragma once

//unity build isn't fond of this, but we really want to completely contain these types and also prevent any collisions.
//there's other ways to do this, but the correct way is a namespace so far as I know.
// see: https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine?application_version=5.4#namespaces
#include "FBarrageKey.h"
	PRAGMA_PUSH_PLATFORM_DEFAULT_PACKING
#include "Jolt/Jolt.h"
#include "Jolt/RegisterTypes.h"
#include "Jolt/Core/Factory.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Physics/PhysicsSettings.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Body/BodyActivationListener.h"
#include "Jolt/ConfigurationString.h"
	PRAGMA_POP_PLATFORM_DEFAULT_PACKING

	// STL includes
#include <iostream>
#include <cstdarg>
#include <thread>
#include <math.h>
#include "FBShapeParams.h"
	// All Jolt symbols are in the JPH namespace


	// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
	JPH_SUPPRESS_WARNINGS
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

	using namespace JPH;

	// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
	using namespace JPH::literals;

	// We're also using STL classes in this example
	using namespace std;
#ifdef JPH_ENABLE_ASSERTS

	// Callback for asserts, connect this to your own assert handler if you have one
	static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine)
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
		static constexpr ObjectLayer NON_MOVING = 0;
		static constexpr ObjectLayer MOVING = 1;
		static constexpr ObjectLayer NUM_LAYERS = 2;
	};

	/// Class that determines if two object layers can collide
	class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
	{
	public:
		virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
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

	// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
	// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
	// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
	// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
	// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
	namespace BroadPhaseLayers
	{
		static constexpr BroadPhaseLayer NON_MOVING(0);
		static constexpr BroadPhaseLayer MOVING(1);
		static constexpr uint NUM_LAYERS(2);
	};


	class FWorldSimOwner
	{
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
		//BodyId is actually a freaking 4byte struct, so it's _worse_ potentially to have a pointer to it than just copy it.
		TSharedPtr< TMap<FBarrageKey, BodyID>> BarrageToJoltMapping;
		PhysicsSystem physics_system;

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
		MyContactListener contact_listener;

		// A body activation listener gets notified when bodies activate and go to sleep
		// Note that this is called from a job so whatever you do here needs to be thread safe.
		// Registering one is entirely optional.
		MyBodyActivationListener body_activation_listener;

		float DeltaTime = 0.01; //You should set this or pass it in.

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
		// Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
		const uint cMaxContactConstraints = 16384;


		TSharedPtr<TempAllocatorImpl> temp_allocator;

		FWorldSimOwner(float cDeltaTime)
		{
			DeltaTime = cDeltaTime;
			// Register allocation hook. In this example we'll just let Jolt use malloc / free but you can override these if you want (see Memory.h).
			// This needs to be done before any other Jolt function is called.
			BarrageToJoltMapping = MakeShareable(new TMap<FBarrageKey, BodyID>());
			RegisterDefaultAllocator();

			temp_allocator = MakeShareable(new TempAllocatorImpl(100 * 1024 * 1024));
			// Install trace and assert callbacks
			Trace = TraceImpl;
			JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

			// Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
			// It is not directly used in this example but still required.
			Factory::sInstance = new Factory();

			// Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
			// If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
			// If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
			RegisterTypes();

			// We need a temp allocator for temporary allocations during the physics update. We're
			// pre-allocating 10 MB to avoid having to do allocations during the physics update.
			// B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
			// If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
			// malloc / free.


			// We need a job system that will execute physics jobs on multiple threads. Typically
			// you would implement the JobSystem interface yourself and let Jolt Physics run on top
			// of your own job scheduler. JobSystemThreadPool is an example implementation.
			job_system = MakeShareable(
				new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1));
			// Now we can create the actual physics system.
			physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
			                    broad_phase_layer_interface, object_vs_broadphase_layer_filter,
			                    object_vs_object_layer_filter);


			physics_system.SetBodyActivationListener(&body_activation_listener);


			physics_system.SetContactListener(&contact_listener);

			// The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
			// variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
			body_interface = &physics_system.GetBodyInterface();


			// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
			// You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
			// Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
			physics_system.OptimizeBroadPhase();

		// here's Andrea's transform into jolt.
		//	https://youtu.be/jhCupKFly_M?si=umi0zvJer8NymGzX&t=438
		}

		//we could use type indirection or inheritance, but the fact of the matter is that this is much easier
		//to understand and vastly vastly faster. it's also easier to optimize out allocations, and it's very
		//very easy to read for people who are probably already drowning in new types.
		//finally, it allows FBShapeParams to be a POD and so we can reason about it really easily.
		FBarrageKey CreatePrimitive(FBShapeParams& ToCreate)
		{
			uint64_t KeyCompose;
			KeyCompose = PointerHash(this);
			KeyCompose = KeyCompose << 32;
			BodyID BodyIDTemp = BodyID();
			EMotionType MovementType = ToCreate.layer == 0 ? EMotionType::Static : EMotionType::Dynamic;

			//prolly convert this to a case once it's settled.
			if(ToCreate.MyShape == ToCreate.Box)
			{
				BoxShapeSettings box_settings(Vec3(ToCreate.bound1, ToCreate.bound2, ToCreate.bound3));
				//floor_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.
				// Create the shape
				ShapeSettings::ShapeResult box = box_settings.Create();
				ShapeRefC box_shape = box.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()
				// Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
				BodyCreationSettings floor_settings(box_shape, RVec3(ToCreate.pointx,ToCreate.pointy, ToCreate.pointz), Quat::sIdentity(), MovementType, ToCreate.layer);
				// Create the actual rigid body
				Body* floor = body_interface->CreateBody(floor_settings); // Note that if we run out of bodies this can return nullptr

				// Add it to the world
				body_interface->AddBody(floor->GetID(), EActivation::DontActivate);
				BodyIDTemp = floor->GetID();
			}
			else if(ToCreate.MyShape == ToCreate.Sphere)
			{
				BodyCreationSettings sphere_settings(new SphereShape(ToCreate.bound1),
					RVec3(ToCreate.pointx,ToCreate.pointy, ToCreate.pointz),
					Quat::sIdentity(),
					MovementType,
					ToCreate.layer);
				BodyIDTemp = body_interface->CreateAndAddBody(sphere_settings, EActivation::Activate);
			}
			else if(ToCreate.MyShape == ToCreate.Capsule)
			{
				throw; //we don't support capsule yet.
			}
			else
			{
				throw; // we don't support any others.
			}
			// Now create a dynamic body to bounce on the floor
			// Note that this uses the shorthand version of creating and adding a body to the world
			
			KeyCompose |= BodyIDTemp.GetIndexAndSequenceNumber();
			//Barrage key is unique to WORLD and BODY. This is crushingly important.
			BarrageToJoltMapping->Add(static_cast<FBarrageKey>(KeyCompose), BodyIDTemp);

			return static_cast<FBarrageKey>(KeyCompose);
		}

		//This'll be trouble.
		//https://www.youtube.com/watch?v=KKC3VePrBOY&lc=Ugw9YRxHjcywQKH5LO54AaABAg
		void StepSimulation()
		{
			// If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
			constexpr int cCollisionSteps = 1;

			// Step the world
			physics_system.Update(DeltaTime, cCollisionSteps, temp_allocator.Get(), job_system.Get());

			//TODO: tombstone handling
		}

		//Broad Phase is the first pass in the engine's cycle, and the optimization used to accelerate it breaks down as objects are added. As a result, when you have time after adding objects,
		//you should call optimize broad phase. You should also batch object creation whenever possible, but we don't support that well yet.
		//Generally, as we add and remove objects, we'll want to perform this, but we really don't want to run it every tick. We can either use trigger logic or a cadenced ticklite
		void OptimizeBroadPhase()
		{
			// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
			// You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
			// Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
			physics_system.OptimizeBroadPhase();
		}

		~FWorldSimOwner()
		{
			UnregisterTypes();

			// Destroy the factory
			delete Factory::sInstance;
			Factory::sInstance = nullptr;
			GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Green, TEXT("Goodnight, Barrage!"));
		}
	};

