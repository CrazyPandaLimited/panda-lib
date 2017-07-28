#pragma once

#include <panda/refcnt.h>

namespace panda {
using namespace std;

using panda::make_shared;
using panda::shared_ptr;

template <typename Ret, typename... Args>
struct Ifunction : public RefCounted {
    virtual ~Ifunction() {}
    virtual Ret operator()(Args...) = 0;
    virtual bool equals(Ifunction* oth) const = 0;
};

template<typename T, bool Trivial = std::is_class<T>::value>
struct is_comparable {
    static const bool value = true;
};

template<typename T>
struct is_comparable<T, true> {
    struct fallback { bool operator==(const fallback& oth); };
    struct mixed_type: std::remove_reference<T>::type, fallback {};
    template < typename U, U > struct type_check {};

    template < typename U > static std::false_type  test( type_check< bool (fallback::*)(const fallback&), &U::operator== >* = 0 );
    template < typename U > static std::true_type   test( ... );

//    static const bool value = sizeof( yes ) == sizeof( test<mixed_type>(nullptr) );

    static const bool value = std::is_same<decltype(test<mixed_type>(nullptr)), std::true_type>::value;
};


template <typename Func, typename Ret, bool Comparable, typename... Args>
class abstract_function {};


template <typename Func, typename Ret, typename... Args>
class abstract_function<Func, Ret, true, Args...> : public Ifunction<Ret, Args...>{
public:
    template <typename F>
    explicit abstract_function(F&& f) : func(std::forward<F>(f)) {}

    Ret operator()(Args... args) override {
        static_assert(std::is_same<decltype(func(args...)), Ret>::value, "return type mismatch");
        return func(args...);
    }

    bool equals(Ifunction<Ret, Args...>* oth) const override {
        auto foth = dynamic_cast<abstract_function*>(oth);
        if (foth == nullptr) return false;

        return func == foth->func;
    }

    Func func;
};

template <typename Func, typename Ret, typename... Args>
class abstract_function<Func, Ret, false, Args...> : public Ifunction<Ret, Args...>{
public:
    template <typename F>
    explicit abstract_function(F&& f) : func(std::forward<F>(f)) {}

    Ret operator()(Args... args) override {
        return func(args...);
    }

    bool equals(Ifunction<Ret, Args...>* oth) const override {
        return this == oth;
    }

    Func func;
};


template <typename Ret, typename... Args>
class abstract_function<Ret (*)(Args...), Ret, true, Args...> : public Ifunction<Ret, Args...> {
public:
    using Func = Ret (*)(Args...);
    explicit abstract_function(const Func& f) : func(f) {}

    Ret operator()(Args... args) override {
        return func(args...);
    }

    bool equals(Ifunction<Ret, Args...>* oth) const override {
        auto foth = dynamic_cast<abstract_function*>(oth);
        if (foth == nullptr) return false;

        return func == foth->func;
    }

    Ret (*func)(Args...);
};

template <typename Ret, typename... Args>
auto make_abstract_function(Ret (*f)(Args...)) -> shared_ptr<abstract_function<Ret (*)(Args...), Ret, true, Args...>> {
    return make_shared<abstract_function<Ret (*)(Args...), Ret, true, Args...>>(f);
}

template <typename Ret, typename... Args,
          typename Functor, bool IsComp = is_comparable<typename std::remove_reference<Functor>::type>::value,
          typename = decltype(std::declval<Functor>()(declval<Args>()...))>
shared_ptr<abstract_function<Functor, Ret, IsComp, Args...>> make_abstract_function(Functor&& f) {
    return make_shared<abstract_function<Functor, Ret, IsComp, Args...>>(std::forward<Functor>(f));
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

    bool equals(ifunction* oth) const override {
        auto moth = dynamic_cast<method<Class, Ret, Args...>*>(oth);
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
    return make_shared<method<Class, Ret, Args...>>(meth, thiz);
}

template <typename Ret, typename... Args, class Class>
inline shared_ptr<method<Class, Ret, Args...>> make_abstract_function(Ret (Class::*meth)(Args...), shared_ptr<Class> thiz = nullptr) {
    return make_shared<method<Class, Ret, Args...>>(meth, thiz);
}
}
