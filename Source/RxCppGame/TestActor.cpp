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

	observable<>::everyframe(30, On_TG_PrePhysics)
		.take_while([self](auto _) { return self.IsValid(); })
		.subscribe([](auto v) {
			UE_LOG(LogTemp, Warning, TEXT("frame => %d"), v);
		});
		
	observable<>::everyframe(60, On_TG_PostPhysics)
		.take_while([self](auto _) { return self.IsValid(); })
		.subscribe([](auto v) {
			UE_LOG(LogTemp, Warning, TEXT("frame (Post) => %d"), v);
		});

	observable<>::timer(5.f, On_TG_PostUpdateWork)
		.take_while([self](auto _) { return self.IsValid(); })
		.subscribe([](auto _) {
			UE_LOG(LogTemp, Warning, TEXT("Time out~"));
		}, []() {
			UE_LOG(LogTemp, Warning, TEXT("Completed"));
		});

	static int val = 0;

	observable<>::everyframe(1, On_TG_PrePhysics)
		.take_while([self](auto _) { return self.IsValid(); })
		.map([](auto _) { return val; })
		.distinct_until_changed()
		.subscribe([](auto v) {
			UE_LOG(LogTemp, Warning, TEXT("val is changed to %d."), v);
		});
		
	observable<>::interval(3.f, On_TG_PrePhysics)
		.take_while([self](auto _) { return self.IsValid(); })
		.subscribe([](auto _) {
			++val;
		});
}

void ATestActor::BeginDestroy()
{
    Super::BeginDestroy();
    
    FRxCppManager::Instance().Destroy();
}
