#pragma once

#include <panda/refcnt.h>
#include <iostream>
#include <panda/lib/traits.h>

namespace panda {

using panda::shared_ptr;

template <typename Ret, typename... Args>
class function;

namespace function_details {
    struct AnyFunction {
        virtual ~AnyFunction() {}

        virtual const AnyFunction* get_base() const {
            return this;
        }
    };
}


template <typename Ret, typename... Args>
struct Ifunction : public function_details::AnyFunction, virtual public RefCounted {
    virtual ~Ifunction() {}
    virtual Ret operator()(Args...) = 0;
    virtual bool equals(const function_details::AnyFunction* oth) const = 0;
};

namespace function_details {

template <typename Func, typename Ret, bool Comparable, typename... Args>
class abstract_function {};

template <typename Func>
class storage {
public:
    template <typename F>
    explicit storage(F&& f) : func(std::forward<F>(f)) {}

    typename std::remove_reference<Func>::type func;
};

template <typename Func, typename Ret, bool SELF, typename Self,  typename... Args>
class callable {};

template <typename Func, typename Ret, typename Self, typename... Args>
class callable<Func, Ret, false, Self, Args...> : public storage<Func>
{
public:
    template <typename F>
    explicit callable(F&& f) : storage<Func>(std::forward<F>(f)) {}

    Ret operator()(Self&, Args... args) {
        return this->func(args...);
    }
};


template <typename Func, typename Ret, typename Self, typename... Args>
class callable<Func, Ret, true, Self, Args...> : public storage<Func>
{
public:
    template <typename F>
    explicit callable(F&& f) : storage<Func>(std::forward<F>(f)) {}

    Ret operator()(Self& self, Args... args) {
        return this->func(self, args...);
    }
};

template <typename Func, typename Ret, typename... Args>
class abstract_function<Func, Ret, true, Args...> : public Ifunction<Ret, Args...>, public storage<Func> {
public:
    template <typename F>
    explicit abstract_function(F&& f) : storage<Func>(std::forward<F>(f)) {}

    Ret operator()(Args... args) override {
        static_assert(std::is_convertible<decltype(this->func(args...)), Ret>::value, "return type mismatch");
        return this->func(args...);
    }

    bool equals(const AnyFunction* oth) const override {
        auto foth = dynamic_cast<const storage<Func>*>(oth->get_base());
        if (foth == nullptr) {
            return false;
        }

        return this->func == foth->func;
    }
};

template <typename Func, typename Ret, typename... Args>
class abstract_function<Func, Ret, false, Args...> : public Ifunction<Ret, Args...>
        , public callable<Func, Ret, !lib::traits::has_call_operator<Func, Args...>::value, Ifunction<Ret, Args...>, Args...>
{
public:
    using Derfed = typename std::remove_reference<Func>::type;
    using Caller = callable<Func, Ret, !lib::traits::has_call_operator<Func, Args...>::value, Ifunction<Ret, Args...>, Args...>;

    template <typename F>
    explicit abstract_function(F&& f) : Caller(std::forward<F>(f)) {}

    Ret operator()(Args... args) override {
        return Caller::operator()(*this, args...);
    }

    bool equals(const AnyFunction* oth) const override {
        return this->get_base() == oth->get_base();
    }

private:
    template <Ret(Derfed::*meth)(Args...)>
    Ret call(Args... args) {
       return (this->template func.*meth)(args...);
    }
};


template <typename FRet, typename... FArgs, typename Ret, typename... Args>
class abstract_function<FRet (*)(FArgs...), Ret, true, Args...> : public Ifunction<Ret, Args...>, public storage<FRet (*)(FArgs...)> {
public:
    using Func = FRet (*)(FArgs...);
    explicit abstract_function(const Func& f) : storage<Func>(f) {}

    Ret operator()(Args... args) override {
        return this->func(args...);
    }

    bool equals(const AnyFunction* oth) const override {
        auto foth = dynamic_cast<const storage<Func>*>(oth->get_base());
        if (foth == nullptr) return false;

        return this->func == foth->func;
    }
};

template <typename From, typename Ret, typename... Args>
struct function_caster : public Ifunction<Ret, Args...> {
    From src;
    using Check = std::is_base_of<AnyFunction, decltype(*src)>;

    function_caster(From src) : src(src) {}

    Ret operator()(Args... args) override {
        return src->operator()(args...);
    }

    bool equals(const function_details::AnyFunction* oth) const override {
        return src->equals(oth);
    }

    const AnyFunction* get_base() const override {
        return src->get_base();
    }


};

template <typename... Tail>
struct is_panda_function_t {
    static constexpr bool value = false;
};

template <typename... Params>
struct is_panda_function_t<function<Params...>> {
    static constexpr bool value = true;
};

template <typename Ret, typename... Args>
auto make_abstract_function(Ret (*f)(Args...)) -> shared_ptr<abstract_function<Ret (*)(Args...), Ret, true, Args...>> {
    return panda::make_shared<abstract_function<Ret (*)(Args...), Ret, true, Args...>>(f);
}

template <typename Ret, typename... Args>
auto tmp_abstract_function(Ret (*f)(Args...)) -> abstract_function<Ret (*)(Args...), Ret, true, Args...> {
    return abstract_function<Ret (*)(Args...), Ret, true, Args...>(f);
}

template <typename Ret, typename... Args,
          typename Functor, bool IsComp = lib::traits::is_comparable<typename std::remove_reference<Functor>::type>::value,
          typename DeFunctor = typename std::remove_reference<Functor>::type,
          typename Check = decltype(std::declval<Functor>()(std::declval<Args>()...)),
          typename = typename std::enable_if<!std::is_same<Functor, Ret(&)(Args...)>::value>::type,
          typename = typename std::enable_if<!is_panda_function_t<DeFunctor>::value>::type>
shared_ptr<abstract_function<DeFunctor, Ret, IsComp, Args...>> make_abstract_function(Functor&& f, Check(*)() = 0) {
    return panda::make_shared<abstract_function<DeFunctor, Ret, IsComp, Args...>>(std::forward<Functor>(f));
}

template <typename Ret, typename... Args,
          typename Functor, bool IsComp = lib::traits::is_comparable<typename std::remove_reference<Functor>::type>::value,
          typename DeFunctor = typename std::remove_reference<Functor>::type,
          typename Check = decltype(std::declval<Functor>()(std::declval<Args>()...)),
          typename = typename std::enable_if<!std::is_same<Functor, Ret(&)(Args...)>::value>::type>
abstract_function<DeFunctor, Ret, IsComp, Args...> tmp_abstract_function(Functor&& f, Check(*)() = 0) {
    return abstract_function<DeFunctor, Ret, IsComp, Args...>(std::forward<Functor>(f));
}

template <typename Ret, typename... Args,
          typename Functor, bool IsComp = lib::traits::is_comparable<typename std::remove_reference<Functor>::type>::value,
          typename DeFunctor = typename std::remove_reference<Functor>::type,
          typename Check = decltype(std::declval<Functor>()(std::declval<Ifunction<Ret, Args...>&>(), std::declval<Args>()...))>
shared_ptr<abstract_function<DeFunctor, Ret, IsComp, Args...>> make_abstract_function(Functor&& f) {
    return panda::make_shared<abstract_function<DeFunctor, Ret, IsComp, Args...>>(std::forward<Functor>(f));
}

template <typename Ret, typename... Args,
          typename Functor, bool IsComp = lib::traits::is_comparable<typename std::remove_reference<Functor>::type>::value,
          typename DeFunctor = typename std::remove_reference<Functor>::type,
          typename Check = decltype(std::declval<Functor>()(std::declval<Ifunction<Ret, Args...>&>(), std::declval<Args>()...))>
abstract_function<DeFunctor, Ret, IsComp, Args...> tmp_abstract_function(Functor&& f) {
    return abstract_function<DeFunctor, Ret, IsComp, Args...>(std::forward<Functor>(f));
}

template <typename Ret, typename... Args, typename ORet, typename... OArgs,
          typename = typename std::enable_if<lib::traits::has_call_operator<function<ORet, OArgs...>, Args...>::value>::type>
auto make_abstract_function(const function<ORet, OArgs...>& func) -> shared_ptr<function_caster<decltype(func.func), Ret, Args...>> {
    return new function_caster<decltype(func.func), Ret, Args...>(func.func);
}


template <class Class, typename Ret, typename... Args>
struct method : public Ifunction<Ret, Args...>{
    using Method = Ret (Class::*)(Args...);
    using ifunction = Ifunction<Ret, Args...>;

    method(Method method, shared_ptr<Class> thiz = nullptr) : thiz(thiz), meth(method) {}
    shared_ptr<method, true> bind(shared_ptr<Class> thiz) {
        this->thiz = thiz;
        return shared_ptr<method, true>(this);
    }

    Ret operator()(Args... args) override {
        return (thiz.get()->*meth)(std::forward<Args>(args)...);
    }

    bool equals(const AnyFunction* oth) const override {
        auto moth = dynamic_cast<const method<Class, Ret, Args...>*>(oth->get_base());
        if (moth == nullptr) return false;

        return operator ==(*moth);
    }

    bool operator==(const method& oth) const {
        return thiz == oth.thiz && meth == oth.meth;
    }

    bool operator !=(const method& oth) const {
        return !operator ==(oth);
    }

    explicit operator bool() const {
        return thiz && meth;
    }

private:
    shared_ptr<Class> thiz;
    Method meth;
};

template <class Class, typename Ret, typename... Args>
inline shared_ptr<method<Class, Ret, Args...>> make_method(Ret (Class::*meth)(Args...), shared_ptr<Class> thiz = nullptr) {
    return panda::make_shared<method<Class, Ret, Args...>>(meth, thiz);
}

template <typename Ret, typename... Args, class Class>
inline shared_ptr<method<Class, Ret, Args...>> make_abstract_function(Ret (Class::*meth)(Args...), shared_ptr<Class> thiz = nullptr) {
    return panda::make_shared<method<Class, Ret, Args...>>(meth, thiz);
}

template <typename Ret, typename... Args, class Class>
inline method<Class, Ret, Args...> tmp_abstract_function(Ret (Class::*meth)(Args...), shared_ptr<Class> thiz = nullptr) {
    return method<Class, Ret, Args...>(meth, thiz);
}
}

}
