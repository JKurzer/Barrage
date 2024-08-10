#pragma once
PRAGMA_PUSH_PLATFORM_DEFAULT_PACKING
#include "Jolt/Jolt.h"
#include "Jolt/RegisterTypes.h"
#include "Jolt/Math/Quat.h"
#include "Jolt/Math/Vec3.h"
PRAGMA_POP_PLATFORM_DEFAULT_PACKING
using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;

class CoordinateUtils
{
public:
	static inline Vec3 ToJoltCoordinates(FVector3d In)
	{
		return Vec3(In.X*100.0, In.Z*100.0, In.Y*100.0); //reverse is 0,2,1
	};
	
	static inline Float3 ToJoltCoordinates( const Chaos::TVector<float, 3> In)
	{
		return Float3(In.X*100.0, In.Z*100.0, In.Y*100.0); //reverse is 0,2,1
	};

	static inline double RadiusToJolt( double In)
	{
		return In*100.0; //reverse is 0,2,1
	};
	
	static inline FVector3d FromJoltCoordinates(Vec3 In)
{
		return FVector3d(In.X/100.0, In.Z/100.0, In.Y/100.0); // this looks _wrong_.
}
	static inline Quat ToJoltRotation(FQuat4d In)
	{
		return Quat(-In.X, -In.Z, -In.Y, In.W);
	}
	static inline FQuat4d FromJoltRotation(Quat In)
{
		return FQuat4d(-In.X, -In.Z, -In.Y, In.W);
}
};
