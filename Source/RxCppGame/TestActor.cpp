#include "TestActor.h"
#include "RxCpp/RxCppManager.h"
#include "RxCpp/RxCppLoader.h"


ATestActor::ATestActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATestActor::BeginPlay()
{
	Super::BeginPlay();
	
	//* Initializes RxCppManager
    FRxCppManager::Instance().Init(GetWorld());

	TWeakObjectPtr<AActor> self(this);

	// //* Every 30 frames which registered in TG_PrePhysics tick group, call the lambda in subscribe() method.
	observable<>::everyframe(30, On_TG_PrePhysics)
		.take_while([self](auto _) { return self.IsValid(); })
		.subscribe([](auto v) {
			UE_LOG(LogTemp, Warning, TEXT("frame => %d"), v);
		});
		
	//* Here, every 60 frames in TG_PostPhysics tick group.
	observable<>::everyframe(60, On_TG_PostPhysics)
		.take_while([self](auto _) { return self.IsValid(); })
		.subscribe([](auto v) {
			UE_LOG(LogTemp, Warning, TEXT("frame (Post) => %d"), v);
		});

	//* After 5 seconds, call the lambda in subscribe() method. (Actually executed in TG_PostUpdateWork tick group)
	observable<>::timer(5.f, On_TG_PostUpdateWork)
		.take_while([self](auto _) { return self.IsValid(); })
		.subscribe([](auto _) {
			UE_LOG(LogTemp, Warning, TEXT("Time out~"));
		}, []() {
			UE_LOG(LogTemp, Warning, TEXT("Completed"));
		});

	static int val = 0;

	//* Every 3 seconds, call the  lambda in subscribe() method.
	observable<>::interval(3.f, On_TG_PrePhysics)
		.take_while([self](auto _) { return self.IsValid(); })
		.subscribe([](auto _) {
			++val;
		});

	//* Watch 'val' and when it's value changed, call the  lambda in subscribe() method.
	observable<>::everyframe(1, On_TG_PrePhysics)
		.take_while([self](auto _) { return self.IsValid(); })
		.map([](auto _) { return val; })
		.distinct_until_changed()
		.subscribe([](auto v) {
			UE_LOG(LogTemp, Warning, TEXT("val is changed to %d."), v);
		});

	ARxCppLoader::Instance().LoadTObject<UTexture2D>("/Game/StarterContent/Textures/T_Brick_Cut_Stone_N")
		.subscribe([](auto v) {
			if (v != nullptr)
				UE_LOG(LogTemp, Warning, TEXT("UTexture2D '%s' is loaded"), *(v->GetFullName()));
		});

	ARxCppLoader::Instance().LoadMap(TEXT("/Game/StarterContent/Maps/TestMap"))
		.subscribe([](auto vv) {
			for (auto &l : vv)
				UE_LOG(LogTemp, Warning, TEXT("ULevelStreaming '%s' is loaded"), *FPackageName::GetShortName(l->GetWorldAssetPackageFName()));
		});
}

void ATestActor::BeginDestroy()
{
    Super::BeginDestroy();
	
	//* Destory FRxCppManager
    FRxCppManager::Instance().Destroy();
}

void ATestActor::TestLatentAction(UObject* WCO, struct FLatentActionInfo LatentInfo, FTestLatentAction_OnNext OnNext) 
{
	observable<>::timer(5.f, On_TG_PrePhysics)
		.map([](auto _) { return FString("Test"); })
		.SUBSCRIBE_ON_LATENT_ACTION(LatentInfo, OnNext);
}
