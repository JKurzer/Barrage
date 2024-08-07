#pragma once
#include "FBShapeParams.h"
//A Barrage shapelet accepts forces and transformations as though it were not managed by an evil secret machine
//and this allows us to pretty much Do The Right Thing. I've chosen to actually hide the specific kind of shape as an
//enum prop rather than a class parameter. The pack pragma makes me reluctant to use non-POD approaches.
//primitives MUST be passed by reference or pointer. doing otherwise means that you may have an out of date view
//of tombstone state, which is not particularly safe.
struct FBarragePrimitive
{
	//you cannot safely reorder these or it will change the pack width.
	FBarrageKey KeyIntoBarrage; 
	//This breaks our type dependency by using the underlying hashkey instead of the artillery type.
	//this is pretty risky, but it's basically necessary to avoid a dependency on artillery until we factor our types
	//into a type plugin, which is a wild but not unexpected thing to do.
	uint64_t KeyOutOfBarrage;

	//Tombstone state. Used to ensure that we don't nullity sliced. 
	//0 is normal.
	//1 is dead and indicates that the primitive could be released at any time.
	//anything else is a cycles-remaining.
	//a primitive is guaranteed to be tombstoned for at least 10 cycles before it is released, so this can safely be
	//used in conjunction with null checks to allow a guarantee for at least 1 cycle.
	uint32 tombstone; //4b 
	FBShapeParams::FBShape Me; //4b
};
typedef FBarragePrimitive FBShapelet;
typedef FBShapelet FBLet;

//Note the use of shared pointers. Due to tombstoning, FBlets must always be used by reference.
//this is actually why they're called FBLets, as they're rented (or let) shapes that are also thus both shapelets and shape-lets.
class FBarrageTransformer
{
	//transform forces transparently from UE world space to jolt world space
	//then apply them directly to the "primitive"
	static void ApplyForce(FVector3d Force, TSharedPtr<FBLet> Target)
	{
		
	}
	//transform the quaternion from the UE ref to the Jolt ref
	//then apply it to the "primitive"
	static void ApplyRotation(FQuat4d Rotator, TSharedPtr<FBLet>  Target)
	{
		
	}

	//as the barrage primitive contains both the in and out keys, that is sufficient to act as a full mapping
	//IFF you can supply the dispatch provider that owns the out key. this is done as a template arg
	template <typename OutKeyDispatch>
	static void PublishTransformFromJolt(TSharedPtr<FBLet>  Target)
	{
		
	}

	//tombstoned primitives are treated as null even by live references, because while the primitive is valid
	//and operations against it can be performed safely, no new operations should be allowed to start.
	//the tombstone period is effectively a grace period due to the fact that we have quite a lot of different
	//timings in play. it should be largely unnecessary, but it's also a very useful semantic for any pooled
	//data and allows us to batch disposal nicely.
	static inline bool IsNotNull(TSharedPtr<FBarragePrimitive> Target)
	{
		return Target != nullptr && Target.IsValid() && Target->tombstone == 0;
	}
	
};