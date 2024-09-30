#pragma once
#include "EPhysicsLayer.h"
#include "IsolatedJoltIncludes.h"

using namespace JPH;
using namespace JPH::literals;

struct BarrageContactEntity
{
	BarrageContactEntity(const FSkeletonKey ContactKeyIn)
	{
		ContactKey = ContactKeyIn;
		bIsProjectile = false;
	}
	BarrageContactEntity(const FSkeletonKey ContactKeyIn, const Body& BodyIn)
	{
		ContactKey = ContactKeyIn;
		bIsProjectile = BodyIn.GetObjectLayer() == Layers::PROJECTILE;
	}
	FSkeletonKey ContactKey;
	bool bIsProjectile:1;
};

enum class EBarrageContactEventType
{
	ADDED,
	PERSISTED,
	REMOVED
};

struct BarrageContactEvent
{
	EBarrageContactEventType ContactEventType;
	BarrageContactEntity ContactEntity1;
	BarrageContactEntity ContactEntity2;

	bool IsEitherEntityAProjectile() const
	{
		return ContactEntity1.bIsProjectile || ContactEntity2.bIsProjectile;
	}
};