#pragma once
#include "CoreMinimal.h"
#include "BarrageDispatch.h"
#include "BarrageContactEvent.h"

namespace JOLT
{
	class BarrageContactListener : public ContactListener
	{
	private:
		TWeakObjectPtr<UBarrageDispatch> BarrageDispatch;
		
	public:
		BarrageContactListener(UBarrageDispatch* BarrageDispatchIn) : BarrageDispatch(BarrageDispatchIn)
		{
			
		}
		
		virtual ValidateResult OnContactValidate(const Body& inBody1, const Body& inBody2, RVec3Arg inBaseOffset,
												 const CollideShapeResult& inCollisionResult) override;

		virtual void OnContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold,
									ContactSettings& ioSettings) override;

		virtual void OnContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold,
										ContactSettings& ioSettings) override;

		virtual void OnContactRemoved(const SubShapeIDPair& inSubShapePair) override;
	};
}