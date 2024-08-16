#include "FWorldSimOwner.h"
#include "CoordinateUtils.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"

FWorldSimOwner::FWorldSimOwner(float cDeltaTime)
{
	DeltaTime = cDeltaTime;
	// Register allocation hook. In this example we'll just let Jolt use malloc / free but you can override these if you want (see Memory.h).
	// This needs to be done before any other Jolt function is called.
	BarrageToJoltMapping = MakeShareable(new TMap<FBarrageKey, BodyID>());
	RegisterDefaultAllocator();

	Allocator = MakeShareable(new TempAllocatorImpl(AllocationArenaSize));
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

//we need the coordinate utils, but we don't really want to include them in the .h
inline FBarrageKey FWorldSimOwner::CreatePrimitive(FBBoxParams& ToCreate, uint16 Layer)
{
	uint64_t KeyCompose;
	KeyCompose = PointerHash(this);
	KeyCompose = KeyCompose << 32;
	BodyID BodyIDTemp = BodyID();
	EMotionType MovementType = Layer == 0 ? EMotionType::Static : EMotionType::Dynamic;

	BoxShapeSettings box_settings(Vec3(ToCreate.JoltX, ToCreate.JoltY, ToCreate.JoltZ));
	//floor_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.
	// Create the shape
	ShapeSettings::ShapeResult box = box_settings.Create();
	ShapeRefC box_shape = box.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()
	// Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
	BodyCreationSettings box_body_settings(box_shape, CoordinateUtils::ToJoltCoordinates(ToCreate.point), Quat::sIdentity(), MovementType, Layer);
	// Create the actual rigid body
	Body* box_body = body_interface->CreateBody(box_body_settings); // Note that if we run out of bodies this can return nullptr

	// Add it to the world
	body_interface->AddBody(box_body->GetID(), EActivation::Activate);
	BodyIDTemp = box_body->GetID();

			
	KeyCompose |= BodyIDTemp.GetIndexAndSequenceNumber();
	//Barrage key is unique to WORLD and BODY. This is crushingly important.
	BarrageToJoltMapping->Add(static_cast<FBarrageKey>(KeyCompose), BodyIDTemp);

	return static_cast<FBarrageKey>(KeyCompose);
}


inline FBarrageKey FWorldSimOwner::CreatePrimitive(FBSphereParams& ToCreate, uint16 Layer)
{
	uint64_t KeyCompose;
	KeyCompose = PointerHash(this);
	KeyCompose = KeyCompose << 32;
	BodyID BodyIDTemp = BodyID();
	EMotionType MovementType = Layer == 0 ? EMotionType::Static : EMotionType::Dynamic;
	
	BodyCreationSettings sphere_settings(new SphereShape(ToCreate.JoltRadius),
		                                     CoordinateUtils::ToJoltCoordinates(ToCreate.point),
		                                     Quat::sIdentity(),
		                                     MovementType,
		                                     Layer);
	BodyIDTemp = body_interface->CreateAndAddBody(sphere_settings, EActivation::Activate);
	
	KeyCompose |= BodyIDTemp.GetIndexAndSequenceNumber();
	//Barrage key is unique to WORLD and BODY. This is crushingly important.
	BarrageToJoltMapping->Add(static_cast<FBarrageKey>(KeyCompose), BodyIDTemp);
	return static_cast<FBarrageKey>(KeyCompose);
}

inline FBarrageKey FWorldSimOwner::CreatePrimitive(FBCapParams& ToCreate, uint16 Layer)
{
	uint64_t KeyCompose;
	KeyCompose = PointerHash(this);
	KeyCompose = KeyCompose << 32;
	BodyID BodyIDTemp = BodyID();
	EMotionType MovementType = Layer == 0 ? EMotionType::Static : EMotionType::Dynamic;
	BodyCreationSettings cap_settings(new CapsuleShape(ToCreate.JoltHalfHeightOfCylinder, ToCreate.JoltRadius),
										 CoordinateUtils::ToJoltCoordinates(ToCreate.point),
										 Quat::sIdentity(),
										 MovementType,
										 Layer);
	BodyIDTemp = body_interface->CreateAndAddBody(cap_settings, EActivation::Activate);
	KeyCompose |= BodyIDTemp.GetIndexAndSequenceNumber();
	//Barrage key is unique to WORLD and BODY. This is crushingly important.
	BarrageToJoltMapping->Add(static_cast<FBarrageKey>(KeyCompose), BodyIDTemp);

	return static_cast<FBarrageKey>(KeyCompose);
}

void FWorldSimOwner::StepSimulation()
{
	// If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
	constexpr int cCollisionSteps = 1;

	// Step the world
	physics_system.Update(DeltaTime, cCollisionSteps, Allocator.Get(), job_system.Get());

	//TODO: tombstone handling
}

FWorldSimOwner::~FWorldSimOwner()
{
	UnregisterTypes();
	
	// Destroy the factory
	delete Factory::sInstance;
	Factory::sInstance = nullptr;
	GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Green, TEXT("Goodnight, Barrage!"));
}
