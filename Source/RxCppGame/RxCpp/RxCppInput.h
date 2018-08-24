#pragma once

#include "CoreMinimal.h"
#include "Kismet/GameplayStatics.h"
#include "RxCpp/RxCppManager.h"
#include "RxCppInput.generated.h"


UCLASS()
class UBindKeyCallback : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
		void CallbackImpl();

private:
    FKey _key;
    EInputEvent _keyEvent;
	TArray<Rx::subscriber<bool>> _subscriberHolder;

friend class ARxCppInput;
};

UCLASS()
class UBindActionCallback : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
		void CallbackImpl();

private:
    FName _actionName;
    EInputEvent _keyEvent;
	TArray<Rx::subscriber<bool>> _subscriberHolder;

friend class ARxCppInput;
};

UCLASS()
class UBindAxisCallback : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
		void CallbackImpl(float value);

private:
    FName _axisName;
	TArray<Rx::subscriber<float>> _subscriberHolder;

friend class ARxCppInput;
};


UCLASS()
class RXCPPGAME_API ARxCppInput : public AInfo
{
	GENERATED_BODY()
	
public:
	ARxCppInput();

protected:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
	
#pragma region 싱글턴
public:
	static ARxCppInput& Instance()	
	{ 
		if (_pInstance == nullptr)
			_pInstance = (ARxCppInput*)const_cast<UWorld*>(FRxCppManager::Instance().pWorld)->SpawnActor(ARxCppInput::StaticClass());

		return *_pInstance; 
	}
private:
	static ARxCppInput* _pInstance;
#pragma endregion

public:
    void ResetInputComponent(APlayerController* pPlayerCtrler);

public:
    Rx::observable<bool> KeyPressed(const FKey& key);
    Rx::observable<bool> KeyRepeated(const FKey& key);
    Rx::observable<bool> KeyReleased(const FKey& key);
	
    Rx::observable<bool> ActionPressed(const FName& actionName);
    Rx::observable<bool> ActionRepeated(const FName& actionName);
    Rx::observable<bool> ActionReleased(const FName& actionName);
	
    Rx::observable<float> Axis(const FName& axisName);

private:
    UBindKeyCallback* BindKey(const FKey& key, const EInputEvent& keyEvent);
    UBindActionCallback* BindAction(const FName& actionName, const EInputEvent& keyEvent);
    UBindAxisCallback* BindAxis(const FName& axisName);

    UPROPERTY(Transient)
        TArray<UBindKeyCallback*> _bindKeyCallbacks;
		
    UPROPERTY(Transient)
        TArray<UBindActionCallback*> _bindActionCallbacks;
		
    UPROPERTY(Transient)
        TArray<UBindAxisCallback*> _bindAxisCallbacks;

};
