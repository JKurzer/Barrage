#pragma once
#include "IsolatedJoltIncludes.h"


class CoordinateUtils
{

public:
	static inline JPH::Vec3 ToJoltCoordinates(FVector3d In)
	{
		return JPH::Vec3(In.X/100.0, In.Z/100.0, In.Y/100.0); //reverse is 0,2,1
	};

	static inline JPH::Vec3 ToJoltCoordinates(double InX, double InY, double InZ)
	{
		return JPH::Vec3(InX/100.0, InZ/100.0, InY/100.0); //reverse is 0,2,1
	};

	
	//we store forces and rotations both in 4d vecs to allow better memory locality.
	static inline JPH::Quat ToBarrageForce(FVector3d In)
	{
		return JPH::Quat(In.X, In.Z, In.Y, 1); //reverse is 0,2,1
	};
	//we store forces and rotations both in 4d vecs to allow better memory locality.
	static inline JPH::Quat ToBarrageRotation(FQuat4d In)
	{
		return ToJoltRotation(In);
	};
	//we store velocity and rotations both in 4d vecs to allow better memory locality.
	// This one requires a unit conversion unlike force since force is newtons
	static inline JPH::Quat ToBarrageVelocity(FVector3d In)
	{
		return JPH::Quat(In.X/100.0, In.Z/100.0, In.Y/100.0, 1); //reverse is 0,2,1
	};
	
	static inline JPH::Float3 ToJoltCoordinates( const Chaos::TVector<float, 3> In)
	{
		return JPH::Float3(In.X/100.0, In.Z/100.0, In.Y/100.0); //reverse is 0,2,1
	};

	static inline double RadiusToJolt(double In)
	{
		return In/100.0; 
	};

	static inline double DiamToJoltHalfExtent(double In)
	{
		return In/200.0; 
	};
	
	
	static inline FVector3f FromJoltCoordinates(JPH::Vec3 In)
	{
		return FVector3f(In[0]*100.0, In[2]*100.0, In[1]*100.0); // this looks _wrong_.
	}
	static inline FVector3f FromJoltVector(JPH::Vec3 In)
	{
		return FVector3f(In[0], In[2], In[1]); // this looks _wrong_.
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
