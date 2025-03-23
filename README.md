# myMuduoWeb
这个是我复写了陈硕的muduo库，并且借助了一个webserver项目的前端代码，实现了一个小型的http服务器，支持get请求和静态资源访问。
这里的各个模块介绍如果以后有机会会一点一点的加到里边。

如果想要运行的话，记得把数据库连接的那个位置换成你自己的，要不然注册和登录的时候会失败。




## 以下是我的测试样例
![image](https://github.com/user-attachments/assets/6fdf0ac8-54fb-47e1-a4a5-c52bc1f261af)

接下来终端输入  ./src/Http/test/http_test
![image](https://github.com/user-attachments/assets/50b50b5e-58b9-43d2-950b-5d883e7516e8)
#### 首页
![image](https://github.com/user-attachments/assets/7e768be2-28d9-440a-98bd-f5b9eca16192)

#### 注册界面
![image](https://github.com/user-attachments/assets/787ac33c-c8ae-49fa-9225-aa0039a5afbc)

#### 登陆界面
![image](https://github.com/user-attachments/assets/5b3e0905-d0cb-4848-a87e-774075293f4c)

#### 图片界面
![image](https://github.com/user-attachments/assets/18b04452-9273-4f51-ad97-ab6a2bd6c58f)

#### 视频界面 
这个只需要换到xxx.mp4一个合适的视频就可以，我用的是我的世界的录屏，打不开，有时间改一下，但是没错误。

#### 内存池模块的设计
* 大块内存回收后归还给操作系统
* 小块内存回收后先不归还操作系统
* 同时添加了越界判断，类似于stl库，当访问越界就直接报错，
* 有独占锁，支持多线程同时访问内存池。
![image](https://github.com/user-attachments/assets/e7a0619d-f147-4c36-b0e9-51c46db42a29)

#### 红黑树设置定时器
使用红黑树设计了一个定时器，并添加到了响应里，如果有新连接到达，但是连接之后长时间不与服务器通信，在muduo库中应该没有设置服务器主动关闭连接的，所以我只要服务器与某个客户端通信（主动 or 被动），都会重新更新定时器里边的时间，然后在指定的时间进行服务端主动断开连接. 当然这个定时任务也可以用到其他地方。
![image](https://github.com/user-attachments/assets/e86a90be-8ead-4434-8fbc-4a5b438191ae)


#### 用webbench进行的压力测试界面
  结果跟设置的subLoop数量有很大关系，我这个是随便设置了一个。
![image](https://github.com/user-attachments/assets/c5f18ed0-2836-4654-ae73-d86d46142aec)

#### 日志打印
  这个最后的FATAL等级我实现一直报错，需要再考虑下问题出在什么地方了。
![image](https://github.com/user-attachments/assets/3263625a-27c2-4849-bc67-4c1b600b1d92)

之后直接./build.sh即可。

