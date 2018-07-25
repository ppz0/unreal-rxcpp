#include "RxCppManager.h"

DEFINE_LOG_CATEGORY(RxCpp);

void FRxCppManager::Init(const UWorld* pWorld)
{
	this->pWorld = pWorld;

	_tickFunctions.Add(TG_PrePhysics, new FRunLoopTickFunction());
	_tickFunctions.Add(TG_DuringPhysics, new FRunLoopTickFunction());
	_tickFunctions.Add(TG_PostPhysics, new FRunLoopTickFunction());
	_tickFunctions.Add(TG_PostUpdateWork, new FRunLoopTickFunction());

	for (auto p : _tickFunctions)
	{
		p.Value->TickGroup = p.Key;
		p.Value->TickInterval = 0.f;
		p.Value->bCanEverTick = true;
		p.Value->bHighPriority = true;
		p.Value->bTickEvenWhenPaused = false;
		p.Value->bStartWithTickEnabled = true;

		p.Value->RegisterTickFunction(pWorld->PersistentLevel);
	}

	UE_LOG(RxCpp, Warning, TEXT("FRxCppManager inited"));
}

void FRxCppManager::Destroy()
{
	for (auto p : _tickFunctions)
	{
		p.Value->UnRegisterTickFunction();
		delete p.Value;
	}

	_tickFunctions.Empty();

	get_run_loop<TG_PrePhysics>().reset();
	get_run_loop<TG_DuringPhysics>().reset();
	get_run_loop<TG_PostPhysics>().reset();
	get_run_loop<TG_PostUpdateWork>().reset();

	pWorld = nullptr;
	
	UE_LOG(RxCpp, Warning, TEXT("FRxCppManager destroyed"));
}

void FRxCppManager::FRunLoopTickFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef &MyCompletionGraphEvent)
{
	//! Avoid the crash report on the application exit
#if WITH_EDITOR
	if (!GIsPlayInEditorWorld)
		return;
#else
	if (!GIsRunning)
		return;
#endif

	if (TickGroup == TG_PrePhysics)
	{
		get_tick_group_delta<TG_PrePhysics>() = DeltaTime;
		get_run_loop<TG_PrePhysics>().dispatch_many();
	}
	else if (TickGroup == TG_DuringPhysics)
	{
		get_tick_group_delta<TG_DuringPhysics>() = DeltaTime;
		get_run_loop<TG_DuringPhysics>().dispatch_many();
	}
	else if (TickGroup == TG_PostPhysics)
	{
		get_tick_group_delta<TG_PostPhysics>() = DeltaTime;
		get_run_loop<TG_PostPhysics>().dispatch_many();
	}
	else if (TickGroup == TG_PostUpdateWork)
	{
		get_tick_group_delta<TG_PostUpdateWork>() = DeltaTime;
		get_run_loop<TG_PostUpdateWork>().dispatch_many();
	}
}
