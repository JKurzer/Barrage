#pragma once
#include "CapsuleTypes.h"


//Hey, so you might notice that this is exactly 56 bytes and ordered oddly.
//That's because 56 bytes packs exactly the same on pack 4 or pack 8
//this is important. I'm reasonably sure I've done everything right and ensured that the boundaries are correctly annotated
//and data is marshalled correctly across them, but for the sake of my sanity, I'm also going to make sure this struct is
//identically packed in both domains.

//don't change this unless you're sure it's safe. member size AND order alter packing.
//remember to convert https://youtu.be/jhCupKFly_M?si=aoBCNbbAA9DzDPyy&t=438

//#pragma pack(push, 1) //you can uncomment this to see the size exactly.

//REMINDER: these use UE type conventions and so are in UE space.
//I bounced back and forth on this a bunch.
struct FBShapeParams
{
	//we'll need to add mesh.
	enum FBShape
	{
		Capsule,
		Box,
		Sphere,
		Static
	};

	//All shapes can be defined using just these bounds. actually, this is more than you need for a sphere but hey
	//maybe we'll do oblates or fuzzy spheres. actually, I think that we'll need a weird-sphere of some sort for numerical
	//stability. @JPOPHAM?
FVector3d point;
	double bound1;
	double bound2;
	double bound3; //calculate your own halfex. :|
	uint32_t layer;
	FBShape MyShape;
};
//this should evaluate in most IDEs, allowing you to see the size if you need to make changes. try not to need to.
//constexpr const static int size = sizeof(FBShapeParams);
//#pragma pack(pop) //uncomment along with the push pragma or you'll have a very funny but very bad time

