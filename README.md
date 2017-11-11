# async-or-coroutine-fastcgi

`mucgi` is a async fastcgi using Muduo Network Library.
`cocgi` is a coroutine fastcgi using Tencent Libco Library.
Modify the BackendProc class then You can pass the http request to  back-end service.

`mucgi` and `cocgi` are the original fastcgi optimization model.  
The two models are for different scenarios. 
`mucgi` asynchronous model for the back-end fast response system.  
`cocgi` coroutine model is suitable for a system where the back-end response speed is uneven

___
## nginx -> fastcgi(同步) --> 同步后端(测试用的是ICE)

__这个框架的fastcgi是用[官网](https://fastcgi-archives.github.io/ "悬停显示") 上面的fcgi库编译C++程序，
用cgi-fcgi或者spawn-fcgi指定ip端口调起这个编译好的C++程序。__  
>网上搜到的nginx + fastcgi +c教程基本都是这个模式。


###缺点：
    1.监听模式是 listen -> fork 共享监听端口给所有的进程。每次只有一个进程可以接受新连接。这个模式的问题后面会讨论到。

    2. 同步进程个数，开多了浪费资源，开少了，nginx会报一堆connect refuse.因为如果所有fastcgi的进程都在忙。而且fastcgi的backlog(这个是存放三次握手成功但没access链接)满，那么nginx新建立的链接会直接返回失败。
    
    3. 这里要重点指出，在fastcgi同步模型中，nginx -> fastcgi必须使用短链接。不要在nginx里面配fastcgi_keep_conn on.原因是上面说的，nginx不是所有请求都用已经存在的长链，他会自动去创建新链接。这个时候一个进程一个链接，已经没有办法接受新的链接，所以会出现一堆请求失败。
    
    4. 不要去改fastcgi的listen backlog. 在backlog里面的已经三次握手链接，但没有accept的链接。这些链接会等待，不会立刻返回失败。模型里面设置是5,曾经试过把他改成128，也就是说有128个请求等在那里，导致超时的链接更多了。超时比拒绝链接更伤性能。

####惊群问题
>Linux内核版本2.6.18以前，listen -> fork 这种做法会有惊群效应。在 2.6.18 以后，这个问题得到修复，仅有一个进程被唤醒并 accept 成功。
多IO+多路IO复用的模型也存在惊群在问题(如epoll)。nginx就花了大力气处理这个问题。nginx配置accept_mutex on的时候，用加锁来保证每次只有一个进程的listen_fd会进入epoll。
但在极高的tps下，如用weighttp短链压测nginx echo。每次都只有一个链接能够被接收的模式，这个接受新连接的速度将是个瓶颈，本机测试一秒最多2.2-2.5w次，cpu有一两个核跑满，其它大量idle.
nginx解决这个速度瓶颈问题提供了两个配置项，一个是关掉accept_mutex 虽然会有惊群，但对比开着的性能要提高太多。至少是能跑满cpu了。一个是打开multi_accept on,表示一次可以处理多个accept，能很好的提高accept的速度，但会引起worker负载不均衡。
fastcgi要解决accept性能瓶颈目前没有很好的方案。使用 SO_REUSEPORT(since Linux 3.9)，可以稍微提升下性能。


###改进点：
    高内核版本使用SO_REUSEPORT提升accept性能。需要改libfcgi源码.
    os_unix.c 
    修改代码如下：
    int OS_CreateLocalIpcFd(const char *bindPath, int backlog)
    {
    ...
     328         if(listenSock >= 0) {
     329             int flag = 1;
     330             if(setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR,
     331                           (char *) &flag, sizeof(flag)) < 0) {
     332                 fprintf(stderr, "Can't set SO_REUSEADDR.\n");
     333             exit(1001);
     334         }
             // 增加下面几行
     335         {
     336             int flag = 1;
     337             setsockopt(listenSock, SOL_SOCKET, SO_REUSEPORT, (char *) &flag, sizeof(flag));
     338         }
     339     }
     ...
    }
    测试性能大概提升10%-15% 不算很高。不过方便添加fastcgi进程。

  

 ---  
## nginx -> mucgi(异步) --> 同步后端(测试用的是ICE)

__这个fastcgi异步处理框架是在muduo网络库的基础上完善了fastcgi的协议解析并引入cgicc库里面http协议解析的几个代码。实现和同步fastcgi相同的功能。
仅需要修改backend.cpp和backend.h就可以使用
doc文档有同步和异步cgi的性能测试对比。__

###优点：
    1. nginx可以配置异步长链接.提供了性能。
    2. 不会出现同步模型不能应付瞬间暴涨的请求这种情况。瞬间暴涨的请求是指：如10秒没有请求，突然下一秒来了1万个。一般发生在公网网络波动的时候。如果是同步模型，这里就会出现几千条connect refuse。

###缺点：
    由于fastcgi把请求转到后端，后端处理是用同步的，这个时候fastcgi进程其实是等在这里了。也就是说，一个CGI进程或线程能处理的请求数是要看后端响应速度的。所以但后端突然出现延迟很高的时候还是会造成堵车。就像多条道的高速公路，如果路上有的车开得很慢，就容易造成堵车。

###改进：
    muduo网络库是支持 在构造函数传入muduo::net::TcpServer::kReusePort即可
    如下：TcpServer server(&loop, addr, "FastCGI", muduo::net::TcpServer::kReusePort)
    可以在进程很忙的时候，不需重启就能多加几个fastcgi进程进来处理请求。

   ---  
## nginx -> cocgi(协程) --> 同步后端(测试用的是ICE)
协程fastcgi是使用了腾讯开源框架libco，并结合muduo的Buffer来实现的。

###优点：
    这个可以算是第一个(同步)模型的改进版本。每个进程可以创建多个协程，也就是说每个进程通过协程能模拟出多个进程的效果。
    具体可以看: https://github.com/Tencent/libco
    在后端有的请求耗时不均衡的时候，特别是有请求耗时很长的时候。因为协程的数量多，所以阻塞几个对模型的影响几乎没有。不像同步和异步模型，开10个进程又20个慢请求就要影响整个系统。

###缺点：
    协程模型也是同步模型，所以nginx->cocgi不能配置长链接，测试时候发现，就算有2000协程，如果前端请求很多，nginx发起的连接会等待回收。然而如果长链接没释放，新连接有两种选择，一种是等待，可能引发超时(代码里面是这种)。一种是直接close，可以返回请求失败。两种都会影响用户体验。
    每个进程的协程数不宜开太高。开多也会浪费资源。测试时候，每个进程一般开30-50个协程。

###改进点：
    nginx->cocgi可以使用长链接，如果要配好超时和自动回收的机制。还要和请求量和协程做个均衡。


## 总结：
__异步模型和协程模型都是原fastcgi的优化模型。  
两者针对的场景略有不同。可以根据业务情况选择使用。  
异步模型适合后端响应速度快的系统。  
协程模型适合后端响应速度快慢不均的系统。__
