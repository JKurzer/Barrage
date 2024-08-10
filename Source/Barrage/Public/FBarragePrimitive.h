#pragma once
#include "FBarrageKey.h"
#include "FBShapeParams.h"


class UBarrageDispatch;
//don't use this, it's just here for speedy access from the barrage primitive destructor until we refactor.
inline UBarrageDispatch* GlobalBarrage = nullptr;
//A Barrage shapelet accepts forces and transformations as though it were not managed by an evil secret machine
//and this allows us to pretty much Do The Right Thing. I've chosen to actually hide the specific kind of shape as an
//enum prop rather than a class parameter. The pack pragma makes me reluctant to use non-POD approaches.
//primitives MUST be passed by reference or pointer. doing otherwise means that you may have an out of date view
//of tombstone state, which is not particularly safe.
class FBarragePrimitive
{
public:
	enum FBShape
	{
		Capsule,
		Box,
		Sphere,
		Static
	};
	//you cannot safely reorder these or it will change the pack width.
	FBarrageKey KeyIntoBarrage; 
	//This breaks our type dependency by using the underlying hashkey instead of the artillery type.
	//this is pretty risky, but it's basically necessary to avoid a dependency on artillery until we factor our types
	//into a type plugin, which is a wild but not unexpected thing to do.
	uint64_t KeyOutOfBarrage;

	//Tombstone state. Used to ensure that we don't nullity sliced. 
	//0 is normal.
	//1 or more is dead and indicates that the primitive could be released at any time. These should be considered
	// opaque values as implementation is not final. a primitive is guaranteed to be tombstoned for at least
	// BarrageDispatch::TombstoneInitialMinimum cycles before it is released, so this can safely be
	// used in conjunction with null checks to ensure that short tasks can finish safely without
	// worrying about the exact structure of our lifecycles. it is also used for pool and rollback handling,
	// and the implementation will change as those come online in artillery.
	uint32 tombstone; //4b 
	FBShape Me; //4b

	FBarragePrimitive(FBarrageKey Into, uint64 OutOf)
	{
		KeyIntoBarrage = Into;
		KeyOutOfBarrage = OutOf;
		tombstone = 0;
	}
	~FBarragePrimitive();
};


typedef FBarragePrimitive FBShapelet;
typedef TSharedPtr<FBShapelet> FBLet;
