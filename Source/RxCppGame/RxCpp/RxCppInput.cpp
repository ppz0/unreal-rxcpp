#include "RxCppInput.h"

void UBindKeyCallback::CallbackImpl()
{
    for (int32 i = _subscriberHolder.Num()-1; i >= 0; --i)
    {
        if (_subscriberHolder[i].is_subscribed())
            _subscriberHolder[i].on_next(true);
        else
            _subscriberHolder.RemoveAt(i, 1, false);
    }
}

void UBindActionCallback::CallbackImpl()
{
    for (int32 i = _subscriberHolder.Num()-1; i >= 0; --i)
    {
        if (_subscriberHolder[i].is_subscribed())
            _subscriberHolder[i].on_next(true);
        else
            _subscriberHolder.RemoveAt(i, 1, false);
    }
}

void UBindAxisCallback::CallbackImpl(float value)
{
    for (int32 i = _subscriberHolder.Num()-1; i >= 0; --i)
    {
        if (_subscriberHolder[i].is_subscribed())
            _subscriberHolder[i].on_next(value);
        else
            _subscriberHolder.RemoveAt(i, 1, false);
    }
}


ARxCppInput* ARxCppInput::_pInstance = nullptr;

ARxCppInput::ARxCppInput()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ARxCppInput::BeginPlay() 
{
    Super::BeginPlay();

    auto* pDefaultPlayerCtrler = UGameplayStatics::GetPlayerController(this, 0);

    if (pDefaultPlayerCtrler == nullptr) 
    {
        UE_LOG(RxCpp, Warning, TEXT("ARxCppInput => Failed to enable InputComponent. PlayerController is not spawned, yet~"));
        return;
    }
    
    ResetInputComponent(pDefaultPlayerCtrler);

}
void ARxCppInput::BeginDestroy()
{
	Super::BeginDestroy();

	ARxCppInput::_pInstance = nullptr;
}

void ARxCppInput::ResetInputComponent(APlayerController* pPlayerCtrler)
{
    EnableInput(pPlayerCtrler);
    
    UE_LOG(RxCpp, Warning, TEXT("ARxCppInput => InputComponent is enabled. Host PlayerController is '%s'"), *pPlayerCtrler->GetName());
}

Rx::observable<bool> ARxCppInput::KeyPressed(const FKey& key)
{
	return Rx::observable<>::create<bool>([=, this](Rx::subscriber<bool> s) {
        BindKey(key, EInputEvent::IE_Pressed)->_subscriberHolder.Add(s);
    });
}

Rx::observable<bool> ARxCppInput::KeyRepeated(const FKey& key)
{
	return Rx::observable<>::create<bool>([=, this](Rx::subscriber<bool> s) {
        BindKey(key, EInputEvent::IE_Repeat)->_subscriberHolder.Add(s);
    });
}

Rx::observable<bool> ARxCppInput::KeyReleased(const FKey& key)
{
	return Rx::observable<>::create<bool>([=, this](Rx::subscriber<bool> s) {
        BindKey(key, EInputEvent::IE_Released)->_subscriberHolder.Add(s);
    });
}

Rx::observable<bool> ARxCppInput::ActionPressed(const FName& actionName)
{
	return Rx::observable<>::create<bool>([=, this](Rx::subscriber<bool> s) {
        BindAction(actionName, EInputEvent::IE_Pressed)->_subscriberHolder.Add(s);
    });
}

Rx::observable<bool> ARxCppInput::ActionRepeated(const FName& actionName)
{
	return Rx::observable<>::create<bool>([=, this](Rx::subscriber<bool> s) {
        BindAction(actionName, EInputEvent::IE_Repeat)->_subscriberHolder.Add(s);
    });
}

Rx::observable<bool> ARxCppInput::ActionReleased(const FName& actionName)
{
	return Rx::observable<>::create<bool>([=, this](Rx::subscriber<bool> s) {
        BindAction(actionName, EInputEvent::IE_Released)->_subscriberHolder.Add(s);
    });
}

Rx::observable<float> ARxCppInput::Axis(const FName& axisName)
{
	return Rx::observable<>::create<float>([=, this](Rx::subscriber<float> s) {
        BindAxis(axisName)->_subscriberHolder.Add(s);
    });
}

UBindKeyCallback* ARxCppInput::BindKey(const FKey& key, const EInputEvent& keyEvent)
{
    auto found = _bindKeyCallbacks.FindLastByPredicate([=](auto v) {
        return v->_key == key && v->_keyEvent == keyEvent;
    });

    UBindKeyCallback* pCallback = nullptr;

    if (found == INDEX_NONE) 
    {
        pCallback = NewObject<UBindKeyCallback>();
        pCallback->_key = key;
        pCallback->_keyEvent = keyEvent;

        _bindKeyCallbacks.Add(pCallback);
        
        InputComponent->BindKey(key, keyEvent, pCallback, &UBindKeyCallback::CallbackImpl);
    }
    else
    {
        pCallback = _bindKeyCallbacks[found];
    }

    return pCallback;
}


UBindActionCallback* ARxCppInput::BindAction(const FName& actionName, const EInputEvent& keyEvent)
{
    auto found = _bindActionCallbacks.FindLastByPredicate([=](auto v) {
        return v->_actionName == actionName && v->_keyEvent == keyEvent;
    });

    UBindActionCallback* pCallback = nullptr;

    if (found == INDEX_NONE) 
    {
        pCallback = NewObject<UBindActionCallback>();
        pCallback->_actionName = actionName;
        pCallback->_keyEvent = keyEvent;

        _bindActionCallbacks.Add(pCallback);
        
        InputComponent->BindAction(actionName, keyEvent, pCallback, &UBindActionCallback::CallbackImpl);
    }
    else
    {
        pCallback = _bindActionCallbacks[found];
    }

    return pCallback;
}

UBindAxisCallback* ARxCppInput::BindAxis(const FName& axisName)
{
    auto found = _bindAxisCallbacks.FindLastByPredicate([=](auto v) {
        return v->_axisName == axisName;
    });

    UBindAxisCallback* pCallback = nullptr;

    if (found == INDEX_NONE) 
    {
        pCallback = NewObject<UBindAxisCallback>();
        pCallback->_axisName = axisName;

        _bindAxisCallbacks.Add(pCallback);
        
        InputComponent->BindAxis(axisName, pCallback, &UBindAxisCallback::CallbackImpl);
    }
    else
    {
        pCallback = _bindAxisCallbacks[found];
    }

    return pCallback;
}
