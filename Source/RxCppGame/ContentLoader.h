#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/LevelStreaming.h"
#include "Engine/StreamableManager.h"
#include "Kismet/GameplayStatics.h"
#include "RxCpp/RxCppManager.h"
#include "ContentLoader.generated.h"


//* UGameplayStatics::LoadStreamLevel() 콜백 타겟 */
UCLASS()
class ULoadLevelStreamingCallback : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void CallbackImpl();

public:
	ULoadLevelStreamingCallback* Init(TArray<ULevelStreaming*>& levelStreamings, const Rx::subscriber<TArray<ULevelStreaming*>>& subscriber, UObject* pWorldContextObj)
	{
		_isCompleted = false;
		_pWorldContextObj = pWorldContextObj;
		_levelStreamings = levelStreamings;
		_levelStreamingCount = levelStreamings.Num();
		_subscriberHolder.Add(subscriber);

		return this;
	}
	
	FLatentActionInfo GetLatentActionInfo() 
	{
		static int32 UUIDNum = 0;

		return FLatentActionInfo(0, ++UUIDNum, TEXT("CallbackImpl"), this);
	}

	bool IsCompleted() { return _isCompleted; }

private:
	bool _isCompleted;
	int32 _levelStreamingCount;
	UObject* _pWorldContextObj;
	TArray<ULevelStreaming*> _levelStreamings;
	TArray<Rx::subscriber<TArray<ULevelStreaming*>>> _subscriberHolder;
	
friend class AContentLoader;
};


//* FStreamableManager::RequestAsyncLoad() 콜백 타겟 */
struct FStreamableDelegateCallback
{
public:
	void CallbackImpl();

public:
	FStreamableDelegateCallback(const FString& objectPath, const Rx::subscriber<UObject*>& subscriber, UObject* pWorldContextObj) : _softObjectPtr(objectPath) 
	{
		_isCompleted = false;
		_pWorldContextObj = pWorldContextObj;
		_subscriberHolder.Add(subscriber);
	}

	UObject* GetUObject() { return _softObjectPtr.Get(); }
	bool IsCompleted() { return _isCompleted; }
	
private:
	bool _isCompleted;
	UObject* _pWorldContextObj;
	TSoftObjectPtr<UObject> _softObjectPtr;
	TSharedPtr<FStreamableHandle> _streamableHandle;
	TArray<Rx::subscriber<UObject*>> _subscriberHolder;
	
friend class AContentLoader;
};


//* Content 폴더 이하에 존재하는 에셋 로더 */
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
	Rx::observable<UObject*> LoadUObject(const FString& objectPath);

	//* LoadUObject() 템플릿 버전 */
	template <typename T>
	Rx::observable<T*, Rx::dynamic_observable<T*>> LoadTObject(const FString& objectPath)
	{
		return LoadUObject(objectPath).map([](auto v) { return (T*)v; }).as_dynamic();
	}

public:
	void ClearCompletedCallbacks();

private:
	//* UPackage 에셋 내부의 ULevelStreaming Object를 추출하여 리턴
	UFUNCTION(BlueprintCallable)
	TArray<ULevelStreaming*> GetLevelStreamingFromPackage(const FString& packagePath);

	//** Creates a new instance of this streaming level with a provided unique instance name (LevelStreaming.h에서 발췌함)
	UFUNCTION(BlueprintCallable)
	ULevelStreaming* CreateInstance(ULevelStreaming* pSourceLevel, FString UniqueInstanceName);
	
	UPROPERTY(Transient)
	TArray<ULoadLevelStreamingCallback*> _loadLevelStreamingCallbacks;

	FStreamableManager _streamableManager;
	TMap<FString, FStreamableDelegateCallback> _streamableDelegateCallbacks;
};
