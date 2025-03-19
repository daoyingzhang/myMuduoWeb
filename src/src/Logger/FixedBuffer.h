#pragma once


#include "noncopyable.h"
#include <assert.h>
#include <string.h> // memcpy
#include <strings.h>
#include <string>

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000*1000; 

template <int SIZE> //在声明的时候就不用给出具体的大小，只需要实例化的时候给出就可以
class FixedBuffer : noncopyable
{
public:
    FixedBuffer()
        : cur_(data_)
    {
    }

    void append(const char* buf, size_t len)
    {
        if (static_cast<size_t>(avail()) > len)
        {
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }
    const char* data() const { return data_; }
    int length() const { return static_cast<int>(cur_ - data_); }
    // int length() const { return static_cast<int>(end() - data_); }

    char* current() { return cur_; }
    int avail() const { return static_cast<int>(end() - cur_); }
    void add(size_t len) { cur_ += len; }

    void reset() { cur_ = data_; }
    // void bzero() { ::bzero(data_, SIZE); }
    void bzero() { ::bzero(data_, sizeof(data_)); }

    std::string toString() const { return std::string(data_, length()); }

private:
    const char* end() const { return data_ + sizeof(data_); }
    // const char* end() const { return data_ + SIZE; }

    char data_[SIZE]; //这里不用给出其大小，等到实例化的时候给出就可以
    char* cur_; 
};
