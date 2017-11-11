// Taken from muduo fastcgi exmaple
// https://github.com/chenshuo/muduo
#ifndef FASTCGI_FASTCGI_H
#define FASTCGI_FASTCGI_H

#include "type.h"
#include "muduo_lib/Buffer.h"

// one FastCgiCodec per TcpConnection
// one Tcp Connection per Coroutine
// there is no concurrent requests of one connection.
class FastCgiCodec
{
  public:
    FastCgiCodec()
    {
        m_gotRequest = false;
        m_keepConn = false;
    };

    int readData(int &fd);
    int doRequest(int &fd);
    void respond(std::string &out, muduo::net::Buffer* response);

  private:
    struct FcgiRecordHeader;
    bool parseFcgiPackage();
    bool onBeginRequest(const FcgiRecordHeader& header, const muduo::net::Buffer* buf);
    void onStdin(const char* content, uint16_t length);
    bool onParams(const char* content, uint16_t length);
    bool parseAllParams();
    uint32_t readLen();

    static void endStdout(muduo::net::Buffer* buf);
    static void endRequest(muduo::net::Buffer* buf);

    bool m_gotRequest;
    bool m_keepConn;
    muduo::net::Buffer m_buf;
    muduo::net::Buffer m_stdin;
    muduo::net::Buffer m_paramsStream;
    ParamMap m_params;

    const static unsigned kRecordHeader; 
};

#endif  // FASTCGI_FASTCGI_H
