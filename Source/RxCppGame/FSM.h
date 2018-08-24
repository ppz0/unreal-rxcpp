#pragma once

#include "CoreMinimal.h"
#include "RxCpp/RxCppManager.h"


template <typename T>
class FSM {
    
public:
    T prev;
    T curr;

    void SetState(T state) {
        prev = this->curr;
        curr = state;
        
        for (int32 i = _onExitSubscriberHolder.Num()-1; i >= 0; --i)
        {
            if (_onExitSubscriberHolder[i].is_subscribed())
                _onExitSubscriberHolder[i].on_next(curr);
            else
                _onExitSubscriberHolder.RemoveAt(i, 1, false);
        }
        
        for (int32 i = _onEnterSubscriberHolder.Num()-1; i >= 0; --i)
        {
            if (_onEnterSubscriberHolder[i].is_subscribed())
                _onEnterSubscriberHolder[i].on_next(prev);
            else
                _onEnterSubscriberHolder.RemoveAt(i, 1, false);
        }
    }

    T GetState() {
        return curr;
    }

    Rx::observable<T, Rx::dynamic_observable<T>> ListenOnEnter(T value) {
        return Rx::observable<>::create<T>([=, this](Rx::subscriber<T> s) { _onEnterSubscriberHolder.Add(s); })
            .filter([this](auto v) { return curr == v; })
            .as_dynamic();
    }

    Rx::observable<T, Rx::dynamic_observable<T>> ListenOnExit(T value) {
        return Rx::observable<>::create<T>([=, this](Rx::subscriber<T> s) { _onExitSubscriberHolder.Add(s); })
            .filter([this](auto v) { return prev == v; })
            .as_dynamic();
    }

private:
    TArray<Rx::subscriber<T>> _onEnterSubscriberHolder;
    TArray<Rx::subscriber<T>> _onExitSubscriberHolder;

};
