# Coroutine Fastcgi

`cocgi` is a coroutine fastcgi using Tencent Libco Library.  
Use Muduo Buffer.cpp for tcp receive buffer.  
Use Some Cgicc Files to parse http request.  
Modify the [BackendProc Class](backend.cpp) then You can pass the http request to back-end service.

---

## INSTALL
```sh
yum install scons -y
scons
```

__Or run:__
```sh 
sh make.sh
```

## Usage
```sh
./cocgi [IP] [PORT] [TASK_COUNT] [PROCESS_COUNT]
./cocgi [IP] [PORT] [TASK_COUNT] [PROCESS_COUNT] -d   # daemonize mode
```

__Linux Version > 3.9, support for the SO_REUSEPORT socket option. We can add process on runtime.__
```sh
./cocgi 127.0.0.1 6888 30 1 -d
./cocgi 127.0.0.1 6888 30 1 -d 
./cocgi 127.0.0.1 6888 30 1 -d
./cocgi 127.0.0.1 6888 30 1 -d
...
```

* [nginx configure](/doc/nginx_configure.conf)  
> cocgi is a synchronize model, must use short-link.

* Test   
![测试](/doc/image/last02.png)


> Test on Centos 6/7

