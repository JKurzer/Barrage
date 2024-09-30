#include "BarrageDispatch.h"

#include "BarrageContactListener.h"
#include "IsolatedJoltIncludes.h"

#include "FWorldSimOwner.h"
#include "CoordinateUtils.h"
#include "FBPhysicsInput.h"
#include "Jolt/Physics/Collision/ShapeCast.h"

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

	UE_LOG(LogTemp, Warning, TEXT("Barrage:TransformUpdateQueue: Online"));
	GameTransformPump = MakeShareable(new TransformUpdatesForGameThread(20024));
	FBarragePrimitive::GlobalBarrage = this;
	//this approach may actually be too slow. it is pleasingly lockless, but it allocs 16megs
	//and just iterating through that could be Rough for the gamethread.
	//TODO: investigate this thoroughly for perf.

	//TODO: Oh look we did it again except this time for contact events
	//But limiting this one to 4004 capacity (I just chose a number)
	ContactEventPump = MakeShareable(new TCircularQueue<BarrageContactEvent>(4004));
	JoltGameSim = MakeShareable(new JOLT::FWorldSimOwner(TickRateInDelta, MakeShareable<JOLT::BarrageContactListener>(new JOLT::BarrageContactListener(this))));
	JoltBodyLifecycleMapping = MakeShareable(new KeyToFBLet());
	TranslationMapping = MakeShareable(new KeyToKey());

	UE_LOG(LogTemp, Warning, TEXT("Barrage:Subsystem: Online"));
}

void UBarrageDispatch::OnWorldBeginPlay(UWorld& InWorld)
{

	Super::OnWorldBeginPlay(InWorld);

}

void UBarrageDispatch::Deinitialize()
{
	Super::Deinitialize();
	JoltBodyLifecycleMapping = nullptr;
	TranslationMapping = nullptr;
	for (auto& x : Tombs)
	{
		x = nullptr;
	}
	auto HoldOpen = GameTransformPump;
	GameTransformPump = nullptr;
	if (HoldOpen)
	{
		auto val = HoldOpen.GetSharedReferenceCount();
		if (val > 1)
		{
			UE_LOG(LogTemp, Warning,
			       TEXT(
				       "Hey, so something's holding live references to the queue. Maybe. Shared Ref Count is not reliable."
			       ));
		}
		HoldOpen->Empty();
	}
	HoldOpen = nullptr;

	auto HoldOpen2 = ContactEventPump;
	ContactEventPump = nullptr;
	if (HoldOpen2)
	{
		auto val = HoldOpen2.GetSharedReferenceCount();
		if (val > 1)
		{
			UE_LOG(LogTemp, Warning,
				   TEXT(
					   "Hey, so something's holding live references to the queue. Maybe. Shared Ref Count is not reliable."
				   ));
		}
		HoldOpen2->Empty();
	}
	HoldOpen2 = nullptr;
}

void UBarrageDispatch::SphereCast(
	FBarrageKey ShapeSource,
	double Radius,
	double Distance,
	FVector3d CastFrom,
	FVector3d Direction,
	TSharedPtr<FHitResult> OutHit,
	uint64_t timestamp)
{
	auto HoldOpen = JoltGameSim;
	if (HoldOpen) {
		auto bodyID = HoldOpen->BarrageToJoltMapping->Find(ShapeSource);
		HoldOpen->SphereCast(Radius, Distance, CastFrom, Direction, *bodyID, OutHit);
	}
}

//Defactoring the pointer management has actually made this much clearer than I expected.
//these functions are overload polymorphic against our non-polymorphic POD params classes.
//this is because over time, the needs of these classes may diverge and multiply
//and it's not clear to me that Shapefulness is going to actually be the defining shared
//feature. I'm going to wait to refactor the types until testing is complete.
FBLet UBarrageDispatch::CreatePrimitive(FBBoxParams& Definition, FSkeletonKey OutKey, uint16_t Layer, bool isSensor)
{
	auto HoldOpen = JoltGameSim;
	if (HoldOpen)
	{
		auto temp = HoldOpen->CreatePrimitive(Definition, Layer, isSensor);
		return ManagePointers(OutKey, temp, FBarragePrimitive::Box);
	}
	return FBLet();
}

//TODO: COMPLETE MOCK
FBLet UBarrageDispatch::CreatePrimitive(FBCharParams& Definition, FSkeletonKey OutKey, uint16_t Layer)
{
	auto HoldOpen = JoltGameSim;
	if (HoldOpen)
	{
		auto temp = HoldOpen->CreatePrimitive(Definition, Layer);
		return ManagePointers(OutKey, temp, FBarragePrimitive::Character);
	}
	return FBLet();
}

FBLet UBarrageDispatch::CreatePrimitive(FBSphereParams& Definition, FSkeletonKey OutKey, uint16_t Layer, bool isSensor)
{
	auto HoldOpen = JoltGameSim;
	if (HoldOpen)
	{
		auto temp = HoldOpen->CreatePrimitive(Definition, Layer, isSensor);
		return ManagePointers(OutKey, temp, FBarragePrimitive::Sphere);
	}
	return FBLet();
}

FBLet UBarrageDispatch::CreatePrimitive(FBCapParams& Definition, FSkeletonKey OutKey, uint16 Layer, bool isSensor)
{
	auto HoldOpen = JoltGameSim;
	if (HoldOpen)
	{
		auto temp = HoldOpen->CreatePrimitive(Definition, Layer, isSensor);
		return ManagePointers(OutKey, temp, FBarragePrimitive::Capsule);
	}
	return FBLet();
}

FBLet UBarrageDispatch::ManagePointers(FSkeletonKey OutKey, FBarrageKey temp, FBarragePrimitive::FBShape form)
{
	//interestingly, you can't use auto here. don't try. it may allocate a raw pointer internal
	//and that will get stored in the jolt body lifecycle mapping. 
	//it basically ensures that you will get turned into a pillar of salt.
	FBLet indirect = MakeShareable(new FBarragePrimitive(temp, OutKey));
	indirect->Me = form;
	JoltBodyLifecycleMapping->insert_or_assign (indirect->KeyIntoBarrage, indirect);
	TranslationMapping->insert_or_assign(indirect->KeyOutOfBarrage, indirect->KeyIntoBarrage);
	return indirect;
}

//https://github.com/jrouwe/JoltPhysics/blob/master/Samples/Tests/Shapes/MeshShapeTest.cpp
//probably worth reviewing how indexed triangles work, too : https://www.youtube.com/watch?v=dOjZw5VU6aM
FBLet UBarrageDispatch::LoadComplexStaticMesh(FBMeshParams& Definition,
                                              const UStaticMeshComponent* StaticMeshComponent, FSkeletonKey Outkey)
{
	auto HoldOpen = JoltGameSim;
	if (HoldOpen)
	{
		auto shared = HoldOpen->LoadComplexStaticMesh(Definition, StaticMeshComponent, Outkey);
		if(shared)
		{
			JoltBodyLifecycleMapping->insert_or_assign(shared->KeyIntoBarrage, shared);
			TranslationMapping->insert_or_assign(Outkey, shared->KeyIntoBarrage);
		}
		return shared;
	}
	return FBLet();
}

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
	auto holdopen = JoltBodyLifecycleMapping;

	FBLet out;
	if(holdopen && holdopen->find(Existing, out))
	{
		return out;
	}
	return FBLet();
}
FBLet UBarrageDispatch::GetShapeRef(FSkeletonKey Existing) const
{
	//SharedPTR's def val is nullptr. this will return nullptr as soon as entomb succeeds.
	//if entomb gets sliced, the tombstone check will fail as long as it is performed within 27 milliseconds of this call.
	//as a result, one of three states will arise:
	//1) you get a null pointer
	//2) you get a valid shared pointer which will hold the asset open until you're done, but the tombstone markings will be set, letting you know
	//that this thoroughfare? it leads into the region of peril.
	//3) you get a valid shared pointer which will hold the asset open until you're done, but the markings are being set
	//this means your calls will all succeed but none will be applied during the apply shadow phase.
	auto holdopen = TranslationMapping;
	if(holdopen)
	{
		FBarrageKey key;
		auto ref = TranslationMapping->find(Existing, key);
		if(ref)
		{
			auto HoldMapping = JoltBodyLifecycleMapping;
			FBLet deref;
			if(HoldMapping->find(key, deref))
			{
				if(deref && FBarragePrimitive::IsNotNull(deref))
				{
					return deref;
				}
			}
		}
	}
	return FBLet();
}

void UBarrageDispatch::FinalizeReleasePrimitive(FBarrageKey BarrageKey)
{
	auto HoldOpen = JoltGameSim;
	if (HoldOpen)
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
	//currently, these are only characters but that could change. This would likely become a TMap then but maybe not.
	if (WorldSimOwner)
	{
		for (auto& x : WorldSimOwner->ThreadAcc)
		{
			TSharedPtr<JOLT::FWorldSimOwner::ThreadFeed> HoldOpen;
			if (x.Queue && ((HoldOpen = x.Queue)) && x.That != std::thread::id()) //if there IS a thread.
			{
				while (HoldOpen && !HoldOpen->IsEmpty())
				{
					auto input = HoldOpen->Peek();
					auto bID = WorldSimOwner->BarrageToJoltMapping->Find(input->Target->KeyIntoBarrage);
					if (input->Target->Me == FBarragePrimitive::Character)
					{
						UpdateCharacter(const_cast<FBPhysicsInput&>(*input));
					}
					else if (input->Action == PhysicsInputType::Rotation)
					{
						//prolly gonna wanna change this to add torque................... not sure.
						WorldSimOwner->body_interface->SetRotation(*bID, input->State, JPH::EActivation::Activate);
					}
					else if (input->Action == PhysicsInputType::OtherForce)
					{
						WorldSimOwner->body_interface->
						               AddForce(*bID, input->State.GetXYZ(), JPH::EActivation::Activate);
					}
					else if (input->Action == PhysicsInputType::Velocity)
					{
						WorldSimOwner->body_interface->
						               SetLinearVelocity(*bID, input->State.GetXYZ());
					}
					else if (input->Action == PhysicsInputType::SelfMovement)
					{
						WorldSimOwner->body_interface->
						               AddForce(*bID, input->State.GetXYZ(), JPH::EActivation::Activate);
					}
					HoldOpen->Dequeue();
				}

			}
		}
	}
}

bool UBarrageDispatch::UpdateCharacters(TSharedPtr<TArray<FBPhysicsInput>> CharacterInputs)
{
	return JoltGameSim->UpdateCharacters(CharacterInputs);
}

bool UBarrageDispatch::UpdateCharacter(FBPhysicsInput& CharacterInput)
{
	return JoltGameSim->UpdateCharacter(CharacterInput);
}

void UBarrageDispatch::StepWorld(uint64 Time)
{
	auto HoldOpenWorld = JoltGameSim;
	if (HoldOpenWorld)
	{
		JoltGameSim->StepSimulation();
		auto HoldOpenCharacters = JoltGameSim->CharacterToJoltMapping;
		if(HoldOpenCharacters)
		{
			for(auto x : *HoldOpenCharacters)
			{
				if(x.Value->mCharacter && !x.Value->mCharacter->GetPosition().IsNaN())
				{
					x.Value->StepCharacter();
				}
				else if (x.Value->mCharacter)
				{
					x.Value->mCharacter->SetLinearVelocity(x.Value->World->GetGravity());
					x.Value->mCharacter->SetPosition(x.Value->mInitialPosition);
					x.Value->mForcesUpdate = x.Value->World->GetGravity();
					x.Value->StepCharacter();
				}
			}
		}
		//maintain tombstones
		CleanTombs();
		auto HoldOpen = JoltBodyLifecycleMapping;
		if (HoldOpen && HoldOpen.Get() && !HoldOpen.Get()->empty())
		{
			for (auto& x : HoldOpen->lock_table())
			{
				;
				if (x.first.KeyIntoBarrage != 0 && FBarragePrimitive::IsNotNull(x.second))
				{
						FBarragePrimitive::TryUpdateTransformFromJolt(x.second, Time);
						if(x.second->tombstone)
						{
							//TODO: MAY NOT BE THREADSAFE. CHECK NLT 10/5/24
							Entomb(x.second);
						}
					//returns a bool that can be used for debug.
				}
			}
		}
	}
}

bool UBarrageDispatch::BroadcastContactEvents()
{
	if(GetWorld())
	{
		auto HoldOpen = ContactEventPump;

		while(HoldOpen && !HoldOpen->IsEmpty())
		{
			auto Update = HoldOpen->Peek();
			if(Update)
			{
				try
				{
					switch (Update->ContactEventType)
					{
						case EBarrageContactEventType::ADDED:
							OnBarrageContactAddedDelegate.Broadcast(*Update);
							break;
						case EBarrageContactEventType::PERSISTED:
							OnBarrageContactPersistedDelegate.Broadcast(*Update);
							break;
						case EBarrageContactEventType::REMOVED:
							OnBarrageContactRemovedDelegate.Broadcast(*Update);
							break;
					}
					HoldOpen->Dequeue();
				}
				catch (...)
				{
					return false; //we'll be back! we'll be back!!!!
				}
			}
		}
		return true;
	}
	return false;
}

inline BarrageContactEvent ConstructContactEvent(EBarrageContactEventType EventType, UBarrageDispatch* BarrageDispatch, const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold,
									ContactSettings& ioSettings)
{
	auto Body1Let = BarrageDispatch->GetShapeRef(BarrageDispatch->GenerateBarrageKeyFromBodyId(inBody1.GetID()));
	auto Body2Let = BarrageDispatch->GetShapeRef(BarrageDispatch->GenerateBarrageKeyFromBodyId(inBody2.GetID()));

	return BarrageContactEvent(EventType, BarrageContactEntity(Body1Let->KeyOutOfBarrage, inBody1), BarrageContactEntity(Body2Let->KeyOutOfBarrage, inBody2));
}

void UBarrageDispatch::HandleContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold,
									ContactSettings& ioSettings)
{
	if (!JoltBodyLifecycleMapping.IsValid())
	{
		return;
	}
	
	auto ContactEventToEnqueue = ConstructContactEvent(EBarrageContactEventType::ADDED, this, inBody1, inBody2, inManifold, ioSettings);
	ContactEventPump->Enqueue(ContactEventToEnqueue);
}
void UBarrageDispatch::HandleContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold,
									ContactSettings& ioSettings)
{
	if (!JoltBodyLifecycleMapping.IsValid())
	{
		return;
	}
	auto ContactEventToEnqueue = ConstructContactEvent(EBarrageContactEventType::PERSISTED, this, inBody1, inBody2, inManifold, ioSettings);
	ContactEventPump->Enqueue(ContactEventToEnqueue);
}
void UBarrageDispatch::HandleContactRemoved(const SubShapeIDPair& inSubShapePair)
{
	if (!JoltBodyLifecycleMapping.IsValid())
	{
		return;
	}
	auto Body1Let = this->GetShapeRef(this->GenerateBarrageKeyFromBodyId(inSubShapePair.GetBody1ID()));
	auto Body2Let = this->GetShapeRef(this->GenerateBarrageKeyFromBodyId(inSubShapePair.GetBody2ID()));

	auto ContactEventToEnqueue = BarrageContactEvent(EBarrageContactEventType::REMOVED, BarrageContactEntity(Body1Let->KeyOutOfBarrage), BarrageContactEntity(Body2Let->KeyOutOfBarrage));
	ContactEventPump->Enqueue(ContactEventToEnqueue);
}

FBarrageKey UBarrageDispatch::GenerateBarrageKeyFromBodyId(const JPH::BodyID& Input) const
{
	return JoltGameSim->GenerateBarrageKeyFromBodyId(Input);
};

FBarrageKey UBarrageDispatch::GenerateBarrageKeyFromBodyId(const uint32 RawIndexAndSequenceNumberInput) const
{
	return JoltGameSim->GenerateBarrageKeyFromBodyId(RawIndexAndSequenceNumberInput);
};

//Bounds are OPAQUE. do not reference them. they are protected for a reason, because they are
//subject to semantic changes. the Point is left in the UE space. 
FBBoxParams FBarrageBounder::GenerateBoxBounds(FVector3d point, double xDiam,
                                               double yDiam, double zDiam, FVector3d OffsetCenterToMatchBoundedShape)
{
	FBBoxParams blob;
	blob.point = point;
	blob.euler_angle = 0;
	blob.OffsetX = OffsetCenterToMatchBoundedShape.X;
	blob.OffsetY = OffsetCenterToMatchBoundedShape.Y;
	blob.OffsetZ = OffsetCenterToMatchBoundedShape.Z;
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

//Bounds are OPAQUE. do not reference them. they are protected for a reason, because they are
//subject to change. the Point is left in the UE space, signified by the UE type. 
FBCharParams FBarrageBounder::GenerateCharacterBounds(FVector3d point, double radius, double extent, double speed)
{
	FBCharParams blob;
	blob.point = point;
	blob.JoltRadius = CoordinateUtils::RadiusToJolt(radius);
	blob.JoltHalfHeightOfCylinder = CoordinateUtils::RadiusToJolt(extent);
	blob.speed = speed;
	return blob;
}