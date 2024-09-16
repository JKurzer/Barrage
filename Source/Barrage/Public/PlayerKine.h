#pragma once

#include "CoreMinimal.h"
#include "Kines.h"

class PlayerKine : public ActorKine
{
	//static inline FActorComponentTickFunction FALSEHOOD_MALEVOLENCE_TRICKERY = FActorComponentTickFunction();
public:
	TWeakObjectPtr<AActor> MySelf;
	
	explicit PlayerKine(const TWeakObjectPtr<AActor>& MySelf, const ActorKey& Target)
		: MySelf(MySelf)
	{
		MyKey = Target;
	}

	virtual void SetLocationAndRotation(FVector3d Loc, FQuat4d Rot) override
	{
		TObjectPtr<AActor> Pin;
		Pin = MySelf.Get();
		if(Pin)
		{
			Pin->SetActorLocationAndRotation(Loc, Rot);
		}
	}

	virtual void SetLocation(FVector3d Location) override
	{
		TObjectPtr<AActor> Pin;
		Pin = MySelf.Get();
		if(Pin)
		{
			Pin->SetActorLocation(Location, false, nullptr, ETeleportType::None);
		}
	}

	virtual void SetRotation(FQuat4d Rotation) override
	{
		TObjectPtr<AActor> Pin;
		Pin = MySelf.Get();
		if(Pin)
		{
			Pin->SetActorRotation(Rotation, ETeleportType::None);
		}
	}
};
