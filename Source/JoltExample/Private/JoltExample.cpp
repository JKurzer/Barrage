// Copyright Epic Games, Inc. All Rights Reserved.

#include "JoltExample.h"
#include "JoltPhysicsTest.h"

#include "Containers/Ticker.h"

#define LOCTEXT_NAMESPACE "FJoltExampleModule"



void FJoltExampleModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FTickerDelegate cb;

	cb.BindLambda([this](float DeltaTime){
		JoltPhysicsTest test;
		printf("");
		return true;
	});
	FTSTicker::GetCoreTicker().AddTicker(cb,2.0f);
}

void FJoltExampleModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FJoltExampleModule, JoltExample)