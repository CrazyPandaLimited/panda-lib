#pragma once
#include <memory>
#include <stdint.h>
#include <stddef.h>
#include <panda/cast.h>
#include <assert.h>

namespace panda {

class Refcnt {
public:
    void retain  () const { ++_refcnt; }
    void release () const {
        if (_refcnt > 1) --_refcnt;
        else delete this;
    }
    uint32_t refcnt () const { return _refcnt; }
protected:
    Refcnt () : _refcnt(0) {}
    virtual ~Refcnt () {}
private:
    mutable uint32_t _refcnt;
};

template <typename T>
class iptr {
public:
    template <class U> friend class iptr;
    typedef T element_type;

    iptr ()                   : ptr(NULL)    {}
    iptr (T* pointer)         : ptr(pointer) { if (ptr) refcnt_inc(ptr); }
    iptr (const iptr& oth)    : ptr(oth.ptr) { if (ptr) refcnt_inc(ptr); }
    template<class U>
    iptr (const iptr<U>& oth) : ptr(oth.ptr) { if (ptr) refcnt_inc(ptr); }

    iptr (iptr&& oth) {
        ptr = oth.ptr;
        oth.ptr = NULL;
    }

    template<class U>
    iptr (iptr<U>&& oth) {
        ptr = oth.ptr;
        oth.ptr = NULL;
    }

    ~iptr () { if (ptr) refcnt_dec(ptr); }

    iptr& operator= (T* pointer) {
        if (ptr) refcnt_dec(ptr);
        ptr = pointer;
        if (pointer) refcnt_inc(pointer);
        return *this;
    }

    iptr& operator= (const iptr& oth)    { return operator=(oth.ptr); }
    template<class U>
    iptr& operator= (const iptr<U>& oth) { return operator=(oth.ptr); }

    iptr& operator= (iptr&& oth) {
        std::swap(ptr, oth.ptr);
        return *this;
    }

    template<class U>
    iptr& operator= (iptr<U>&& oth) {
        if (ptr) refcnt_dec(ptr);
        ptr = oth.ptr;
        oth.ptr = NULL;
        return *this;
    }

    void reset () {
        if (ptr) refcnt_dec(ptr);
        ptr = NULL;
    }

    void reset (T* p) { operator=(p); }

    T* operator-> () const { return ptr; }
    T& operator*  () const { return *ptr; }
    operator T*   () const { return ptr; }
    explicit
    operator bool () const { return ptr; }

    T* get () const { return ptr; }

    uint32_t use_count () const { return refcnt_get(ptr); }

private:
    T* ptr;
};

inline void     refcnt_inc (const Refcnt* o) { o->retain(); }
inline void     refcnt_dec (const Refcnt* o) { o->release(); }
inline uint32_t refcnt_get (const Refcnt* o) { return o->refcnt(); }

template <typename T1, typename T2> inline iptr<T1> static_pointer_cast  (const iptr<T2>& ptr) { return iptr<T1>(static_cast<T1*>(ptr.get())); }
template <typename T1, typename T2> inline iptr<T1> const_pointer_cast   (const iptr<T2>& ptr) { return iptr<T1>(const_cast<T1*>(ptr.get())); }
template <typename T1, typename T2> inline iptr<T1> dynamic_pointer_cast (const iptr<T2>& ptr) { return iptr<T1>(dyn_cast<T1*>(ptr.get())); }

template <typename T1, typename T2> inline std::shared_ptr<T1> static_pointer_cast  (const std::shared_ptr<T2>& shptr) { return std::static_pointer_cast<T1>(shptr); }
template <typename T1, typename T2> inline std::shared_ptr<T1> const_pointer_cast   (const std::shared_ptr<T2>& shptr) { return std::const_pointer_cast<T1>(shptr); }
template <typename T1, typename T2> inline std::shared_ptr<T1> dynamic_pointer_cast (const std::shared_ptr<T2>& shptr) { return std::dynamic_pointer_cast<T1>(shptr); }

}
