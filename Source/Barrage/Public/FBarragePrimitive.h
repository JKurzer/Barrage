#pragma once
#include "FBShapeParams.h"
//A Barrage shapelet accepts forces and transformations as though it were not managed by an evil secret machine
//and this allows us to pretty much Do The Right Thing. I've chosen to actually hide the specific kind of shape as an
//enum prop rather than a class parameter. This will probably have to change, but for now, it's very pleasant.
//by which I mean a little terrifying, because actually, this is just a shim.
struct FBarragePrimitive
{
	//you cannot safely reorder these or it will change the pack width.
	FBarrageKey KeyIntoBarrage; 
	//This breaks our type dependency by using the underlying hashkey instead of the artillery type.
	//this is pretty risky, but it's basically necessary to avoid a dependency on artillery until we factor our types
	//into a type plugin, which is a wild but not unexpected thing to do.
	uint64_t KeyOutOfBarrage;

	//this can store the mID underlying the Jolt Body Id, allowing us to break the type dependency and preserve
	//the TLU boundary.
	uint32 inID; //4b 
	FBShapeParams::FBShape Me; //4b
};
typedef FBarragePrimitive FBShapelet;
typedef FBShapelet FBlet;