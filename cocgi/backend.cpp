#include "backend.h"
#include <stdio.h> 

std::string BackendProc::printRequest(ParamMap &qmap, ParamMap &header)
{
    for (ParamMap::iterator it = qmap.begin(); it != qmap.end(); it++)
    {
        printf("Param: [%s] ==> [%s] \n", it->first.c_str(), it->second.c_str());
    }

    for (ParamMap::iterator it = header.begin(); it != header.end(); it++)
    {
        printf("Header:[%s] ==> [%s] \n", it->first.c_str(), it->second.c_str());
    }

    std::string content = "OK!!";
    std::string resp = "Content-Type: text/html; charset=UTF-8\r\n";
    resp += "Content-Length: "; 
    resp += content.length();
    resp += "\r\n\r\n";
    resp += content; 

    return resp;
}

