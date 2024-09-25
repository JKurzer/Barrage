#pragma once

// Repurposed from Jolt `VehicleCollisionTester.cpp`
class SphereCastCollector : public JPH::CastShapeCollector
{
public:
	SphereCastCollector(JPH::PhysicsSystem &inPhysicsSystem, const JPH::RShapeCast &inShapeCast) :
		mPhysicsSystem(inPhysicsSystem),
		mShapeCast(inShapeCast)
	{
	}

	virtual void AddHit(const JPH::ShapeCastResult &inResult) override
	{
		// Test if this collision is closer/deeper than the previous one
		float early_out = inResult.GetEarlyOutFraction();
		if (early_out < GetEarlyOutFraction())
		{
			// Lock the body
			JPH::BodyLockRead lock(mPhysicsSystem.GetBodyLockInterfaceNoLock(), inResult.mBodyID2);
			JPH_ASSERT(lock.Succeeded()); // When this runs all bodies are locked so this should not fail
			const JPH::Body* body = &lock.GetBody();

			JPH::Vec3 normal = -inResult.mPenetrationAxis.Normalized();
					
			// Update early out fraction to this hit
			UpdateEarlyOutFraction(early_out);

			// Get the contact properties
			mBody = body;
			mSubShapeID2 = inResult.mSubShapeID2;
			mContactPosition = mShapeCast.mCenterOfMassStart.GetTranslation() + inResult.mContactPointOn2;
			mContactNormal = normal;
			mFraction = inResult.mFraction;
		}
	}

	// Configuration
	JPH::PhysicsSystem &	mPhysicsSystem;
	const JPH::RShapeCast &	mShapeCast;

	// Resulting closest collision
	const JPH::Body*		mBody = nullptr;
	JPH::SubShapeID			mSubShapeID2;
	JPH::RVec3				mContactPosition;
	JPH::Vec3				mContactNormal;
	float					mFraction;
};