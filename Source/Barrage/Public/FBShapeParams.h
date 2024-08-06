#pragma once

//shape params are short-lived, so we're okay to actually be fairly elegant at the cost of some sanity.
//it turns out to be really important that this is a POD with NO dependencies. it makes using barrage vastly easier until
//we build a type lib plugin
//remember to convert https://youtu.be/jhCupKFly_M?si=aoBCNbbAA9DzDPyy&t=438
struct FBShapeParams
{
	//we'll need to add mesh.
	enum FBShape
	{
		Capsule,
		Box,
		Sphere
	};

	//All shapes can be defined using just these bounds. actually, this is more than you need for a sphere but hey
	//maybe we'll do oblates or fuzzy spheres. actually, I think that we'll need a weird-sphere of some sort for numerical
	//stability. @JPOPHAM?
	FVector3d Point;
	double bound1;
	double bound2;
	double bound3; //calculate your own halfex. :|
	unsigned short layer;
	FBShape MyShape;
};
