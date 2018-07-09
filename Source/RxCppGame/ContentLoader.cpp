#include "ContentLoader.h"


AContentLoader* AContentLoader::_pInstance = nullptr;

void ULoadLevelStreamingCallback::CallbackImpl()
{
	if (--_levelStreamingCount == 0)
	{
		_isCompleted = true;

		_subscriberHolder.Last().on_next(_levelStreamings);
		_subscriberHolder.Last().on_completed();
		
		AContentLoader::GetIntance(_pWorldContextObj).ClearCompletedCallbacks();
	}

	UE_LOG(LogTemp, Warning, TEXT("_levelCount set to %d."), _levelStreamingCount);
}

void FStreamableDelegateCallback::CallbackImpl() 
{
	auto* pObject = GetUObject();
	
	if (pObject != nullptr)
	{
		_subscriberHolder.Last().on_next(pObject);
		_subscriberHolder.Last().on_completed();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("'%s' in invalid."), *_softObjectPtr.ToString());

#if WITH_EDITOR
		//* 크래시 방지를 위해서 EDITOR 모드이면 on_next() 호출
		_subscriberHolder.Last().on_next(nullptr);
		_subscriberHolder.Last().on_completed();
#else
		_subscriberHolder.Last().on_error(std::make_exception_ptr(std::exception("invalid uobject")));
#endif
	}
	
	_isCompleted = true;
}

AContentLoader::AContentLoader()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AContentLoader::BeginPlay()
{
	Super::BeginPlay();
}

void AContentLoader::BeginDestroy()
{
	Super::BeginDestroy();

	//! 싱글턴 값 클리어
	AContentLoader::_pInstance = nullptr;
}

Rx::observable<TArray<ULevelStreaming*>> AContentLoader::LoadLevelStreaming(const FString &packagePath, bool bMakeVisibleAfterLoad)
{
	//! 이전 LoadLevelStreaming은 삭제 (동시에 2개 이상의 요청을 할 수 없음을 의미함)
	_loadLevelStreamingCallbacks.Empty();

	return Rx::observable<>::create<TArray<ULevelStreaming*>>([&, this](Rx::subscriber<TArray<ULevelStreaming*>> s) {
		auto streamingLevels = GetLevelStreamingFromPackage(packagePath);

		if (streamingLevels.Num() < 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("'%s' has 0 LevelStreaming (or package is invalid)."), *packagePath);
			
#if WITH_EDITOR
			//* 크래시 방지를 위해서 EDITOR 모드이면 on_next() 호출
			if (s.is_subscribed())
			{
				s.on_next(streamingLevels);
				s.on_completed();
			}
#else
			if (s.is_subscribed())
				s.on_error(std::make_exception_ptr(std::exception("invalid map")));
#endif

			return;
		}

		auto* pCallback = NewObject<ULoadLevelStreamingCallback>()->Init(streamingLevels, s, GetWorld());

		_loadLevelStreamingCallbacks.Add(pCallback);
		
		for (auto &l : streamingLevels)
		{
			GetWorld()->StreamingLevels.Add(l);
			
			UE_LOG(LogTemp, Warning, TEXT("ULevelStreaming '%s' is added"), *FPackageName::GetShortName(l->GetWorldAssetPackageFName()));

			UGameplayStatics::LoadStreamLevel(GetWorld(), 
				FName(*FPackageName::GetShortName(l->GetWorldAssetPackageFName())), bMakeVisibleAfterLoad, false, 
				pCallback->GetLatentActionInfo()
				);
		}
	}).as_dynamic();
}

TArray<ULevelStreaming*> AContentLoader::GetLevelStreamingFromPackage(const FString &packagePath)
{
	TArray<ULevelStreaming*> ret;

	auto *pPkg = FindPackage(nullptr, *packagePath);

	if (pPkg == nullptr)
	{
		pPkg = LoadPackage(nullptr, *packagePath, LOAD_None);

		if (pPkg == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("'%s' is not valid path"), *packagePath);
			return ret;
		}
	}

	auto *pWorld = UWorld::FindWorldInPackage(pPkg);

	if (pWorld == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("'%s' has no UWorld object"), *packagePath);
		return ret;
	}

	for (auto &l : pWorld->StreamingLevels)
		ret.Add(CreateInstance(l, l->GetWorldAssetPackageName()));

	return ret;
}

ULevelStreaming* AContentLoader::CreateInstance(ULevelStreaming* pSourceLevel, FString InstanceUniqueName)
{
	ULevelStreaming* StreamingLevelInstance = nullptr;
	
	UWorld* InWorld = GetWorld();
	if (InWorld)
	{
		// Create instance long package name 
		FString InstanceShortPackageName = InWorld->StreamingLevelsPrefix + FPackageName::GetShortName(InstanceUniqueName);
		FString InstancePackagePath = FPackageName::GetLongPackagePath(pSourceLevel->GetWorldAssetPackageName()) + TEXT("/");
		FName	InstanceUniquePackageName = FName(*(InstancePackagePath + InstanceShortPackageName));

		// check if instance name is unique among existing streaming level objects
		const bool bUniqueName = (InWorld->StreamingLevels.IndexOfByPredicate(ULevelStreaming::FPackageNameMatcher(InstanceUniquePackageName)) == INDEX_NONE);
				
		if (bUniqueName)
		{
			StreamingLevelInstance = NewObject<ULevelStreaming>(InWorld, pSourceLevel->GetClass(), NAME_None, RF_Transient, NULL);
			// new level streaming instance will load the same map package as this object
			StreamingLevelInstance->PackageNameToLoad = (pSourceLevel->PackageNameToLoad == NAME_None ? pSourceLevel->GetWorldAssetPackageFName() : pSourceLevel->PackageNameToLoad);
			// under a provided unique name
			StreamingLevelInstance->SetWorldAssetByPackageName(InstanceUniquePackageName);
			StreamingLevelInstance->bShouldBeLoaded = false;
			StreamingLevelInstance->bShouldBeVisible = false;
			StreamingLevelInstance->LevelTransform = pSourceLevel->LevelTransform;

			// // add a new instance to streaming level list
			// InWorld->StreamingLevels.Add(StreamingLevelInstance);
		}
		else
		{
			UE_LOG(LogStreaming, Warning, TEXT("Provided streaming level instance name is not unique: %s"), *InstanceUniquePackageName.ToString());
		}
	}
	
	return StreamingLevelInstance;
}

Rx::observable<UObject*> AContentLoader::LoadUObject(const FString& objectPath)
{
	return Rx::observable<>::create<UObject*>([&, this](Rx::subscriber<UObject*> s) {
		_streamableDelegateCallbacks.Add(objectPath, FStreamableDelegateCallback(objectPath, s, GetWorld()));

		_streamableDelegateCallbacks[objectPath]._streamableHandle = _streamableManager.RequestAsyncLoad(
			FSoftObjectPath(objectPath), 
			FStreamableDelegate::CreateRaw(&(_streamableDelegateCallbacks[objectPath]), &FStreamableDelegateCallback::CallbackImpl)
			);

		if (!_streamableDelegateCallbacks[objectPath]._streamableHandle.IsValid())
			_streamableDelegateCallbacks[objectPath].CallbackImpl();
	});
}

void AContentLoader::ClearCompletedCallbacks()
{
	_loadLevelStreamingCallbacks.RemoveAll([](auto v) { return v->_isCompleted; });

	TArray<FString> keys;

	_streamableDelegateCallbacks.GenerateKeyArray(keys);

	for (auto& k : keys)
	{
		if (_streamableDelegateCallbacks[k]._isCompleted)
			_streamableDelegateCallbacks.Remove(k);
	}
}
