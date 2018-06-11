// Fill out your copyright notice in the Description page of Project Settings.

#include "RxCppGameGameModeBase.h"
#include "RxCpp/RxCppManager.h"

void ARxCppGameGameModeBase::BeginPlay()
{
    Super::BeginPlay();

    // FRxCppManager::Instance().Init(GetWorld());
}

void ARxCppGameGameModeBase::BeginDestroy()
{
    Super::BeginDestroy();
    
    // FRxCppManager::Instance().Destroy();
}