#pragma once

#include <iostream>
#include "MemoryPool.h"
#include <mutex>
#define GUARD_SIZE 4      // 保护字节大小
#define GUARD_VALUE 0xAB  // 保护字节填充值


//设计了这样一个类，用来保存已经分配内存的元素，这样就可以进行越界访问等一系列的事情。
template<typename T>
class MemoryView {
private:
    T* basePtr;  // 数据首地址
    size_t count;  // 元素个数

public:
    MemoryView(T* base, size_t cnt) : basePtr(base), count(cnt){}
    ~MemoryView()=default; //因为basePtr是用的别人的，所以没必要设置析构函数。
    // 下标访问运算符，进行越界检查
    T& operator[](size_t index) {
        if (index >= count) {
            std::cerr << "Index out of bounds!" << std::endl;
            abort();
        }
        return basePtr[index];
    }

    // 获取首地址
    T* getBasePtr() { return basePtr; }

    // 获取元素个数
    size_t getCount() { return count; }
};


class Memory {
public:
    Memory();
    ~Memory();
        

    Memory *getInstance();
    void resetPool();
    //我这样分配会使得内存给保护字节分配位置。
    // 分配内存，添加前后保护字节 这个只能在.h中否则一定要在cpp中实例化
    template <typename T>
    MemoryView<T> allocate(size_t size=1){
        std::lock_guard<std::mutex> locker(poolMutex_);
        size_t totalSize = size*sizeof(T); // 计算总分配大小，包含前后保护字节
        char* ptr = (char*)pool_.malloc(totalSize);     // 分配内存
        if (!ptr) return MemoryView<T>(nullptr, 0);                // 分配失败返回空指针
        return MemoryView<T>((T*)(ptr), size); // 返回用户可用内存地址，跳过前保护字节 强转
    }

    // 释放内存，并检查是否越界访问  这个只能在.h中否则一定要在cpp中实例化
    // 这个可能要设计成，大块内存就到获取锁的队列，而小块内存优先级更高，因为它不归还给操作系统，所以我们可以直接先释放。
    template<typename T>
    void deallocate(MemoryView<T>& memView) {
        if (!memView.getBasePtr()) return;
        T* basePtr = memView.getBasePtr();
        std::lock_guard<std::mutex> locker(poolMutex_);
        pool_.freeMemory((void *)basePtr); // 释放完整内存块
    }

private:
    Memory *memory;
    MemoryPool pool_; //内存池模块
    std::mutex poolMutex_;
};




