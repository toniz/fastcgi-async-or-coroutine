NGINX短连接压力上不去问题分析 
---
# abc
使用火焰图对比短连接和长链接的Cpu使用情况  
__短链接__  
![短链接](/doc/image/fireframe1.png)
__长链接__
![长链接](/doc/image/fireframe2.png)
 
    可以看到_spin_unlock_irqrestore这个自旋锁占用了大部分的系统调用时间。所以可以怀疑是短连接是由于锁操作导致压力上不去。

__在nginx的accept源码里面找到这一段:__  
![code](/doc/image/code.png)

    也就是说，开启ngx_use_accept_mutex的时候，线程之间是互斥的。只有一个线程能够accept连接。由于处理速度慢，
    导致accept队列里面很多已经完成三次握手的链接被直接丢掉（没日志，客户端直接报connect time断开）。
    nginx文档说开启这个变量是为了避免惊群，但事实上，nginx一般只配置Cpu个数X2的进程。
    所以惊群其实影响不大。去掉nginx配置文件里面加上accept_mutex off（默认是on）。  

再次压测，短连接直接占满CPU，性能达到每秒9万次。



