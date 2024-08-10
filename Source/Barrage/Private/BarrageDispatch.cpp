#include "BarrageDispatch.h"
#include "FWorldSimOwner.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "Chaos/TriangleMesh.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "PhysicsEngine/BodySetup.h"
#include "Runtime/Experimental/Chaos/Private/Chaos/PhysicsObjectInternal.h"

//https://github.com/GaijinEntertainment/DagorEngine/blob/71a26585082f16df80011e06e7a4e95302f5bb7f/prog/engine/phys/physJolt/joltPhysics.cpp#L800
//this is how gaijin uses jolt, and war thunder's honestly a pretty strong comp to our use case.


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


/*
 *
*if(!IsValidLowLevel()) return;
if(!StaticMeshComponent) return;
if(!StaticMeshComponent->StaticMesh) return;
if(!StaticMeshComponent->StaticMesh->RenderData) return;

if(StaticMeshComponent->StaticMesh->RenderData->LODResources.Num() > 0)
{`
FPositionVertexBuffer* VertexBuffer = &StaticMeshComponent->StaticMesh->RenderData->LODResources[0].PositionVertexBuffer;

const FVector WorldSpaceVertexLocation = **GetActorLocation() + GetTransform().TransformVector(VertexBuffer->VertexPosition(Index));**

*/

//https://github.com/jrouwe/JoltPhysics/blob/master/Samples/Tests/Shapes/MeshShapeTest.cpp
//probably worth reviewing how indexed triangles work, too : https://www.youtube.com/watch?v=dOjZw5VU6aM
FBLet UBarrageDispatch::LoadStaticMeshLoadStaticMesh(FBShapeParams& Definition,
	const UStaticMeshComponent* StaticMeshComponent, uint64 Outkey)
{
	if(!StaticMeshComponent) return nullptr;
	if(!StaticMeshComponent->GetStaticMesh()) return nullptr;
	if(!StaticMeshComponent->GetStaticMesh()->GetRenderData()) return nullptr;
	UBodySetup* body = StaticMeshComponent->GetStaticMesh()->GetBodySetup();
	if(!body || body->CollisionTraceFlag != CTF_UseComplexAsSimple)
	{
		return nullptr; // we don't accept simple vs complex yet.
	}

	auto& complex = StaticMeshComponent->GetStaticMesh()->ComplexCollisionMesh;
	auto collbody = complex->GetBodySetup();
	if(collbody == nullptr)
	{
		return nullptr;
	}

	//Here we go!
	auto& MeshSet = collbody->ChaosTriMeshes;
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
		auto& VertToTriMap = Mesh->Elements();
		auto& Verts = Mesh->Particles().X();
		for(auto& aTri : VertToTriMap)
		{
			JoltIndexedTriangles.push_back(IndexedTriangle(aTri[2], aTri[1], aTri[0]));
		}
		for(auto& vtx : Verts)
		{
			JoltVerts.push_back(Float3(vtx.X/100.0, vtx.Z/100.0, vtx.Y/100.0));
		}
	}
	JPH::MeshShapeSettings FullMesh(JoltVerts, JoltIndexedTriangles);
	//just the last boiler plate for now.

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
