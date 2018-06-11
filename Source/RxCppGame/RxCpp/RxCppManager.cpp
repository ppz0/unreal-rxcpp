// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "RxCppManager.h"

void FRxCppManager::Init(UWorld* pWorld)
{
	TickFunctions.Add(TG_PrePhysics, new FRunLoopTickFunction());
	TickFunctions.Add(TG_DuringPhysics, new FRunLoopTickFunction());
	TickFunctions.Add(TG_PostPhysics, new FRunLoopTickFunction());
	TickFunctions.Add(TG_PostUpdateWork, new FRunLoopTickFunction());

	for (auto p : TickFunctions)
	{
		p.Value->TickGroup = p.Key;
		p.Value->TickInterval = 0.f;
		p.Value->bCanEverTick = true;
		p.Value->bHighPriority = true;
		p.Value->bTickEvenWhenPaused = false;
		p.Value->bStartWithTickEnabled = true;

		p.Value->RegisterTickFunction(pWorld->PersistentLevel);
	}
}

void FRxCppManager::Destroy()
{
	for (auto p : TickFunctions)
	{
		p.Value->UnRegisterTickFunction();
		delete p.Value;
	}

	get_run_loop<TG_PrePhysics>().reset();
	get_run_loop<TG_DuringPhysics>().reset();
	get_run_loop<TG_PostPhysics>().reset();
	get_run_loop<TG_PostUpdateWork>().reset();
}

void FRxCppManager::FRunLoopTickFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	if (TickGroup == TG_PrePhysics)
	{
		get_run_loop_delta_sec<TG_PrePhysics>() = DeltaTime;
		get_run_loop<TG_PrePhysics>().dispatch_many();
	}
	else if (TickGroup == TG_DuringPhysics)
	{
		get_run_loop_delta_sec<TG_DuringPhysics>() = DeltaTime;
		get_run_loop<TG_DuringPhysics>().dispatch_many();
	}
	else if (TickGroup == TG_PostPhysics)
	{
		get_run_loop_delta_sec<TG_PostPhysics>() = DeltaTime;
		get_run_loop<TG_PostPhysics>().dispatch_many();
	}
	else if (TickGroup == TG_PostUpdateWork)
	{
		get_run_loop_delta_sec<TG_PostUpdateWork>() = DeltaTime;
		get_run_loop<TG_PostUpdateWork>().dispatch_many();
	}
}
