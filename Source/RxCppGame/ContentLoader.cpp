#include "ContentLoader.h"


AContentLoader* AContentLoader::_pInstance = nullptr;

AContentLoader::AContentLoader()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AContentLoader::BeginPlay()
{
	Super::BeginPlay();
}

Rx::observable<TArray<ULevelStreaming*>> AContentLoader::LoadLevelStreaming(const FString &packagePath)
{
	//! 이전 LoadLevelStreaming은 삭제 (동시에 2개 이상의 요청을 할 수 없음을 의미함)
	_loadLevelStreamingCallbacks.Empty();

	return Rx::observable<>::create<TArray<ULevelStreaming *>>([this, &packagePath](Rx::subscriber<TArray<ULevelStreaming*>> s) {
		auto streamingLevels = GetLevelStreamingFromPackage(packagePath);

		if (streamingLevels.Num() < 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("'%s' has 0 LevelStreaming"), *packagePath);
			return;
		}

		auto* pCallback = NewObject<ULoadLevelStreamingCallback>()->Init(streamingLevels, &s);

		_loadLevelStreamingCallbacks.Add(pCallback);
		
		for (auto &l : streamingLevels)
		{
			GetWorld()->StreamingLevels.Add(l);

			UGameplayStatics::LoadStreamLevel(GetWorld(), 
				FName(*FPackageName::GetShortName(l->GetWorldAssetPackageFName())), false, false, 
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
		UE_LOG(LogTemp, Warning, TEXT("ULevelStreaming '%s' is added"), *FPackageName::GetShortName(l->GetWorldAssetPackageFName()));
		ret.Add(l);
	}

	return ret;
}
