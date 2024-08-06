#pragma once

//unlike usual, we're actually making a struct here because I'm pretty sure we'll need it for blueprint typing.
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