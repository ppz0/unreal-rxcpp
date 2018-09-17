#include "TestActor.h"
#include "RxCpp/RxCppManager.h"
#include "RxCpp/RxCppLoader.h"
#include "RxCpp/RxCppInput.h"


ATestActor::ATestActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATestActor::BeginPlay()
{
	Super::BeginPlay();
	
	//* Initializes RxCppManager
    FRxCppManager::Instance().Init(GetWorld());

	// TWeakObjectPtr<AActor> self(this);

	// //* Every 30 frames which registered in TG_PrePhysics tick group, call the lambda in subscribe() method.
	observable<>::everyframe(30, On_TG_PrePhysics).is_valid(this)
		.subscribe([](auto v) {
			UE_LOG(LogTemp, Warning, TEXT("frame => %d"), v);
		});
		
	//* Here, every 60 frames in TG_PostPhysics tick group.
	observable<>::everyframe(60, On_TG_PostPhysics).is_valid(this)
		.subscribe([](auto v) {
			UE_LOG(LogTemp, Warning, TEXT("frame (Post) => %d"), v);
		});

	//* After 5 seconds, call the lambda in subscribe() method. (Actually executed in TG_PostUpdateWork tick group)
	observable<>::timer(5.f, On_TG_PostUpdateWork).is_valid(this)
		.subscribe([](auto _) {
			UE_LOG(LogTemp, Warning, TEXT("Time out~"));
		}, []() {
			UE_LOG(LogTemp, Warning, TEXT("Completed"));
		});

	static int val = 0;

	//* Every 3 seconds, call the  lambda in subscribe() method.
	observable<>::interval(3.f, On_TG_PrePhysics).is_valid(this)
		.subscribe([](auto _) {
			++val;
		});

	//* Watch 'val' and when it's value changed, call the  lambda in subscribe() method.
	observable<>::everyframe(1, On_TG_PrePhysics).is_valid(this)
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

	ARxCppInput::Instance().KeyPressed(EKeys::A)
		.subscribe([this](auto _) {
			UE_LOG(LogTemp, Warning, TEXT("A is pressed~"));

			testFSM.SetState(ETestStates::Busy);
		});
		
	ARxCppInput::Instance().KeyReleased(EKeys::B)
		.subscribe([this](auto _) {
			UE_LOG(LogTemp, Warning, TEXT("B is releaseed~"));
			
			testFSM.SetState(ETestStates::Idle);
		});
		
	ARxCppInput::Instance().KeyRepeated(EKeys::C)
		.subscribe([](auto _) {
			UE_LOG(LogTemp, Warning, TEXT("C is repeated~"));
		});

	ARxCppInput::Instance().Axis(TEXT("NewAxisMapping_0"))
		.filter([](auto v) { return !FMath::IsNearlyZero(v); })
		.subscribe([](auto v) {
			UE_LOG(LogTemp, Warning, TEXT("NewAxisMapping_0: %f"), v);
		});
		
	ARxCppInput::Instance().Axis(TEXT("NewAxisMapping_1"))
		.filter([](auto v) { return !FMath::IsNearlyZero(v); })
		.subscribe([](auto v) {
			UE_LOG(LogTemp, Warning, TEXT("NewAxisMapping_1: %f"), v);
		});

	testFSM.ListenOnEnter(ETestStates::Busy)
		.subscribe([this](auto prev) {
			if (prev == ETestStates::Idle) {
				UE_LOG(LogTemp, Warning, TEXT("Enter ETestStates::Busy"));
			}
		});

	testFSM.ListenOnExit(ETestStates::Busy)
		.subscribe([this](auto next) {
			if (next == ETestStates::Idle) {
				UE_LOG(LogTemp, Warning, TEXT("Leave ETestStates::Busy"));
			}
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
