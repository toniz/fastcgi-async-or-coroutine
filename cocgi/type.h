#ifndef COCGI_TYPE_H
#define COCGI_TYPE_H
#include <stdint.h>  
#include <map>
#include <string>

#define hton16(A) ((((uint16_t)(A) & 0xff00) >> 8) | \
                (((uint16_t)(A) & 0x00ff) << 8))
#define hton32(A) ((((uint32_t)(A) & 0xff000000) >> 24) | \
                (((uint32_t)(A) & 0x00ff0000) >> 8)  | \
                (((uint32_t)(A) & 0x0000ff00) << 8)  | \
                (((uint32_t)(A) & 0x000000ff) << 24))
#define hton64(A) (((((uint64_t)A)<<56) & 0xFF00000000000000ULL)  | \
                ((((uint64_t)A)<<40) & 0x00FF000000000000ULL)  | \
                ((((uint64_t)A)<<24) & 0x0000FF0000000000ULL)  | \
                ((((uint64_t)A)<< 8) & 0x000000FF00000000ULL)  | \
                ((((uint64_t)A)>> 8) & 0x00000000FF000000ULL)  | \
                ((((uint64_t)A)>>24) & 0x0000000000FF0000ULL)  | \
                ((((uint64_t)A)>>40) & 0x000000000000FF00ULL)  | \
                ((((uint64_t)A)>>56) & 0x00000000000000FFULL))
#define ntoh16  hton16
#define ntoh32  hton32
#define ntoh64  hton64

typedef std::map<std::string, std::string> ParamMap;

enum error_code_e
{
    ERR_OK = 0,
    ERR_SOCKET_READ,
    ERR_SOCKET_WRITE,
    ERR_SOCKET_FINISH,
};

#endif  // COCGI_TYPE_H
