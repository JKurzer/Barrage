#include "FWorldSimOwner.h"
#include "CoordinateUtils.h"

//we need the coordinate utils, but we don't really want to include them in the .h
inline FBarrageKey FWorldSimOwner::CreatePrimitive(FBShapeParams& ToCreate)
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
		BodyCreationSettings box_body_settings(box_shape, CoordinateUtils::ToJoltCoordinates(ToCreate.point), Quat::sIdentity(), MovementType, ToCreate.layer);
		// Create the actual rigid body
		Body* box_body = body_interface->CreateBody(box_body_settings); // Note that if we run out of bodies this can return nullptr

		// Add it to the world
		body_interface->AddBody(box_body->GetID(), EActivation::Activate);
		BodyIDTemp = floor->GetID();
	}
	else if(ToCreate.MyShape == ToCreate.Sphere)
	{
		BodyCreationSettings sphere_settings(new SphereShape(ToCreate.bound1),
		                                     CoordinateUtils::ToJoltCoordinates(ToCreate.point),
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
		throw; // static mesh is NOT a primitive type
	}
			
	KeyCompose |= BodyIDTemp.GetIndexAndSequenceNumber();
	//Barrage key is unique to WORLD and BODY. This is crushingly important.
	BarrageToJoltMapping->Add(static_cast<FBarrageKey>(KeyCompose), BodyIDTemp);

	return static_cast<FBarrageKey>(KeyCompose);
}
