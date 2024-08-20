// Copyright Epic Games, Inc. All Rights Reserved.

#include "Barrage.h"


#define LOCTEXT_NAMESPACE "FBarrage"



void FBarrageModule::StartupModule()
{
	// we have some stuff to be mindful of that makes isolating Barrage and the Barrage types a good idea, namely, packing pragmas.
	// https://youtu.be/3OzPQOKFBQU?si=ebfPc14JuPraTvx5&t=81
	// right now, I need to decide how much isolation I want.
}

void FBarrageModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FBarrageModule, Barrage)