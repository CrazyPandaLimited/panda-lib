#pragma once

#include <list>
#include <panda/function.h>
#include <panda/optional.h>

namespace panda {

template <typename Ret, typename... Args>
class CallbackDispatcher {
public:
    struct Event;
    using Callback = function<Ret (Event&, Args...)>;
    using CallbackList = std::list<Callback>;

    using RetType = typename optional_tools<Ret>::type;

    struct Event {
        CallbackDispatcher& dispatcher;
        typename CallbackList::iterator state;

        template <typename... RealArgs>
        RetType next(RealArgs&&... args) {
            return dispatcher.next(*this, std::forward<RealArgs>(args)...);
        }
    };

    void add(const Callback& callback) {
        listeners.push_back(callback);
    }

    void add(Callback&& callback) {
        listeners.emplace_back(std::forward<Callback>(callback));
    }

    template <typename... RealArgs>
    RetType operator()(RealArgs&&... args) {
        auto iter = listeners.begin();
        if (iter == listeners.end()) return optional_tools<Ret>::default_value();

        Event e{*this, iter};
        return (*iter)(e, std::forward<RealArgs>(args)...);
    }

    template <typename SmthComparable>
    void remove(const SmthComparable& callback) {
        for (auto iter = listeners.rbegin(); iter != listeners.rend(); ++iter) {
            if (*iter == callback) {
                listeners.erase(std::next(iter).base() );
                break;
            }
        }
    }

private:
    template <typename... RealArgs>
    RetType next(Event& e, RealArgs&&... args) {
        ++e.state;
        if (e.state != listeners.end()) {
            return (*e.state)(e, std::forward<RealArgs>(args)...);
        } else {
            return optional_tools<Ret>::default_value();
        }
    }

    CallbackList listeners;
};

template <typename Ret, typename... Args>
class CallbackDispatcher<Ret(Args...)> : public CallbackDispatcher<Ret, Args...> {};

}
