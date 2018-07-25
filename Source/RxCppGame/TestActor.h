#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestActor.generated.h"


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
	
};
