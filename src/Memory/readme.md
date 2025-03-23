# 内存池模块
* 目前这个内存池模块还没有被调用。


### 一些用法示例
#### 这个是沿用的Nginx的内存池模式
* 加上了内存的越界访问，当数据越界的时候会进行数据报错，这个类似vector，当访问到没有的数据的时候，会提示越界并结束程序

```
void test5(){
    Memory mem;
    MemoryView<int> arr = mem.allocate<int>(10);
    arr[1]=10;
    arr[2]=11;
    cout<<arr[1]<<arr[2]<<endl;
    mem.deallocate(arr);
    std::cout<<"删除了分配的空间"<<endl;
    MemoryView<int> arr1 = mem.allocate<int>(2);
    arr1[1]=10;
    cout<<arr1[1]<<endl;
    arr1[11]=10;
    mem.deallocate(arr1);
}
int main()
{
    // test1();
    // cout<<"###################################"<<endl;
    // test2();
    // test3();
    // test4();
    test5();
    return 0;
}


result：

1011
删除了分配的空间
10
Index out of bounds!
Aborted (core dumped)
```
