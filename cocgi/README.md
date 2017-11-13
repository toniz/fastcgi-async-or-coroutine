# coroutine fastcgi using libco

`cocgi` is a coroutine fastcgi using Tencent Libco Library.  
Use Muduo Buffer.cpp for tcp receive buffer.  
Use Some Cgicc Files to parse http request.  
Modify the BackendProc class then You can pass the http request to  back-end service.

---

## INSTALL
```sh
yum install scons -y
scons
```

or run:
```sh 
sh make.sh
```

## Usage
```sh
./cocgi [IP] [PORT] [TASK_COUNT] [PROCESS_COUNT]
./cocgi [IP] [PORT] [TASK_COUNT] [PROCESS_COUNT] -d   # daemonize mode
```

Linux Version > 3.9, support for the SO_REUSEPORT socket option. We can add process on runtime.
```sh
./cocgi 127.0.0.1 6888 30 1 -d
./cocgi 127.0.0.1 6888 30 1 -d 
./cocgi 127.0.0.1 6888 30 1 -d
./cocgi 127.0.0.1 6888 30 1 -d
...
```
> Test on Centos 6/7

