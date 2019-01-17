#pragma once
#include <vector>
#include <memory>
#include <stdexcept>
#include <assert.h>

namespace panda { namespace lib __attribute__ ((visibility ("default"))) {

class MemoryPool {
public:
    MemoryPool (size_t blocksize) : first_free(NULL) {
        this->blocksize = round_up(blocksize);
    }

    void* allocate () {
        if (!first_free) grow();
        void* ret = first_free;
        first_free = *((void**)ret);
        return ret;
    }

    void deallocate (void* elem) {
#ifdef TEST_FULL
        if(!is_mine(elem)) abort(); // protection for debugging, normally you MUST NEVER pass a pointer that wasn't created via current mempool
#endif
        *((void**)elem) = first_free;
        first_free = elem;
    }

    ~MemoryPool ();

private:
    struct Pool {
        char*  list;
        size_t size;
        size_t len;
    };
    size_t            blocksize;
    std::vector<Pool> pools;
    void*             first_free;

    void grow    ();
    bool is_mine (void* elem);

    inline static size_t round_up(size_t size) {
        assert(size > 0);
        const size_t factor = sizeof(void*);
        if ((size & (factor-1)) == 0) return size;
        size += factor;
        size &= ~((size_t)(factor-1));
        return size;
    }

};

// !!!! It cannot be used !!!!
// StaticMemoryPool::instance and StaticMemoryPool::tls_instance are different in different .so
// Let it stay commented till probles is not solved
// !!!!!!!!! DO NOT return &_tls_inst from tls_instance() !!!!!!!!!!!! On some compilers, performance drop down up to 4x may occur.
//template <int BLOCKSIZE>
//class StaticMemoryPool {
//public:
//    static MemoryPool* instance () { return &_inst; }


//    static MemoryPool*  tls_instance () {
//        static thread_local MemoryPool*  ptr;
//        if (!ptr) {
//            static thread_local MemoryPool inst(BLOCKSIZE);
//            ptr = &inst;
//        }
//        return ptr;
//    }

//private:
//    static MemoryPool _inst;
//};

//template <int T>
//MemoryPool StaticMemoryPool<T>::_inst(T);

//template <> struct StaticMemoryPool<7> : StaticMemoryPool<8> {};
//template <> struct StaticMemoryPool<6> : StaticMemoryPool<8> {};
//template <> struct StaticMemoryPool<5> : StaticMemoryPool<8> {};
//template <> struct StaticMemoryPool<4> : StaticMemoryPool<8> {};
//template <> struct StaticMemoryPool<3> : StaticMemoryPool<8> {};
//template <> struct StaticMemoryPool<2> : StaticMemoryPool<8> {};
//template <> struct StaticMemoryPool<1> : StaticMemoryPool<8> {};

class ObjectAllocator {
public:
    static ObjectAllocator* instance     () { return _inst; }
    static ObjectAllocator* tls_instance () { return _tls_inst; }

    ObjectAllocator ();

    void* allocate (size_t size) {
        if (size == 0) return NULL;
        MemoryPool* pool;
        if (size <= 1024) {
            pool = small_pools[(size-1)>>2];
            if (!pool) pool = small_pools[(size-1)>>2] = new MemoryPool((((size-1)>>2) + 1)<<2);
        }
        else if (size <= 16384) {
            pool = medium_pools[(size-1)>>6];
            if (!pool) pool = medium_pools[(size-1)>>6] = new MemoryPool((((size-1)>>6) + 1)<<6);
        }
        else if (size <= 262144) {
            pool = big_pools[(size-1)>>10];
            if (!pool) pool = big_pools[(size-1)>>10] = new MemoryPool((((size-1)>>10) + 1)<<10);
        }
        else throw std::invalid_argument("ObjectAllocator: object size cannot exceed 256k");

        return pool->allocate();
    }

    void deallocate (void* ptr, size_t size) {
        if (ptr == NULL || size == 0) return;
        MemoryPool* pool;
        if      (size <= 1024)   pool = small_pools[(size-1)>>2];
        else if (size <= 16384)  pool = medium_pools[(size-1)>>6];
        else if (size <= 262144) pool = big_pools[(size-1)>>10];
        else throw std::invalid_argument("ObjectAllocator: object size cannot exceed 256k");
        pool->deallocate(ptr);
    }

    ~ObjectAllocator ();

private:
    static constexpr const int POOLS_CNT = 256;
    static ObjectAllocator* _inst;
    static thread_local ObjectAllocator* _tls_inst;
    MemoryPool* small_pools[POOLS_CNT];
    MemoryPool* medium_pools[POOLS_CNT];
    MemoryPool* big_pools[POOLS_CNT];

};

template <class TARGET, bool TLS_ALLOC = true>
struct AllocatedObject {
    static void* operator new (size_t size) {
        return ObjectAllocator::tls_instance()->allocate(size);
    }

    static void operator delete (void* p, size_t size) {
        ObjectAllocator::tls_instance()->deallocate(p, size);
    }
};

template <class TARGET>
struct AllocatedObject<TARGET, false> {
    static void* operator new (size_t size) {
        return ObjectAllocator::instance()->allocate(size);
    }

    static void operator delete (void* p, size_t size) {
        ObjectAllocator::instance()->deallocate(p, size);
    }
};

}}
