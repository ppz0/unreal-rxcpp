// Fill out your copyright notice in the Description page of Project Settings.

#include "TestActor.h"
#include "RxCpp/RxCppManager.h"


// Sets default values
ATestActor::ATestActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ATestActor::BeginPlay()
{
	Super::BeginPlay();

    FRxCppManager::Instance().Init(GetWorld());

	TWeakObjectPtr<AActor> self(this);

	observable<>::everyframe(60, observe_on_tick_group<TG_PrePhysics>())
		.take_while([self](auto _) { return self.IsValid(); })
		.subscribe([](auto v) {
			UE_LOG(LogTemp, Warning, TEXT("frame => %d"), v);
		});
		
	observable<>::everyframe(60, observe_on_tick_group<TG_PostPhysics>())
		.take_while([self](auto _) { return self.IsValid(); })
		.subscribe([](auto v) {
			UE_LOG(LogTemp, Warning, TEXT("frame (Post) => %d"), v);
		});

	observable<>::timer(std::chrono::milliseconds(4000), observe_on_tick_group<TG_PostUpdateWork>())
		.take_while([self](auto _) { return self.IsValid(); })
		.subscribe([](auto _) {
			UE_LOG(LogTemp, Warning, TEXT("Time out~"));
		});
}

void ATestActor::BeginDestroy()
{
    Super::BeginDestroy();
    
    FRxCppManager::Instance().Destroy();
}
