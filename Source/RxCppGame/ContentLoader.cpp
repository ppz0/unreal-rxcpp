#include "ContentLoader.h"


AContentLoader* AContentLoader::_pInstance = nullptr;

void ULoadLevelStreamingCallback::CallbackImpl()
{
	if (--_levelStreamingCount == 0)
	{
		_isCompleted = true;

		if (_subscriberHolder.Num() > 0)
		{
			_subscriberHolder.Last().on_next(_levelStreamings);
			_subscriberHolder.Last().on_completed();
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("_levelCount set to %d."), _levelStreamingCount);
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

	return Rx::observable<>::create<TArray<ULevelStreaming *>>([&, this](Rx::subscriber<TArray<ULevelStreaming*>> s) {
		auto streamingLevels = GetLevelStreamingFromPackage(packagePath);

		if (streamingLevels.Num() < 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("'%s' has 0 LevelStreaming"), *packagePath);
			return;
		}

		auto* pCallback = NewObject<ULoadLevelStreamingCallback>()->Init(streamingLevels, s);

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
	{
		// // //* 에디터 모드면 RenameForPIE() 메소드를 호출하여 이름을 변경해준다
		// // if (GIsPlayInEditorWorld) 
		// // {
		// // 	auto prevName = l->GetWorldAssetPackageName();
		// // 	l->RenameForPIE(GetWorld()->GetOutermost()->PIEInstanceID);

		// // 	UE_LOG(LogTemp, Warning, TEXT("ULevelStreaming '%s' is renamed to '%s'"), *prevName, *l->GetWorldAssetPackageName());
		// // }

		ret.Add(CreateInstance(l, l->GetWorldAssetPackageName()));
	}

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
