
#include "Memory.h"


Memory *Memory::getInstance(){
    if(!memory){
        memory=new Memory();
    }
    return memory;
}

Memory::Memory(){
    pool_.createPool();
}

Memory::~Memory(){
    pool_.destroyPool();
}

void Memory::resetPool(){
    std::lock_guard<std::mutex> locker(poolMutex_);
    pool_.resetPool();
}