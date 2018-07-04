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
	void CallbackImpl()
	{
		if (--_levelStreamingCount == 0)
		{
			_isCompleted = true;

			if (_pSubscriber && _pSubscriber->is_subscribed())
			{
				_pSubscriber->on_next(_levelStreamings);
				_pSubscriber->on_completed();
			}
		}
		
		UE_LOG(LogTemp, Warning, TEXT("_levelCount set to %d."), _levelStreamingCount);
	}

public:
	ULoadLevelStreamingCallback* Init(TArray<ULevelStreaming*>& levelStreamings, Rx::subscriber<TArray<ULevelStreaming*>>* pSubscriber)
	{
		_levelStreamings = levelStreamings;
		_levelStreamingCount = levelStreamings.Num();
		_pSubscriber = pSubscriber;
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
	Rx::subscriber<TArray<ULevelStreaming*>>* _pSubscriber;
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
	Rx::observable<TArray<ULevelStreaming*>> LoadLevelStreaming(const FString& packagePath);

private:
	//* UPackage 에셋 내부의 ULevelStreaming Object를 추출하여 리턴
	UFUNCTION(BlueprintCallable)
	TArray<ULevelStreaming*> GetLevelStreamingFromPackage(const FString& packagePath);

	UPROPERTY(Transient)
	TArray<ULoadLevelStreamingCallback*> _loadLevelStreamingCallbacks;
};
