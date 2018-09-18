#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FSM.h"
#include "TestActor.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(RxCppGame, Log, All);

UENUM(BlueprintType)
enum class ETestStates : uint8
{
	Idle,
	Busy,
	GameOver
};


UCLASS()
class RXCPPGAME_API ATestActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ATestActor();

protected:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
	
public:
	DECLARE_DYNAMIC_DELEGATE_OneParam(FTestLatentAction_OnNext, FString, value);

	/** Send request using latent action */ 
	UFUNCTION(BlueprintCallable, meta = (Latent, LatentInfo = "LatentInfo", HidePin = "WCO", DefaultToSelf = "WCO")) 
		virtual void TestLatentAction(UObject* WCO, struct FLatentActionInfo LatentInfo, FTestLatentAction_OnNext OnNext);

public:
	FSM<ETestStates> testFSM;
	
};
