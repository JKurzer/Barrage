#pragma once

//NOTE: If these are a serious memory drawdown, the code elsewhere is written such that they can be fully decomposed
//into their sub classes and the layer parameter can be inferred. This allows the removal of unused bounds, the layer param
// and the enum. this would result in potentially signficant savings BUT generally, these will actually be stored and reused
// because the bounds are common across most objects of a type - missiles aren't getting scaled every shot -
// and the point is left rewritable.

//we've already bled clarity to avoid polymorphism, but I think that's actually useful, **because** I want the params to be
//opaque so that we can keep the bounds in jolt coordinate space at all times.

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
class FBShapeParams
{
	friend class FBLetter;
	friend class FBarrageBounder;
	friend class CoordinateUtils;
	friend class FWorldSimOwner;
	friend class UBarrageDispatch;
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

protected:
	double bound1;
	double bound2;
	double bound3; //calculate your own halfex. :|
	uint32_t layer;
	FBShape MyShape;
};
//this should evaluate in most IDEs, allowing you to see the size if you need to make changes. try not to need to.
//constexpr const static int size = sizeof(FBShapeParams);
//#pragma pack(pop) //uncomment along with the push pragma or you'll have a very funny but very bad time

