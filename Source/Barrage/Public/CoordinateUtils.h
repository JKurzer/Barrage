#pragma once
PRAGMA_PUSH_PLATFORM_DEFAULT_PACKING

#include "mimalloc.h"
#include "Jolt/Jolt.h"
#include "Jolt/Math/Quat.h"
#include "Jolt/Math/Vec3.h"

PRAGMA_POP_PLATFORM_DEFAULT_PACKING


class CoordinateUtils
{

public:
	static inline JPH::Vec3 ToJoltCoordinates(FVector3d In)
	{
		return JPH::Vec3(In.X*100.0, In.Z*100.0, In.Y*100.0); //reverse is 0,2,1
	};

	//we store forces and rotations both in 4d vecs to allow better memory locality.
	static inline JPH::Quat ToBarrageForce(FVector3d In)
	{
		return JPH::Quat(In.X*100.0, In.Z*100.0, In.Y*100.0, 0); //reverse is 0,2,1
	};
	//we store forces and rotations both in 4d vecs to allow better memory locality.
	static inline JPH::Quat ToBarrageRotation(FQuat4d In)
	{
		return ToJoltRotation(In);
	};
	
	static inline JPH::Float3 ToJoltCoordinates( const Chaos::TVector<float, 3> In)
	{
		return JPH::Float3(In.X*100.0, In.Z*100.0, In.Y*100.0); //reverse is 0,2,1
	};

	static inline double RadiusToJolt(double In)
	{
		return In*100.0; 
	};

	static inline double DiamToJoltHalfExtent(double In)
	{
		return In*50.0; 
	};
	
	
	static inline FVector3f FromJoltCoordinates(JPH::Vec3 In)
{
		return FVector3f(In[0]/100.0, In[2]/100.0, In[1]/100.0); // this looks _wrong_.
}
	static inline JPH::Quat ToJoltRotation(FQuat4d In)
	{
		return JPH::Quat(-In.X, -In.Z, -In.Y, In.W);
	}
	static inline FQuat4f FromJoltRotation(JPH::Quat In)
{
		return FQuat4f(-In.GetX(), -In.GetZ(), -In.GetY(), In.GetW());
}
};
