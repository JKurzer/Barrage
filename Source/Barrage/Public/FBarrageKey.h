#pragma once
#include "skeletonize.h"
//unlike usual, we're actually making a struct here because I'm pretty sure we'll need it for blueprint typing.
//you might notice something kind of odd about all the barrage structs. They're all multiples of both 8 AND 4. this is to minimize the FUN
//that packing can cause, because those pack identically for 4 and 8.

struct FBarrageKey
{	
	uint64 KeyIntoBarrage;
	friend uint64 GetTypeHash(const FBarrageKey& Other)
	{
		//it looks like get type hash can be a 64bit return? 
		return  FORGE_SKELETON_KEY(GetTypeHash(Other.KeyIntoBarrage), SKELLY::SFIX_BAR_PRIM);
	}
};
static bool operator==(FBarrageKey const& lhs, FBarrageKey const& rhs) {
	return  (lhs.KeyIntoBarrage == rhs.KeyIntoBarrage);
}