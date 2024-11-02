# myMuduoWeb
这个是我复写了陈硕的muduo库，并且借助了一个webserver项目的前端代码，实现了一个小型的http服务器，支持get请求和静态资源访问。
这里的各个模块介绍如果以后有机会会一点一点的加到里边。

现在只是提供了两个模块的测试代码，主要是没时间搞了。
如果想要运行的话，记得把数据库连接的那个位置换成你自己的，要不然注册和登录的时候会失败。
之后直接./build.sh即可。



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
这个记得缩小以下页面，我前段对图片大小的设置不是很好。
![image](https://github.com/user-attachments/assets/881058a4-1783-4e9d-b6a3-92ce61156d88)

#### 视频界面 

这个只需要换到xxx.mp4一个合适的视频就可以，我用的是我的世界的录屏，打不开，有时间改一下，但是没错误。
