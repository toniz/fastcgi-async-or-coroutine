#ifndef CGI_GLOBAL_TYPE_H_
#define CGI_GLOBAL_TYPE_H_
#include <string>
#include <vector>
#include <map>

typedef std::map<std::string, std::string> SSMap;
typedef std::vector<std::string> SVec;

enum error_code_e
{
    ERR_OK = 0,
    ERR_ICE_EXCEPTION,
    ERR_CALL_PROXY_FAILED,
};


#endif

