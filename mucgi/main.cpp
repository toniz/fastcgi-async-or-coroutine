#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include "muduo_base/Logging.h"
#include "muduo_net/EventLoop.h"
#include "muduo_net/TcpServer.h"
#include "cgicc_lib/Cgicc.h"
#include "fastcgi.h"
#include "backend.h"

using namespace muduo::net;

void onRequest(const TcpConnectionPtr& conn, FastCgiCodec::ParamMap& params, Buffer* in)
{
    std::string queryString = params["QUERY_STRING"];
    std::string queryType   = params["CONTENT_TYPE"];
    std::string queryCookie = params["HTTP_COOKIE"];
    std::string postData;
    if (in->readableBytes() > 0)
    {
        postData = in->retrieveAllAsString();
    }

    SSMap qmap;
    // Parse Url String
    // Parse Post Data With Standard content type = application/x-www-form-urlencoded 
    cgicc::Cgicc cgi(queryString, queryCookie, postData, queryType);
    cgicc::const_form_iterator iterE;
    for(iterE = cgi.getElements().begin(); iterE != cgi.getElements().end(); ++iterE) 
    {   
        std::string key = boost::to_lower_copy(iterE->getName());
        qmap.insert(SSMap::value_type(key, iterE->getValue()));
        LOG_DEBUG << "Query Element:" << key << " Value:" << iterE->getValue();
    }

    // Parse File Uplod, File upload type = multipart/form-data 
    cgicc::const_file_iterator  iterF;
    for(iterF = cgi.getFiles().begin(); iterF != cgi.getFiles().end(); ++iterF)
    {
        qmap.insert(SSMap::value_type(iterF->getName(), iterF->getData()));
    }

    SSMap header;
    for (FastCgiCodec::ParamMap::const_iterator it = params.begin(); it != params.end(); ++it)
    {
        header[it->first] = it->second;
        LOG_DEBUG << "Query Headers " << it->first << " = " << it->second;
    }

    // Parse Cookie List
    cgicc::const_cookie_iterator iterV;
    for(iterV = cgi.getCookieList().begin(); iterV != cgi.getCookieList().end(); ++iterV)
    {
        header[iterV->getName()] = iterV->getValue();
        LOG_DEBUG << "Cookie Param: Name["<< iterV->getName() << "] Value[" << iterV->getValue() << "].";
    }

    // Call Backend Process
    BackendProc proc;
    std::string res = proc.printRequest(qmap, header);

    LOG_DEBUG << "Query Result: "<< res.length();
    muduo::net::Buffer response;
    FastCgiCodec::respond(res, &response);
    conn->send(&response);
}

void onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        typedef boost::shared_ptr<FastCgiCodec> CodecPtr;
        CodecPtr codec(new FastCgiCodec(onRequest));
        conn->setContext(codec);
        conn->setMessageCallback(boost::bind(&FastCgiCodec::onMessage, codec, _1, _2, _3));
        conn->setTcpNoDelay(true);
    }
}

int main(int argc, char* argv[])
{
    int port = 19981;
    int threads = 0;
    std::string g_log_path;
    if (argc > 1) port     = atoi(argv[1]);
    if (argc > 2) threads  = atoi(argv[2]);

    muduo::Logger::setLogLevel(muduo::Logger::INFO);

    InetAddress addr(static_cast<uint16_t>(port));
    LOG_INFO << "FastCGI listens on " << addr.toIpPort() << " threads " << threads;

    muduo::net::EventLoop loop;
    //TcpServer server(&loop, addr, "FastCGI", muduo::net::TcpServer::kReusePort);
    TcpServer server(&loop, addr, "FastCGI");
    server.setConnectionCallback(onConnection);
    server.setThreadNum(threads);
    server.start();
    loop.loop();
}
