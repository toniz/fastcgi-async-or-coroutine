#scons: Reading SConscript files ...
#scons: done reading SConscript files.
#scons: Building targets ...
g++ -o backend.o -c -fPIC -DLINUX -pipe -c -fno-inline -g -fno-strict-aliasing -O2 -export-dynamic -Wall -D_GNU_SOURCE -D_REENTRANT -m64 -Wno-deprecated -D_LINUX_OS_ -I. backend.cpp
g++ -o cgicc_lib/CgiUtils.o -c -fPIC -DLINUX -pipe -c -fno-inline -g -fno-strict-aliasing -O2 -export-dynamic -Wall -D_GNU_SOURCE -D_REENTRANT -m64 -Wno-deprecated -D_LINUX_OS_ -I. cgicc_lib/CgiUtils.cpp
g++ -o cgicc_lib/Cgicc.o -c -fPIC -DLINUX -pipe -c -fno-inline -g -fno-strict-aliasing -O2 -export-dynamic -Wall -D_GNU_SOURCE -D_REENTRANT -m64 -Wno-deprecated -D_LINUX_OS_ -I. cgicc_lib/Cgicc.cpp
g++ -o cgicc_lib/FormEntry.o -c -fPIC -DLINUX -pipe -c -fno-inline -g -fno-strict-aliasing -O2 -export-dynamic -Wall -D_GNU_SOURCE -D_REENTRANT -m64 -Wno-deprecated -D_LINUX_OS_ -I. cgicc_lib/FormEntry.cpp
g++ -o cgicc_lib/FormFile.o -c -fPIC -DLINUX -pipe -c -fno-inline -g -fno-strict-aliasing -O2 -export-dynamic -Wall -D_GNU_SOURCE -D_REENTRANT -m64 -Wno-deprecated -D_LINUX_OS_ -I. cgicc_lib/FormFile.cpp
g++ -o cgicc_lib/HTTPCookie.o -c -fPIC -DLINUX -pipe -c -fno-inline -g -fno-strict-aliasing -O2 -export-dynamic -Wall -D_GNU_SOURCE -D_REENTRANT -m64 -Wno-deprecated -D_LINUX_OS_ -I. cgicc_lib/HTTPCookie.cpp
g++ -o cgicc_lib/MStreamable.o -c -fPIC -DLINUX -pipe -c -fno-inline -g -fno-strict-aliasing -O2 -export-dynamic -Wall -D_GNU_SOURCE -D_REENTRANT -m64 -Wno-deprecated -D_LINUX_OS_ -I. cgicc_lib/MStreamable.cpp
g++ -o libco/co_epoll.o -c -fPIC -DLINUX -pipe -c -fno-inline -g -fno-strict-aliasing -O2 -export-dynamic -Wall -D_GNU_SOURCE -D_REENTRANT -m64 -Wno-deprecated -D_LINUX_OS_ -I. libco/co_epoll.cpp
g++ -o libco/co_routine.o -c -fPIC -DLINUX -pipe -c -fno-inline -g -fno-strict-aliasing -O2 -export-dynamic -Wall -D_GNU_SOURCE -D_REENTRANT -m64 -Wno-deprecated -D_LINUX_OS_ -I. libco/co_routine.cpp
g++ -o libco/co_hook_sys_call.o -c -fPIC -DLINUX -pipe -c -fno-inline -g -fno-strict-aliasing -O2 -export-dynamic -Wall -D_GNU_SOURCE -D_REENTRANT -m64 -Wno-deprecated -D_LINUX_OS_ -I. libco/co_hook_sys_call.cpp
g++ -o libco/coctx.o -c -fPIC -DLINUX -pipe -c -fno-inline -g -fno-strict-aliasing -O2 -export-dynamic -Wall -D_GNU_SOURCE -D_REENTRANT -m64 -Wno-deprecated -D_LINUX_OS_ -I. libco/coctx.cpp
gcc -c -o libco/coctx_swap.o libco/coctx_swap.S
g++ -o muduo_lib/Buffer.o -c -fPIC -DLINUX -pipe -c -fno-inline -g -fno-strict-aliasing -O2 -export-dynamic -Wall -D_GNU_SOURCE -D_REENTRANT -m64 -Wno-deprecated -D_LINUX_OS_ -I. muduo_lib/Buffer.cpp
g++ -o fastcgi.o -c -fPIC -DLINUX -pipe -c -fno-inline -g -fno-strict-aliasing -O2 -export-dynamic -Wall -D_GNU_SOURCE -D_REENTRANT -m64 -Wno-deprecated -D_LINUX_OS_ -I. fastcgi.cpp
g++ -o main.o -c -fPIC -DLINUX -pipe -c -fno-inline -g -fno-strict-aliasing -O2 -export-dynamic -Wall -D_GNU_SOURCE -D_REENTRANT -m64 -Wno-deprecated -D_LINUX_OS_ -I. main.cpp
g++ -o cocgi -Wl,-rpath=/usr/lib64/ -Wl,-rpath=/usr/lib -Wl,-rpath=/usr/local/lib libco/co_epoll.o libco/co_routine.o libco/co_hook_sys_call.o libco/coctx.o libco/coctx_swap.o cgicc_lib/CgiUtils.o cgicc_lib/FormEntry.o cgicc_lib/FormFile.o cgicc_lib/HTTPCookie.o cgicc_lib/MStreamable.o cgicc_lib/Cgicc.o muduo_lib/Buffer.o backend.o fastcgi.o main.o -L. -L/usr/lib -L/usr/lib64 -L/usr/local/lib -lpthread -ldl
#scons: done building targets.
