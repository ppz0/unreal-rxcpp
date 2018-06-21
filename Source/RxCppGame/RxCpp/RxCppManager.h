// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "UndefineMacros_UE_4.18.h"
#define _CPPRTTI 0
#define RXCPP_USE_RTTI 0
#include "Rx/v2/src/rxcpp/rx.hpp"
namespace Rx {
using namespace rxcpp;
using namespace rxcpp::sources;
using namespace rxcpp::operators;
using namespace rxcpp::util;
}
using namespace Rx;
#include "RedefineMacros_UE_4.18.h"


namespace rxcpp
{
	template <ETickingGroup TickGroup>
	inline schedulers::run_loop& get_run_loop()
	{
		static schedulers::run_loop rl;
		return rl;
	}

	template <ETickingGroup TickGroup>
	inline float& get_tick_group_delta()
	{
		static float deltaTime = 0.f;
		return deltaTime;
	}

	template <ETickingGroup TickGroup>
	inline observe_on_one_worker& observe_on_tick_group()
	{
		static observe_on_one_worker r(rxcpp::schedulers::make_run_loop(get_run_loop<TickGroup>()));
		return r;
	}

}

#define On_TG_PrePhysics observe_on_tick_group<TG_PrePhysics>()
#define On_TG_DuringPhysics observe_on_tick_group<TG_DuringPhysics>()
#define On_TG_PostPhysics observe_on_tick_group<TG_PostPhysics>()
#define On_TG_PostUpdateWork observe_on_tick_group<TG_PostUpdateWork>()


class FRxCppManager
{
public:
	inline static FRxCppManager& Instance()
	{
		static FRxCppManager m;
		return m;
	}

public:
	void Init(UWorld* pWorld);
	void Destroy();

public:
	struct FRunLoopTickFunction : public FTickFunction
	{
		virtual void ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
	};

private:
	TMap<ETickingGroup, FRunLoopTickFunction*> TickFunctions;

};
