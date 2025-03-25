#pragma once

#include <iostream>
#include "MemoryPool.h"
#include <unordered_set>
#include <shared_mutex>
#define PAGE_SIZE 4096


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
    friend class Memory; //将memory给设置成友元，这样我们就能设置私有变量count了
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
        std::lock_guard<std::shared_mutex> locker(poolMutex_);
        size_t totalSize = size*sizeof(T); // 计算总分配大小，包含前后保护字节
        char* ptr = (char*)pool_.malloc(totalSize);     // 分配内存
        if (!ptr) return MemoryView<T>(nullptr, 0);                // 分配失败返回空指针
        return MemoryView<T>((T*)(ptr), size); // 返回用户可用内地址，跳过前保护字节 强转
    }

    // 释放内存，并检查是否越界访问  这个只能在.h中否则一定要在cpp中实例化
    // 这个可能要设计成，大块内存就到获取锁的队列，而小块内存优先级更高，因为它不归还给操作系统，所以我们可以直接先释放。
    template<typename T>
    void deallocate(MemoryView<T>& memView) {
        std::unique_lock<std::shared_mutex> locker(poolMutex_);
        if (!memView.getBasePtr()) return;
        T* basePtr = memView.getBasePtr();
        int count=memView.count;
        memView.count=0;
        memView.basePtr=nullptr; //将这个对象进行清空
        if(count*sizeof(T)<PAGE_SIZE){
            // 小块内存独占锁不释放，因为这个涉及到里边的一个hash表的修改
            pool_.freeMemory((void *)basePtr); // 释放完整内存块
        }else {
            locker.unlock(); 
            bool insertBool = false;
            {
                std::lock_guard<std::mutex> locker1(largeSetMutex_); 
                insertBool = largeDeal_.insert((void *)basePtr).second; //用来保存插入成功还是失败
            }
            if(insertBool){    //如果是第一次对这个节点进行  
                /*
                下边的释放后再获取共享锁，这个我是有一个想法的（假设u代表独占锁，s代表共享锁） ，
                如果A：u，B: 等待释放，C: 申请地址。   
                    1. A由u到unlock时，这个瞬间B获得了锁，如果释放的地址是小块的，此时锁不释放直接freeMemory，
                                                        如果地址是大块的，1 地址相同，则不会insert成功，return
                                                                        2  地址不同，插入成功，释放独占锁，如果它能获取共享锁，
                                                                            则A肯定也可以获取，它俩删除的内存不一样，不会有问题。
                    2. A由u到unlock时，这个瞬间C获得了锁，C去申请地址，这个不会影响到这个largeDeal_
                */
                std::shared_lock<std::shared_mutex> locker(poolMutex_); //加上共享锁
                pool_.freeMemory((void *)basePtr); // 释放完整内存块
                //给删除的这个加上一个额外的锁，只在这一个位置使用。这个锁不会影响到别的，也不会被卡住。
                std::lock_guard<std::mutex> locker1(largeSetMutex_); 
                largeDeal_.erase((void *)basePtr); // 删除largeDeal_中刚刚的元素，表明删除结束。
            }
        }
    }

private:
    Memory *memory;
    MemoryPool pool_; //内存池模块
    std::shared_mutex poolMutex_;
    std::mutex largeSetMutex_;
    std::unordered_set<void *> largeDeal_; //保存此时正在删除的大块内存的首地址
};




