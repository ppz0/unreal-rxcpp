#include "RxCppLoader.h"

void ULoadLevelStreamingCallback::CallbackImpl()
{
	--_levelStreamingCount;

	if (!IsUnloading()) {
		UE_LOG(RxCpp, Log, TEXT("ULoadLevelStreamingCallback => Loading %s... _levelCount set to %d."), *_packageName, _levelStreamingCount);
	}
	else {
		UE_LOG(RxCpp, Log, TEXT("ULoadLevelStreamingCallback => Unloading %s... _levelCount set to %d."), *_packageName, _levelStreamingCount);
	}

	if (_levelStreamingCount == 0)
	{
		_isCompleted = true;

		if (!IsUnloading())
		{
			_loadingSubscriberHolder.Last().on_next(_levelStreamings);
			_loadingSubscriberHolder.Last().on_completed();
		}
		else
		{
			_unloadingSubscriberHolder.Last().on_next(true);
			_unloadingSubscriberHolder.Last().on_completed();
		
			if (_removeAfterUnload)
			{
				// 언로드된 레벨은 제거
				for (auto l : _levelStreamings)
				{
					if (_pWorldContextObj->GetWorld()->StreamingLevels.Contains(l))
					{
						_pWorldContextObj->GetWorld()->StreamingLevels.Remove(l);
						UE_LOG(RxCpp, Log, TEXT("ULoadLevelStreamingCallback => '%s' is removed from GetWorld()->StreamingLevels."), *(l->GetWorldAssetPackageName()));
					}
				}
			}
		}
		
		ARxCppLoader::Instance().ClearCompletedCallbacks();
	}
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
		UE_LOG(RxCpp, Warning, TEXT("FStreamableDelegateCallback => '%s' in invalid."), *_softObjectPtr.ToString());

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

ARxCppLoader* ARxCppLoader::_pInstance = nullptr;

ARxCppLoader::ARxCppLoader()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ARxCppLoader::BeginDestroy()
{
	Super::BeginDestroy();

	ARxCppLoader::_pInstance = nullptr;
}

Rx::observable<TArray<ULevelStreaming*>> ARxCppLoader::LoadLevelStreaming(const FString& packageName, bool bMakeVisibleAfterLoad)
{
	//! 동시에 2개 이상의 스트리밍 요청을 할 수 없음
	if (_levelStreamingCallbacks.Num() > 0)
	{
		return Rx::observable<>::create<TArray<ULevelStreaming*>>([=, this](Rx::subscriber<TArray<ULevelStreaming*>> s) {
			UE_LOG(RxCpp, Warning, TEXT("ARxCppLoader => LoadLevelStreaming '%s' has called while other streaming is processed."), *packageName);
			
#if WITH_EDITOR
			//* 크래시 방지를 위해서 EDITOR 모드이면 on_next() 호출
			if (s.is_subscribed())
			{
				TArray<ULevelStreaming*> loadStreamingLevels;

				s.on_next(loadStreamingLevels);
				s.on_completed();
			}
#else
			if (s.is_subscribed())
				s.on_error(std::make_exception_ptr(std::exception("streaming processed is overlapped")));
#endif
		});
	}

	// _levelStreamingCallbacks.Empty();

	auto adjustPackageName = ConvertToWorldAssetPackageName(packageName);

	return Rx::observable<>::create<TArray<ULevelStreaming*>>([=, this](Rx::subscriber<TArray<ULevelStreaming*>> s) {
		auto found = this->GetWorld()->StreamingLevels.FindLastByPredicate([=](auto v) {
			return v->GetWorldAssetPackageName() == adjustPackageName;
		});

		if (found == INDEX_NONE)
		{
			UE_LOG(RxCpp, Warning, TEXT("ARxCppLoader => '%s' is invalid package name."), *adjustPackageName);
			
#if WITH_EDITOR
			//* 크래시 방지를 위해서 EDITOR 모드이면 on_next() 호출
			if (s.is_subscribed())
			{
				s.on_next(TArray<ULevelStreaming*>());
				s.on_completed();
			}
#else
			if (s.is_subscribed())
				s.on_error(std::make_exception_ptr(std::exception("invalid package name")));
#endif

			return;
		}

		TArray<ULevelStreaming*> loadStreamingLevels;

		loadStreamingLevels.Add(this->GetWorld()->StreamingLevels[found]);

		auto* pCallback = NewObject<ULoadLevelStreamingCallback>()->Init(loadStreamingLevels, s, GetWorld(), adjustPackageName);

		_levelStreamingCallbacks.Add(pCallback);
		
		UGameplayStatics::LoadStreamLevel(GetWorld(), 
			FName(*FPackageName::GetShortName(loadStreamingLevels[0]->GetWorldAssetPackageFName())), bMakeVisibleAfterLoad, false, 
			pCallback->GetLatentActionInfo()
			);
	}).as_dynamic();
}

Rx::observable<TArray<ULevelStreaming*>> ARxCppLoader::LoadMap(const FString &packageName, bool bMakeVisibleAfterLoad)
{
	//! 동시에 2개 이상의 스트리밍 요청을 할 수 없음
	if (_levelStreamingCallbacks.Num() > 0)
	{
		return Rx::observable<>::create<TArray<ULevelStreaming*>>([=, this](Rx::subscriber<TArray<ULevelStreaming*>> s) {
			UE_LOG(RxCpp, Warning, TEXT("ARxCppLoader => LoadMap '%s' has called while other streaming is processed."), *packageName);
			
#if WITH_EDITOR
			//* 크래시 방지를 위해서 EDITOR 모드이면 on_next() 호출
			if (s.is_subscribed())
			{
				TArray<ULevelStreaming*> loadStreamingLevels;

				s.on_next(loadStreamingLevels);
				s.on_completed();
			}
#else
			if (s.is_subscribed())
				s.on_error(std::make_exception_ptr(std::exception("streaming processed is overlapped")));
#endif
		});
	}

	// _levelStreamingCallbacks.Empty();

	return Rx::observable<>::create<TArray<ULevelStreaming*>>([=, this](Rx::subscriber<TArray<ULevelStreaming*>> s) {
		TArray<ULevelStreaming*> loadStreamingLevels;

		auto sourceStreamingLevels = GetLevelStreamingFromPackage(packageName);

		//* 월드에 이미 로딩된 레벨은 제외해줌
		if (sourceStreamingLevels.Num() > 0)
		{
			for (auto l : sourceStreamingLevels)
			{
				auto sourcePackageName = ARxCppLoader::Instance().ConvertToWorldAssetPackageName(l->GetWorldAssetPackageName());
				auto persistentLevelName = GetWorld()->PersistentLevel->GetFullName();

				persistentLevelName = persistentLevelName.Left(persistentLevelName.Find(TEXT("."), ESearchCase::IgnoreCase, ESearchDir::FromEnd));

				// PersistentLevel도 체크
				if (persistentLevelName == sourcePackageName) 
					continue;

				auto found = GetWorld()->StreamingLevels.FindLastByPredicate([&sourcePackageName](auto v) {
					return v->GetWorldAssetPackageName() == sourcePackageName;
				}); 

				if (found == INDEX_NONE)
				{
					loadStreamingLevels.Add(CreateInstance(l, l->GetWorldAssetPackageName()));
				}
				else
				{
					// 이미 추가되었지만 로딩되지 않은 경우
					if (!GetWorld()->StreamingLevels[found]->IsLevelLoaded())
						loadStreamingLevels.Add(GetWorld()->StreamingLevels[found]);
				}
			}
		}

		if (loadStreamingLevels.Num() == 0)
		{
			UE_LOG(RxCpp, Warning, TEXT("ARxCppLoader => '%s' has 0 LevelStreaming (or package is invalid)."), *packageName);
			
#if WITH_EDITOR
			//* 크래시 방지를 위해서 EDITOR 모드이면 on_next() 호출
			if (s.is_subscribed())
			{
				s.on_next(loadStreamingLevels);
				s.on_completed();
			}
#else
			if (s.is_subscribed())
				s.on_error(std::make_exception_ptr(std::exception("invalid map")));
#endif

			return;
		}

		auto* pCallback = NewObject<ULoadLevelStreamingCallback>()->Init(loadStreamingLevels, s, GetWorld(), packageName);

		_levelStreamingCallbacks.Add(pCallback);
		
		for (auto &l : loadStreamingLevels)
		{
			if (!GetWorld()->StreamingLevels.Contains(l))
			{
				GetWorld()->StreamingLevels.Add(l);

				UE_LOG(RxCpp, Log, TEXT("ARxCppLoader => ULevelStreaming '%s' is added"), *FPackageName::GetShortName(l->GetWorldAssetPackageFName()));
			}

			UGameplayStatics::LoadStreamLevel(GetWorld(), 
				FName(*FPackageName::GetShortName(l->GetWorldAssetPackageFName())), bMakeVisibleAfterLoad, false, 
				pCallback->GetLatentActionInfo()
				);
				
			UE_LOG(RxCpp, Log, TEXT("ARxCppLoader => LoadStreamLevel() is called on ULevelStreaming '%s'"), *FPackageName::GetShortName(l->GetWorldAssetPackageFName()));
		}
	}).as_dynamic();
}

Rx::observable<bool> ARxCppLoader::UnloadLevelStreaming(TArray<ULevelStreaming*> unloadingLevels, bool removeAfterUnload, FString packageName)
{
	//! 동시에 2개 이상의 스트리밍 요청을 할 수 없음
	if (_levelStreamingCallbacks.Num() > 0)
	{
		return Rx::observable<>::create<bool>([=, this](Rx::subscriber<bool> s) {
			UE_LOG(RxCpp, Warning, TEXT("ARxCppLoader => UnloadLevelStreaming '%s' has called while other streaming is processed."), *packageName);
			
#if WITH_EDITOR
			//* 크래시 방지를 위해서 EDITOR 모드이면 on_next() 호출
			if (s.is_subscribed())
			{
				s.on_next(false);
				s.on_completed();
			}
#else
			if (s.is_subscribed())
				s.on_error(std::make_exception_ptr(std::exception("streaming processed is overlapped")));
#endif
		});
	}

	// _levelStreamingCallbacks.Empty();
	
	return Rx::observable<>::create<bool>([=, this](Rx::subscriber<bool> s) {
		TArray<ULevelStreaming*> unloadStreamingLevels;

		for (auto &l : unloadingLevels)
		{
			if (l->IsLevelLoaded())
				unloadStreamingLevels.Add(l);
		}

		if (unloadStreamingLevels.Num() == 0)
		{
			UE_LOG(RxCpp, Warning, TEXT("ARxCppLoader => Unloading %s... unloadingLevels size is 0"), *packageName);
			
			if (s.is_subscribed())
			{
				s.on_next(true);
				s.on_completed();
			}

			return;
		}

		auto* pCallback = NewObject<ULoadLevelStreamingCallback>()->Init(unloadStreamingLevels, s, GetWorld(), removeAfterUnload, packageName);

		_levelStreamingCallbacks.Add(pCallback);
		
		for (auto &l : unloadStreamingLevels)
		{
			UGameplayStatics::UnloadStreamLevel(GetWorld(), 
				FName(*FPackageName::GetShortName(l->GetWorldAssetPackageFName())),
				pCallback->GetLatentActionInfo()
				);
				
			UE_LOG(RxCpp, Log, TEXT("ARxCppLoader => UnloadStreamLevel() is called on ULevelStreaming '%s'"), *FPackageName::GetShortName(l->GetWorldAssetPackageFName()));
		}
	}).as_dynamic();
}

UWorld* ARxCppLoader::GetWorldFromPackage(const FString& packagePath)
{
	auto *pPkg = FindPackage(nullptr, *packagePath);

	if (pPkg == nullptr)
	{
		pPkg = LoadPackage(nullptr, *packagePath, LOAD_None);

		if (pPkg == nullptr)
		{
			UE_LOG(RxCpp, Warning, TEXT("ARxCppLoader => '%s' is not valid path"), *packagePath);
			return nullptr;
		}
	}

	return UWorld::FindWorldInPackage(pPkg);
}

FString ARxCppLoader::ConvertToWorldAssetPackageName(const FString& packageName)
{
	// Create prefix long package name 
	auto prefixShortPackageName = GetWorld()->StreamingLevelsPrefix + FPackageName::GetShortName(packageName);
	auto packagePath = FPackageName::GetLongPackagePath(packageName) + TEXT("/");

	return packagePath + prefixShortPackageName;
}

TArray<FString> ARxCppLoader::GetLevelStreamingNamesFromPackage(const FString& packagePath)
{
	TArray<FString> ret;

	auto *pWorld = GetWorldFromPackage(packagePath);

	if (pWorld == nullptr)
	{
		UE_LOG(RxCpp, Warning, TEXT("ARxCppLoader => '%s' has no UWorld object"), *packagePath);
		return ret;
	}

	for (auto &l : pWorld->StreamingLevels)
		ret.Add(ConvertToWorldAssetPackageName(l->GetWorldAssetPackageName()));

	return ret;
}

TArray<ULevelStreaming*> ARxCppLoader::GetLevelStreamingFromPackage(const FString &packagePath)
{
	auto *pWorld = GetWorldFromPackage(packagePath);

	if (pWorld == nullptr)
	{
		UE_LOG(RxCpp, Warning, TEXT("ARxCppLoader => '%s' has no UWorld object"), *packagePath);
		return TArray<ULevelStreaming*>();
	}

	return pWorld->StreamingLevels;
}

ULevelStreaming* ARxCppLoader::CreateInstance(ULevelStreaming* pSourceLevel, FString InstanceUniqueName)
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
		}
		else
		{
			UE_LOG(LogStreaming, Warning, TEXT("ARxCppLoader => Provided streaming level instance name is not unique: %s"), *InstanceUniquePackageName.ToString());
		}
	}
	
	return StreamingLevelInstance;
}

Rx::observable<UObject*> ARxCppLoader::LoadUObject(const FString& objectPath)
{
	return Rx::observable<>::create<UObject*>([=, this](Rx::subscriber<UObject*> s) {
		_streamableDelegateCallbacks.Add(objectPath, FStreamableDelegateCallback(objectPath, s, GetWorld()));

		_streamableDelegateCallbacks[objectPath]._streamableHandle = _streamableManager.RequestAsyncLoad(
			FSoftObjectPath(objectPath), 
			FStreamableDelegate::CreateRaw(&(_streamableDelegateCallbacks[objectPath]), &FStreamableDelegateCallback::CallbackImpl)
			);

		if (!_streamableDelegateCallbacks[objectPath]._streamableHandle.IsValid())
			_streamableDelegateCallbacks[objectPath].CallbackImpl();
	});
}

void ARxCppLoader::ClearCompletedCallbacks()
{
	_levelStreamingCallbacks.RemoveAll([](auto v) { return v->_isCompleted; });

	TArray<FString> keys;

	_streamableDelegateCallbacks.GenerateKeyArray(keys);

	for (auto& k : keys)
	{
		if (_streamableDelegateCallbacks[k]._isCompleted)
			_streamableDelegateCallbacks.Remove(k);
	}
}
