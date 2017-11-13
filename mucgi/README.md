# coroutine fastcgi using libco

`mucgi` is a async fastcgi using Muduo Network Library.  
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
    nohup ./mcgi [PORT] [THREAD_NUM] &
```

Linux Version > 3.9, support for the SO_REUSEPORT socket option. We can add process on runtime.
```sh
nohup ./mcgi 6888 10 &
nohup ./mcgi 6888 10 & 
nohup ./mcgi 6888 10 &
nohup ./mcgi 6888 10 &
...
```
> Test on Centos 6/7

