#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/LevelStreaming.h"
#include "Kismet/GameplayStatics.h"
#include "RxCpp/RxCppManager.h"
#include "ContentLoader.generated.h"


//* UGameplayStatics::LoadStreamLevel() 메소드 콜백 타겟 */
UCLASS()
class ULoadLevelStreamingCallback : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void CallbackImpl();

public:
	ULoadLevelStreamingCallback* Init(TArray<ULevelStreaming*>& levelStreamings, const Rx::subscriber<TArray<ULevelStreaming*>>& subscriber)
	{
		_levelStreamings = levelStreamings;
		_levelStreamingCount = levelStreamings.Num();
		_subscriberHolder.Add(subscriber);
		_isCompleted = false;

		return this;
	}
	
	FLatentActionInfo GetLatentActionInfo() 
	{
		static int32 UUIDNum = 0;

		return FLatentActionInfo(0, ++UUIDNum, TEXT("CallbackImpl"), this);
	}

	bool IsCompleted()
	{
		return _isCompleted;
	}

private:
	bool _isCompleted;
	int32 _levelStreamingCount;
	TArray<ULevelStreaming*> _levelStreamings;
	TArray<Rx::subscriber<TArray<ULevelStreaming*>>> _subscriberHolder;
};

//* */
UCLASS()
class RXCPPGAME_API AContentLoader : public AActor
{
	GENERATED_BODY()
	
public:	
	AContentLoader();

protected:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;

#pragma region 싱글턴
public:
	static AContentLoader& GetIntance(const UObject* pWorldContextObj)	
	{ 
		if (_pInstance == nullptr)
		{
			TArray<AActor*> foundActors;

			UGameplayStatics::GetAllActorsOfClass(pWorldContextObj, AContentLoader::StaticClass(), foundActors);

			if (foundActors.Num() > 0)
				_pInstance = (AContentLoader*)foundActors.Last();
		}

		return *_pInstance; 
	}
private:
	static AContentLoader* _pInstance;
#pragma endregion

public:
	Rx::observable<TArray<ULevelStreaming*>> LoadLevelStreaming(const FString& packagePath, bool bMakeVisibleAfterLoad = false);

private:
	//* UPackage 에셋 내부의 ULevelStreaming Object를 추출하여 리턴
	UFUNCTION(BlueprintCallable)
	TArray<ULevelStreaming*> GetLevelStreamingFromPackage(const FString& packagePath);

	//** Creates a new instance of this streaming level with a provided unique instance name (LevelStreaming.h에서 발췌함)
	UFUNCTION(BlueprintCallable)
	ULevelStreaming* CreateInstance(ULevelStreaming* pSourceLevel, FString UniqueInstanceName);
	
	UPROPERTY(Transient)
	TArray<ULoadLevelStreamingCallback*> _loadLevelStreamingCallbacks;
};
