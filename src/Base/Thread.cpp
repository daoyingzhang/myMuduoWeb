#include <semaphore.h>
#include "Thread.h"
#include "CurrentThread.h"

// std::atomic_int32_t Thread::numCreated_(0);
std::atomic_int Thread::numCreated_(0);


Thread::Thread(ThreadFunc func, const std::string &name) :
    started_(false), // 还未开始
    joined_(false),  // 还未设置等待线程
    tid_(0),         // 初始 tid 设置为0
    func_(std::move(func)), // EventLoopThread::threadFunc()
    name_(name)     // 默认姓名是空字符串
{
    // 设置线程索引编号和姓名
    setDefaultName();
}

Thread::~Thread()
{
    // 现场感启动时并且未设置等待线程时才可调用
    if (started_ && !joined_)
    {
        // 设置线程分离(守护线程，不需要等待线程结束，不会产生孤儿线程)
        thread_->detach();
    }
}

void Thread::start()
{
    started_ = true;
    sem_t sem;

    /**
     * int sem_init(sem_t *sem, int pshared, unsigned int value);
     * sem: 指向信号量对象的指针，该对象通常是一个 sem_t 类型的变量。
     * pshared: 如果为 0，信号量是线程私有的；如果为非零值，信号量是进程共享的。
     * value: 初始化信号量的初始值，表示可用资源的数量。
     */
    sem_init(&sem, false, 0);
    // 开启线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        // 获取线程tid
        tid_ = CurrentThread::tid();
        // v操作 用于增加信号量的计数
        sem_post(&sem);
        // 开启一个新线程专门执行该线程函数
        func_();
    }));

    /**
     * 这里必须等待获取上面新创建的线程的tid
     * 未获取到信息则不会执行sem_post，所以会被阻塞住
     * 如果不使用信号量操作，则别的线程访问tid时候，可能上面的线程还没有获取到tid
     */
    sem_wait(&sem);
}

void Thread::join()
{
    joined_ = true;
    // 等待线程执行完毕
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d", num);
        name_ = buf;
    }
}
