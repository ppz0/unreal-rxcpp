#pragma once

//! Depress complier error warnings
#pragma warning(disable:4503)
#pragma warning(disable:4530)
#pragma warning(disable:4577)

//! Depress complier error on packaging
#pragma warning(disable:4530)
#pragma warning(disable:4577)

#include "CoreMinimal.h"

#include "UndefineMacros_UE_4.18.h"
#define _CPPRTTI 0
#define RXCPP_USE_RTTI 0
#include "Rx/v2/src/rxcpp/rx.hpp"
namespace Rx
{
using namespace rxcpp;
using namespace rxcpp::sources;
using namespace rxcpp::operators;
using namespace rxcpp::util;
} // namespace Rx
using namespace Rx;
#include "RedefineMacros_UE_4.18.h"

DECLARE_LOG_CATEGORY_EXTERN(RxCpp, Log, All);

namespace rxcpp
{
template <ETickingGroup TickGroup>
inline schedulers::run_loop &get_run_loop()
{
	if (TickGroup != TG_PrePhysics && TickGroup != TG_DuringPhysics && TickGroup != TG_PostPhysics && TickGroup != TG_PostUpdateWork)
		UE_LOG(RxCpp, Warning, TEXT("The given TickGroup is not running. Choose one of { TG_PrePhysics, TG_DuringPhysics, TG_PostPhysics, TG_PostUpdateWork }."));

	static schedulers::run_loop rl;
	return rl;
}

template <ETickingGroup TickGroup>
inline float &get_tick_group_delta()
{
	static float deltaTime = 0.f;
	return deltaTime;
}

template <ETickingGroup TickGroup>
inline observe_on_one_worker &observe_on_tick_group()
{
	static observe_on_one_worker r(rxcpp::schedulers::make_run_loop(get_run_loop<TickGroup>()));
	return r;
}
} // namespace rxcpp

#define On_TG_PrePhysics observe_on_tick_group<TG_PrePhysics>()
#define On_TG_DuringPhysics observe_on_tick_group<TG_DuringPhysics>()
#define On_TG_PostPhysics observe_on_tick_group<TG_PostPhysics>()
#define On_TG_PostUpdateWork observe_on_tick_group<TG_PostUpdateWork>()

//* Helper macro for observable<>::everyframe
#define RX_EVERYFRAME(x) observable<>::everyframe(x, On_TG_PrePhysics)
#define RX_EVERYFRAME_WITH(x, owner) observable<>::everyframe(x, On_TG_PrePhysics).is_valid(owner)

//* NextFrame macro ('Zero' duration observable<>::timer)
#define RX_NEXTFRAME() observable<>::timer(TNumericLimits<float>::Min, On_TG_PrePhysics)
#define RX_NEXTFRAME_WITH(owner) observable<>::timer(TNumericLimits<float>::Min, On_TG_PrePhysics).is_valid(owner)

//* Helper macro for observable<>::timer
#define RX_TIMER(x)	observable<>::timer(x, On_TG_PrePhysics)
#define RX_TIMER_WITH(x, owner) observable<>::timer(x, On_TG_PrePhysics).is_valid(owner)

//* Helper macro for observable<>::interval
#define RX_INTERVAL(x)	observable<>::interval(x, On_TG_PrePhysics)
#define RX_INTERVAL_WITH(x, owner) observable<>::interval(x, On_TG_PrePhysics).is_valid(owner)

//* Subscribe macro which Bypasses subscribe callback to LatentAction
#define SUBSCRIBE_ON_LATENT_ACTION(latent, next)\
	subscribe([next](auto v) {\
		next.ExecuteIfBound(v);\
	}, [latent]() {\
		if (IsValid(latent.CallbackTarget) && latent.ExecutionFunction != NAME_None && latent.Linkage != INDEX_NONE) {\
			if (auto f = latent.CallbackTarget->FindFunction(latent.ExecutionFunction))\
				latent.CallbackTarget->ProcessEvent(f, (void*)&(latent.Linkage));\
		}\
	})

class FRxCppManager
{
#pragma region Singleton
public:
	inline static FRxCppManager &Instance()
	{
		static FRxCppManager m;
		return m;
	}
#pragma endregion

	const UWorld* pWorld;
	
public:
	FRxCppManager() : pWorld(nullptr) {}

public:
	void Init(const UWorld *pWorld);
	void Destroy();

public:
	struct FRunLoopTickFunction : public FTickFunction
	{
		virtual void ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef &MyCompletionGraphEvent) override;
	};

private:
	TMap<ETickingGroup, FRunLoopTickFunction*> _tickFunctions;
};
