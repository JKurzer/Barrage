#include "BarrageDispatch.h"
#include "FWorldSimOwner.h"
#include "CoordinateUtils.h"
#include "FBPhysicsInput.h"

//https://github.com/GaijinEntertainment/DagorEngine/blob/71a26585082f16df80011e06e7a4e95302f5bb7f/prog/engine/phys/physJolt/joltPhysics.cpp#L800
//this is how gaijin uses jolt, and war thunder's honestly a pretty strong comp to our use case.

	


void UBarrageDispatch::GrantFeed()
{
	FScopeLock GrantFeedLock(&GrowOnlyAccLock);

	//TODO: expand if we need for rollback powers. could be sliiiick
	JoltGameSim->ThreadAcc[ThreadAccTicker] = JOLT::FWorldSimOwner::FeedMap(std::this_thread::get_id(), 1024);
	
	MyBARRAGEIndex = ThreadAccTicker;
	++ThreadAccTicker;
}

UBarrageDispatch::UBarrageDispatch()
{
}

UBarrageDispatch::~UBarrageDispatch()
{
	//now that all primitives are destructed
	FBarragePrimitive::GlobalBarrage = nullptr;
	
}

void UBarrageDispatch::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	for (auto& x : Tombs)
	{
		x = MakeShareable(new TArray<FBLet>());
	}
	FBarragePrimitive::GlobalBarrage = this;
	UE_LOG(LogTemp, Warning, TEXT("Barrage:Subsystem: Online"));
}

void UBarrageDispatch::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	//this approach may actually be too slow. it is pleasingly lockless, but it allocs 16megs
	//and just iterating through that could be Rough for the gamethread.
	//TODO: investigate this thoroughly for perf.
	GameTransformPump = MakeShareable(new TransformUpdatesForGameThread(20024));
	JoltGameSim = MakeShareable(new JOLT::FWorldSimOwner(TickRateInDelta));
	JoltBodyLifecycleOwnerMapping = MakeShareable(new TMap<FBarrageKey, FBLet>());
}

void UBarrageDispatch::Deinitialize()
{
	Super::Deinitialize();
	JoltBodyLifecycleOwnerMapping = nullptr;
	for (auto& x : Tombs)
	{
		x = nullptr;
	}
	GameTransformPump = nullptr;
	
}

void UBarrageDispatch::SphereCast(double Radius, FVector3d CastFrom, uint64_t timestamp)
{
}

//Defactoring the pointer management has actually made this much clearer than I expected.
//these functions are overload polymorphic against our non-polymorphic POD params classes.
//this is because over time, the needs of these classes may diverge and multiply
//and it's not clear to me that Shapefulness is going to actually be the defining shared
//feature. I'm going to wait to refactor the types until testing is complete.
FBLet UBarrageDispatch::CreatePrimitive(FBBoxParams& Definition, ObjectKey OutKey, uint16 Layer)
{
	auto temp = JoltGameSim->CreatePrimitive(Definition, Layer);
	return ManagePointers(OutKey, temp, FBarragePrimitive::Box);
}

FBLet UBarrageDispatch::CreatePrimitive(FBSphereParams& Definition, ObjectKey OutKey, uint16 Layer)
{
	auto temp = JoltGameSim->CreatePrimitive(Definition, Layer);
	return ManagePointers(OutKey, temp, FBarragePrimitive::Sphere);
}
FBLet UBarrageDispatch::CreatePrimitive(FBCapParams& Definition, ObjectKey OutKey, uint16 Layer)
{
	auto temp = JoltGameSim->CreatePrimitive(Definition, Layer);
	return ManagePointers(OutKey, temp, FBarragePrimitive::Capsule);
}

FBLet UBarrageDispatch::ManagePointers(ObjectKey OutKey, FBarrageKey temp, FBarragePrimitive::FBShape form)
{
	auto indirect = MakeShareable(new FBarragePrimitive(temp, OutKey));
	indirect.Object->Me = form;
	JoltBodyLifecycleOwnerMapping->Add(indirect.Object->KeyIntoBarrage, indirect);
	return indirect;
}

//https://github.com/jrouwe/JoltPhysics/blob/master/Samples/Tests/Shapes/MeshShapeTest.cpp
//probably worth reviewing how indexed triangles work, too : https://www.youtube.com/watch?v=dOjZw5VU6aM
FBLet UBarrageDispatch::LoadComplexStaticMesh(FBMeshParams& Definition,
	const UStaticMeshComponent* StaticMeshComponent, ObjectKey Outkey, FBarrageKey& InKey)
{
	auto shared =  JoltGameSim->LoadComplexStaticMesh(Definition, StaticMeshComponent, Outkey, InKey);
	JoltBodyLifecycleOwnerMapping->Add(InKey, shared);
	return shared;
}
//here's the same code, broadly, from War Thunder:
	/*
	    case PhysCollision::TYPE_TRIMESH:
    {
      auto meshColl = static_cast<const PhysTriMeshCollision *>(c);
      JPH::MeshShapeSettings shape;
      shape.mTriangleVertices.resize(meshColl->vnum);
      shape.mIndexedTriangles.resize(meshColl->inum / 3);
      Point3 scl = meshColl->scale;
      int vstride = meshColl->vstride;
      bool rev_face = meshColl->revNorm;

      if (meshColl->vtypeShort)
      {
        JPH::Float3 *d = shape.mTriangleVertices.data();
        for (auto s = (const char *)meshColl->vdata, se = s + meshColl->vnum * vstride; s < se; s += vstride, d++)
          d->x = ((uint16_t *)s)[0] * scl.x, d->y = ((uint16_t *)s)[1] * scl.y, d->z = ((uint16_t *)s)[2] * scl.z;
      }
      else
      {
        JPH::Float3 *d = shape.mTriangleVertices.data();
        for (auto s = (const char *)meshColl->vdata, se = s + meshColl->vnum * vstride; s < se; s += vstride, d++)
          d->x = ((float *)s)[0] * scl.x, d->y = ((float *)s)[1] * scl.y, d->z = ((float *)s)[2] * scl.z;
      }
      if (meshColl->istride == 2)
      {
        JPH::IndexedTriangle *d = shape.mIndexedTriangles.data();
        for (auto s = (const unsigned short *)meshColl->idata, se = s + meshColl->inum; s < se; s += 3, d++)
          d->mIdx[0] = s[0], d->mIdx[1] = s[rev_face ? 2 : 1], d->mIdx[2] = s[rev_face ? 1 : 2];
      }
      else
      {
        JPH::IndexedTriangle *d = shape.mIndexedTriangles.data();
        for (auto s = (const unsigned *)meshColl->idata, se = s + meshColl->inum; s < se; s += 3, d++)
          d->mIdx[0] = s[0], d->mIdx[1] = s[rev_face ? 2 : 1], d->mIdx[2] = s[rev_face ? 1 : 2];
      }

      auto res = shape.Create();
      if (DAGOR_LIKELY(res.IsValid()))
        return res.Get();

      logerr("Failed to create non sanitized mesh shape <%s>: %s", meshColl->debugName, res.GetError().c_str());

      decltype(shape) sanitizedShape;
      sanitizedShape.mTriangleVertices = eastl::move(shape.mTriangleVertices);
      sanitizedShape.mIndexedTriangles = eastl::move(shape.mIndexedTriangles);
      sanitizedShape.Sanitize();
      return check_and_return_shape(sanitizedShape.Create(), __LINE__);
    }
	*/


//unlike our other ecs components in artillery, barrage dispatch does not maintain the mappings directly.
//this is because we may have _many_ jolt sims running if we choose to do deterministic rollback in certain ways.
//This is a copy by value return on purpose, as we want the ref count to rise.
FBLet UBarrageDispatch::GetShapeRef(FBarrageKey Existing) const
{
	//SharedPTR's def val is nullptr. this will return nullptr as soon as entomb succeeds.
	//if entomb gets sliced, the tombstone check will fail as long as it is performed within 27 milliseconds of this call.
	//as a result, one of three states will arise:
	//1) you get a null pointer
	//2) you get a valid shared pointer which will hold the asset open until you're done, but the tombstone markings will be set, letting you know
	//that this thoroughfare? it leads into the region of peril.
	//3) you get a valid shared pointer which will hold the asset open until you're done, but the markings are being set
	//this means your calls will all succeed but none will be applied during the apply shadow phase.
	return JoltBodyLifecycleOwnerMapping->FindRef(Existing);
}

void UBarrageDispatch::FinalizeReleasePrimitive(FBarrageKey BarrageKey)
{
	auto HoldOpen = JoltGameSim;
	if(HoldOpen && JoltGameSim)
	{
		HoldOpen->FinalizeReleasePrimitive(BarrageKey);
	}
}


TStatId UBarrageDispatch::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBarrageDispatch, STATGROUP_Tickables);
}

void UBarrageDispatch::StackUp()
{
	auto WorldSimOwner = JoltGameSim;
	if(WorldSimOwner)
	{
		for(auto& x : WorldSimOwner->ThreadAcc)
		{
			TSharedPtr<JOLT::FWorldSimOwner::ThreadFeed> HoldOpen;
			if( x.Queue && ((HoldOpen = x.Queue)) && x.That != std::thread::id()) //if there IS a thread.
			{
				while (HoldOpen && !HoldOpen->IsEmpty())
				{
					auto input = HoldOpen->Peek();
					auto bID = WorldSimOwner->BarrageToJoltMapping->Find(input->Target->KeyIntoBarrage);
					if(input->Action == PhysicsInputType::Rotation)
					{
						//prolly gonna wanna change this to add torque................... not sure.
						WorldSimOwner->body_interface->SetRotation(*bID, input->State, JPH::EActivation::Activate);
					}
					else if (input->Action == PhysicsInputType::OtherForce)
					{
						WorldSimOwner->body_interface->AddForce(*bID, input->State.GetXYZ(), JPH::EActivation::Activate);
					}
					else if (input->Action == PhysicsInputType::Velocity)
					{
						WorldSimOwner->body_interface->AddForce(*bID, input->State.GetXYZ(), JPH::EActivation::Activate);
					}
					else if(input->Action == PhysicsInputType::SelfMovement)
					{
						WorldSimOwner->body_interface->AddForce(*bID, input->State.GetXYZ(), JPH::EActivation::Activate);
					}
					HoldOpen->Dequeue();
				}
			}
			
		}
	}
}

void UBarrageDispatch::StepWorld(uint64 Time)
{
	auto HoldOpenWorld = JoltGameSim;
	if(HoldOpenWorld)
	{
		JoltGameSim->StepSimulation();
		//maintain tombstones
		CleanTombs();
		auto HoldOpen = JoltBodyLifecycleOwnerMapping;
		if(HoldOpen != nullptr)
		{
			for(auto& x : *HoldOpen.Get())
			{
				auto RefHoldOpen = x.Value;
				if(RefHoldOpen && RefHoldOpen.IsValid() && FBarragePrimitive::IsNotNull(RefHoldOpen))
				{
					FBarragePrimitive::TryGetTransformFromJolt(x.Value, Time); //returns a bool that can be used for debug.
				}
			}
		}
	}
}


//Bounds are OPAQUE. do not reference them. they are protected for a reason, because they are
//subject to semantic changes. the Point is left in the UE space. 
FBBoxParams FBarrageBounder::GenerateBoxBounds(FVector3d point, double xDiam,
	double yDiam, double zDiam)
{
	FBBoxParams blob;
	blob.point = point;
	blob.JoltX = CoordinateUtils::DiamToJoltHalfExtent(xDiam);
	blob.JoltY = CoordinateUtils::DiamToJoltHalfExtent(zDiam); //this isn't a typo.
	blob.JoltZ = CoordinateUtils::DiamToJoltHalfExtent(yDiam);
	return blob;
}
//Bounds are OPAQUE. do not reference them. they are protected for a reason, because they are
//subject to change. the Point is left in the UE space. 
FBSphereParams FBarrageBounder::GenerateSphereBounds(FVector3d point, double radius)
{
	FBSphereParams blob;
	blob.point = point;
	blob.JoltRadius = CoordinateUtils::RadiusToJolt(radius);
	return blob;
}
//Bounds are OPAQUE. do not reference them. they are protected for a reason, because they are
//subject to change. the Point is left in the UE space, signified by the UE type. 
FBCapParams FBarrageBounder::GenerateCapsuleBounds(UE::Geometry::FCapsule3d Capsule)
{
	FBCapParams blob;
	blob.point = Capsule.Center();
	blob.JoltRadius = CoordinateUtils::RadiusToJolt(Capsule.Radius);
	blob.JoltHalfHeightOfCylinder = CoordinateUtils::RadiusToJolt(Capsule.Extent());
	return blob;
}

