#include "BarrageDispatch.h"
#include "Chaos/Particles.h"
#include "FWorldSimOwner.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "PhysicsEngine/BodySetup.h"
#include "CoordinateUtils.h"
#include "Runtime/Experimental/Chaos/Private/Chaos/PhysicsObjectInternal.h"

//https://github.com/GaijinEntertainment/DagorEngine/blob/71a26585082f16df80011e06e7a4e95302f5bb7f/prog/engine/phys/physJolt/joltPhysics.cpp#L800
//this is how gaijin uses jolt, and war thunder's honestly a pretty strong comp to our use case.


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
}

void UBarrageDispatch::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	JoltGameSim = MakeShareable(new FWorldSimOwner(TickRateInDelta));
	BodyLifecycleOwner = MakeShareable(new TMap<FBarrageKey, FBLet>());
}

void UBarrageDispatch::Deinitialize()
{
	Super::Deinitialize();
	BodyLifecycleOwner = nullptr;
	for (auto& x : Tombs)
	{
		x = nullptr;
	}
}

void UBarrageDispatch::SphereCast(double Radius, FVector3d CastFrom, uint64_t timestamp)
{
}

//Defactoring the pointer management has actually made this much clearer than I expected.
//these functions are overload polymorphic against our non-polymorphic POD params classes.
//this is because over time, the needs of these classes may diverge and multiply
//and it's not clear to me that Shapefulness is going to actually be the defining shared
//feature. I'm going to wait to refactor the types until testing is complete.
FBLet UBarrageDispatch::CreatePrimitive(FBBoxParams& Definition, uint64 OutKey, uint16 Layer)
{
	auto temp = JoltGameSim->CreatePrimitive(Definition, Layer);
	return ManagePointers(OutKey, temp, FBarragePrimitive::Box);
}

FBLet UBarrageDispatch::CreatePrimitive(FBSphereParams& Definition, uint64 OutKey, uint16 Layer)
{
	auto temp = JoltGameSim->CreatePrimitive(Definition, Layer);
	return ManagePointers(OutKey, temp, FBarragePrimitive::Sphere);
}
FBLet UBarrageDispatch::CreatePrimitive(FBCapParams& Definition, uint64 OutKey, uint16 Layer)
{
	auto temp = JoltGameSim->CreatePrimitive(Definition, Layer);
	return ManagePointers(OutKey, temp, FBarragePrimitive::Capsule);
}

FBLet UBarrageDispatch::ManagePointers(uint64 OutKey, FBarrageKey temp, FBarragePrimitive::FBShape form)
{
	auto indirect = MakeShareable(new FBarragePrimitive(temp, OutKey));
	indirect.Object->Me = form;
	BodyLifecycleOwner->Add(indirect.Object->KeyIntoBarrage, indirect);
	return indirect;
}

//https://github.com/jrouwe/JoltPhysics/blob/master/Samples/Tests/Shapes/MeshShapeTest.cpp
//probably worth reviewing how indexed triangles work, too : https://www.youtube.com/watch?v=dOjZw5VU6aM
FBLet UBarrageDispatch::LoadComplexStaticMesh(FBMeshParams& Definition,
	const UStaticMeshComponent* StaticMeshComponent, uint64 Outkey, FBarrageKey& InKey)
{
	using ParticlesType = Chaos::TParticles<Chaos::FRealSingle, 3>;
	using ParticleVecType = Chaos::TVec3<Chaos::FRealSingle>;
	using ::CoordinateUtils;
	if(!StaticMeshComponent) return nullptr;
	if(!StaticMeshComponent->GetStaticMesh()) return nullptr;
	if(!StaticMeshComponent->GetStaticMesh()->GetRenderData()) return nullptr;
	UBodySetup* body = StaticMeshComponent->GetStaticMesh()->GetBodySetup();
	if(!body || body->CollisionTraceFlag != CTF_UseComplexAsSimple)
	{
		return nullptr; // we don't accept anything but complex or primitive yet.
		//simple collision tends to use primitives, in which case, don't call this
		//or compound shapes which will get added back in.
	}

	auto& complex = StaticMeshComponent->GetStaticMesh()->ComplexCollisionMesh;
	auto collbody = complex->GetBodySetup();
	if(collbody == nullptr)
	{
		return nullptr;
	}

	//Here we go!
	auto& MeshSet = collbody->TriMeshGeometries;
	JPH::VertexList JoltVerts;
	JPH::IndexedTriangleList JoltIndexedTriangles;
	uint32 tris = 0;
	for(auto& Mesh : MeshSet)
	{
		tris += Mesh->Elements().GetNumTriangles();
	}
	JoltVerts.reserve(tris);
	JoltIndexedTriangles.reserve(tris);
	for( auto& Mesh : MeshSet)
	{
		//indexed triangles are made by collecting the vertexes, then generating triples describing the triangles.
		//this allows the heavier vertices to be stored only once, rather than each time they are used. for large models
		//like terrain, this can be extremely significant. though, it's not truly clear to me if it's worth it.
		auto& VertToTriBuffers = Mesh->Elements();
		auto& Verts = Mesh->Particles().X();
		if(VertToTriBuffers.RequiresLargeIndices())
		{
			for(auto& aTri : VertToTriBuffers.GetLargeIndexBuffer())
			{
				JoltIndexedTriangles.push_back(IndexedTriangle(aTri[2], aTri[1], aTri[0]));
			}
		}
		else
		{
			for(auto& aTri : VertToTriBuffers.GetSmallIndexBuffer())
			{
				JoltIndexedTriangles.push_back(IndexedTriangle(aTri[2], aTri[1], aTri[0]));
			}
		}

		for(auto& vtx : Verts)
		{
			//need to figure out how to defactor this without breaking typehiding or having to create a bunch of util.h files.
			//though, tbh, the util.h is the play. TODO: util.h ?
			JoltVerts.push_back(CoordinateUtils::ToJoltCoordinates(vtx));
		}
	}
	JPH::MeshShapeSettings FullMesh(JoltVerts, JoltIndexedTriangles);
	//just the last boiler plate for now.
	JPH::ShapeSettings::ShapeResult err = FullMesh.Create();
	if(err.HasError())
	{
		return nullptr;
	}
	//TODO: should we be holding the shape ref in gamesim owner?
	auto& shape = err.Get();
	BodyCreationSettings meshbody;
	BodyCreationSettings creation_settings;
	creation_settings.mMotionType = EMotionType::Static;
	creation_settings.mObjectLayer = Layers::NON_MOVING;
	creation_settings.mPosition = CoordinateUtils::ToJoltCoordinates(Definition.point);
	creation_settings.mFriction = 0.5f;
	creation_settings.SetShape(shape);
	auto bID = JoltGameSim->body_interface->CreateAndAddBody(creation_settings, EActivation::Activate);

	JoltGameSim->BarrageToJoltMapping->Add(InKey, bID);
	
	auto shared = MakeShareable(new FBarragePrimitive(InKey, Outkey));
	BodyLifecycleOwner->Add(InKey, shared);
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
	//as a result, one of two states will arise: you get a null pointer, or you get a valid shared pointer
	//which will hold the asset open until you're done, but the tombstone markings will be set, letting you know
	//that this thoroughfare? it leads into the region of peril.
	return BodyLifecycleOwner->FindRef(Existing);
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

//Bounds are OPAQUE. do not reference them. they are protected for a reason, because they are
//subject to change. the Point is left in the UE space. 
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

