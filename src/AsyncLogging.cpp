#include "AsyncLogging.h"
#include "Timestamp.h"

#include <stdio.h>

AsyncLogging::AsyncLogging(const std::string& basename,
                           off_t rollSize,
                           int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(basename),
      rollSize_(rollSize),
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
      mutex_(),
      cond_(),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_()
{
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}

void AsyncLogging::append(const char* logline, int len)
{
    // lock在构造函数中自动绑定它的互斥体并加锁，在析构函数中解锁，大大减少了死锁的风险
    std::lock_guard<std::mutex> lock(mutex_);
    // 缓冲区剩余空间足够则直接写入
    if (currentBuffer_->avail() > len)
    {
        currentBuffer_->append(logline, len);
    }
    else
    {
        // 当前缓冲区空间不够，将新信息写入备用缓冲区
        buffers_.push_back(std::move(currentBuffer_));
        if (nextBuffer_) 
        {
            currentBuffer_ = std::move(nextBuffer_);
        } 
        else 
        {
            // 备用缓冲区也不够时，重新分配缓冲区，这种情况很少见
            currentBuffer_.reset(new Buffer);
        }
        currentBuffer_->append(logline, len);
        std::cout << "After append, currentBuffer_->length(): " << currentBuffer_->length() << std::endl;
        // 唤醒写入磁盘得后端线程
        cond_.notify_one();
    }
}
void AsyncLogging::threadFunc()
{
    LogFile output(basename_, rollSize_, false);
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    // std::cout<<"newBuffer1->length():"<<newBuffer1->length()<<std::endl;
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);

    while (running_)
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (buffers_.empty())
            {
                cond_.wait_for(lock, std::chrono::seconds(3));
            }
            // std::cout<<"out_currentBuffer_->length():"<< currentBuffer_->length()<<std::endl;
            // 确保只写入有内容的 buffer
            if (currentBuffer_->length() > 0)
            {
                // std::cout<<"currentBuffer_->length():"<< currentBuffer_->length()<<std::endl;
                buffers_.push_back(std::move(currentBuffer_));
            }
            std::cout<<"movecurrentBuffer"<<std::endl;
            currentBuffer_ = std::move(newBuffer1);
            buffersToWrite.swap(buffers_);

            if (!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);
            }
        }
        // std::cout<<"out struct"<<std::endl;
        // **确保只写入有效数据**
        for (const auto& buffer : buffersToWrite)
        {
            output.append(buffer->data(), buffer->length()); 
            // std::cout<<"buffer->length():"<< buffer->length()<<std::endl;
        }
        // std::cout<<"out struct1"<<std::endl;
        // **只保留两个 buffer** 如果不够就进行扩容，为了分配给newBuffer1 和 newBuffer2
        if (buffersToWrite.size() > 2)
        {
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
        }else if(buffersToWrite.size()==1){
            buffersToWrite.emplace_back(new Buffer);
        }else if(buffersToWrite.size()==0){
            buffersToWrite.emplace_back(new Buffer);
            buffersToWrite.emplace_back(new Buffer);
        }
        // std::cout<<"out struct2"<<std::endl;
        if (!newBuffer1)
        {
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->bzero();  // **清除残留数据**
        }
        std::cout<<"out struct3"<<std::endl;
        if (!newBuffer2)
        {
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->bzero();  // **清除残留数据**
        }

        buffersToWrite.clear();  // 彻底清除已写入的 buffer
        output.flush();  // **确保数据真正落盘**
    }
}
