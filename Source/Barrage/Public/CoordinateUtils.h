#pragma once
#include "Jolt/Math/Quat.h"
#include "Jolt/Math/Vec3.h"

class CoordinateUtils
{
public:
	static inline JPH::Vec3 ToJoltCoordinates(FVector3d In)
	{
		return Vec3();
	};
	static inline FVector3d FromJoltCoordinates(JPH::Vec3 In)
{
	return FVector3d();
}
	static inline JPH::Quat ToJoltRotation(FQuat4d In)
	{
		return JPH::Quat();
	}
	static inline FQuat4d FromJoltRotation(JPH::Quat In)
{
	return FQuat4d();
}
};
