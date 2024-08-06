#pragma once

//unlike usual, we're actually making a struct here because I'm pretty sure we'll need it for blueprint typing.
//you might notice something kind of odd about all these structs. They're all multiples of both 8 AND 4. this is to minimize the FUN
//that packing can cause, because those pack identically for 4 and 8.
struct FBarrageKey
{
	uint64_t KeyIntoBarrage;
	friend uint32 GetTypeHash(const FBarrageKey& Other)
	{
		// it's probably fine!
		return GetTypeHash(Other.KeyIntoBarrage);
	}
};
static bool operator==(FBarrageKey const& lhs, FBarrageKey const& rhs) {
	return (lhs.KeyIntoBarrage == rhs.KeyIntoBarrage);
}