#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/LevelStreaming.h"
#include "Engine/StreamableManager.h"
#include "Kismet/GameplayStatics.h"
#include "RxCpp/RxCppManager.h"
#include "RxCppLoader.generated.h"


//* UGameplayStatics::LoadStreamLevel() 콜백 타겟 */
UCLASS()
class ULoadLevelStreamingCallback : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
		void CallbackImpl();

public:
	//* 로딩용
	ULoadLevelStreamingCallback* Init(TArray<ULevelStreaming*> levelStreamings, const Rx::subscriber<TArray<ULevelStreaming*>>& subscriber, UObject* pWorldContextObj, const FString& packageName = TEXT(""))
	{
		_isCompleted = false;
		_removeAfterUnload = false;
		_packageName = packageName;
		_pWorldContextObj = pWorldContextObj;
		_levelStreamings = levelStreamings;
		_levelStreamingCount = levelStreamings.Num();
		_loadingSubscriberHolder.Add(subscriber);

		return this;
	}
	
	//* 언로딩용
	ULoadLevelStreamingCallback* Init(TArray<ULevelStreaming*> levelStreamings, const Rx::subscriber<bool>& subscriber, UObject* pWorldContextObj, bool removeAfterUnload, const FString& packageName = TEXT(""))
	{
		_isCompleted = false;
		_removeAfterUnload = removeAfterUnload;
		_packageName = packageName;
		_pWorldContextObj = pWorldContextObj;
		_levelStreamings = levelStreamings;
		_levelStreamingCount = levelStreamings.Num();
		_unloadingSubscriberHolder.Add(subscriber);

		return this;
	}
	
	FLatentActionInfo GetLatentActionInfo() 
	{
		static int32 UUIDNum = 0;

		return FLatentActionInfo(0, ++UUIDNum, TEXT("CallbackImpl"), this);
	}

	bool IsCompleted() { return _isCompleted; }

	//* 언로딩 모드인지 리턴
	bool IsUnloading() { return _unloadingSubscriberHolder.Num() > 0; }

private:
	bool _isCompleted;
	bool _removeAfterUnload;
	int32 _levelStreamingCount;
	FString _packageName;
	UObject* _pWorldContextObj;
	TArray<ULevelStreaming*> _levelStreamings;
	TArray<Rx::subscriber<bool>> _unloadingSubscriberHolder;
	TArray<Rx::subscriber<TArray<ULevelStreaming*>>> _loadingSubscriberHolder;
	
friend class ARxCppLoader;
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
	
friend class ARxCppLoader;
};


//* Content 폴더 이하에 존재하는 에셋 로더 */
UCLASS()
class RXCPPGAME_API ARxCppLoader : public AInfo
{
	GENERATED_BODY()
	
public:
	ARxCppLoader();

protected:
	virtual void BeginDestroy() override;
	
#pragma region 싱글턴
public:
	static ARxCppLoader& Instance()	
	{ 
		if (_pInstance == nullptr)
			_pInstance = (ARxCppLoader*)const_cast<UWorld*>(FRxCppManager::Instance().pWorld)->SpawnActor(ARxCppLoader::StaticClass());

		return *_pInstance; 
	}
private:
	static ARxCppLoader* _pInstance;
#pragma endregion

public:
	//* 이미 존재하는 LevelStreaming을 비동기 로딩
	Rx::observable<TArray<ULevelStreaming*>> LoadLevelStreaming(const FString& packageName, bool bMakeVisibleAfterLoad = true);

	//* 패키지에 저장된 LevelStreaming을 비동기 로딩
	Rx::observable<TArray<ULevelStreaming*>> LoadMap(const FString& packageName, bool bMakeVisibleAfterLoad = true);

	//* 로드된 LevelStreaming을 언로딩 (비동기~)
	Rx::observable<bool> UnloadLevelStreaming(TArray<ULevelStreaming*> unloadingLevels, bool removeAfterUnload = false, FString packageName = TEXT(""));

	//* 패키지에 저장된 UObject 에셋을 비동기 로딩
	Rx::observable<UObject*> LoadUObject(const FString& objectPath);

	//* LoadUObject() 템플릿 버전
	template <typename T>
	Rx::observable<T*, Rx::dynamic_observable<T*>> LoadTObject(const FString& objectPath) {
		return LoadUObject(objectPath).map([](auto v) { return (T*)v; }).as_dynamic();
	}
	
public:
	//* 런타임에 인스턴싱된 레벨은 특정한 Prefix가 붙여짐, 고로 올바른 매칭을 위해선 로컬 패키지 이름에도 Prefix를 붙여줘야함~
	FString ConvertToWorldAssetPackageName(const FString& packageName);

	//* UPackage 내부 ULevelStreaming의 WorldAssetPackageName을 리턴
	TArray<FString> GetLevelStreamingNamesFromPackage(const FString& packagePath);
	
	void ClearCompletedCallbacks();

private:
	UWorld* GetWorldFromPackage(const FString& packagePath);

	//* UPackage 내부의 ULevelStreaming Object를 추출(인스턴싱)하여 리턴
	TArray<ULevelStreaming*> GetLevelStreamingFromPackage(const FString& packagePath);

	//* Creates a new instance of this streaming level with a provided unique instance name (LevelStreaming.h에서 발췌함)
	ULevelStreaming* CreateInstance(ULevelStreaming* pSourceLevel, FString UniqueInstanceName);
	
	UPROPERTY(Transient)
		TArray<ULoadLevelStreamingCallback*> _loadLevelStreamingCallbacks;

	FStreamableManager _streamableManager;
	TMap<FString, FStreamableDelegateCallback> _streamableDelegateCallbacks;
};
