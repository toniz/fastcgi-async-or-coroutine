#include<iostream>
#include<sstream>
#include "fastcgi.h"
#include "muduo_net/Endian.h"
#include "muduo_base/Logging.h"

#define FCGI_MAX_LENGTH 0xffff

struct FastCgiCodec::RecordHeader
{
    uint8_t version;
    uint8_t type;
    uint16_t id;
    uint16_t length;
    uint8_t padding;
    uint8_t unused;
};

const unsigned FastCgiCodec::kRecordHeader = static_cast<unsigned>(sizeof(FastCgiCodec::RecordHeader));

enum FcgiType
{
    kFcgiInvalid = 0,
    kFcgiBeginRequest = 1,
    kFcgiAbortRequest = 2,
    kFcgiEndRequest = 3,
    kFcgiParams = 4,
    kFcgiStdin = 5,
    kFcgiStdout = 6,
    kFcgiStderr = 7,
    kFcgiData = 8,
    kFcgiGetValues = 9,
    kFcgiGetValuesResult = 10,
};

enum FcgiRole
{
    // kFcgiInvalid = 0,
    kFcgiResponder = 1,
    kFcgiAuthorizer = 2,
};

enum FcgiConstant
{
    kFcgiKeepConn = 1,
};

using namespace muduo::net;

bool FastCgiCodec::onParams(const char* content, uint16_t length)
{
    if (length > 0)
    {
        paramsStream_.append(content, length);
    }
    else if (!parseAllParams())
    {
        LOG_ERROR << "parseAllParams() failed";
        return false;
    }
    return true;
}

void FastCgiCodec::onStdin(const char* content, uint16_t length)
{
    if (length > 0)
    {
        stdin_.append(content, length);
    }
    else
    {
        gotRequest_ = true;
    }
}

bool FastCgiCodec::parseAllParams()
{
    while (paramsStream_.readableBytes() > 0)
    {
        uint32_t nameLen = readLen();
        if (nameLen == static_cast<uint32_t>(-1))
            return false;
        uint32_t valueLen = readLen();
        if (valueLen == static_cast<uint32_t>(-1))
            return false;
        if (paramsStream_.readableBytes() >= nameLen+valueLen)
        {
            string name = paramsStream_.retrieveAsString(nameLen);
            params_[name] = paramsStream_.retrieveAsString(valueLen);
        }
        else
        {
            return false;
        }
    }
    return true;
}

uint32_t FastCgiCodec::readLen()
{
    if (paramsStream_.readableBytes() >= 1)
    {
        uint8_t byte = paramsStream_.peekInt8();
        if (byte & 0x80)
        {
            if (paramsStream_.readableBytes() >= sizeof(uint32_t))
            {
                return paramsStream_.readInt32() & 0x7fffffff;
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return paramsStream_.readInt8();
        }
    }
    else
    {
        return -1;
    }
}

using muduo::net::Buffer;

void FastCgiCodec::endStdout(Buffer* buf)
{
    RecordHeader header =
    {
        1,
        kFcgiStdout,
        sockets::hostToNetwork16(1),
        0,
        0,
        0,
    };
    buf->append(&header, kRecordHeader);
}

void FastCgiCodec::endRequest(Buffer* buf)
{
    RecordHeader header =
    {
        1,
        kFcgiEndRequest,
        sockets::hostToNetwork16(1),
        sockets::hostToNetwork16(kRecordHeader),
        0,
        0,
    };
    buf->append(&header, kRecordHeader);
    buf->appendInt32(0);
    buf->appendInt32(0);
}

void FastCgiCodec::respond(std::string &out, Buffer* response)
{
    if(out.empty())
    {
        std::string content = "Empty Result!";
        std::ostringstream resp;
        resp << "Content-Type: text/html; charset=UTF-8\r\n";
        resp << "Content-Length: " << content.length() << "\r\n\r\n";
        resp << content;
        out = resp.str();
    }

    if (out.length() <= FCGI_MAX_LENGTH)
    {
        RecordHeader header =
        {
            1,
            kFcgiStdout,
            sockets::hostToNetwork16(1),
            sockets::hostToNetwork16(static_cast<uint16_t>(out.length())),
            static_cast<uint8_t>(-out.length() & 7),
            0,
        };
        response->append(&header, kRecordHeader);
        response->append(out);
        response->append("\0\0\0\0\0\0\0\0", header.padding);
    }
    else
    {
        size_t numSubstrings = out.length() / FCGI_MAX_LENGTH;
        for (size_t i = 0; i < numSubstrings; i++)
        {
            std::string outSubString = out.substr(i * FCGI_MAX_LENGTH, FCGI_MAX_LENGTH);
            RecordHeader header =
            {
                1,
                kFcgiStdout,
                sockets::hostToNetwork16(1),
                sockets::hostToNetwork16(static_cast<uint16_t>(FCGI_MAX_LENGTH)),
                static_cast<uint8_t>(-FCGI_MAX_LENGTH & 7),
                0,
            };
            response->append(&header, kRecordHeader);
            response->append(outSubString);
            response->append("\0\0\0\0\0\0\0\0", header.padding);
        }

        if (out.length() % FCGI_MAX_LENGTH != 0)
        {
            std::string outSubString = out.substr(FCGI_MAX_LENGTH * numSubstrings);
            RecordHeader header =
            {
                1,
                kFcgiStdout,
                sockets::hostToNetwork16(1),
                sockets::hostToNetwork16(static_cast<uint16_t>(outSubString.length())),
                static_cast<uint8_t>(-outSubString.length() & 7),
                0,
            };
            response->append(&header, kRecordHeader);
            response->append(outSubString);
            response->append("\0\0\0\0\0\0\0\0", header.padding);
        }
    }

    endStdout(response);
    endRequest(response);
}

bool FastCgiCodec::parseRequest(Buffer* buf)
{
    while (buf->readableBytes() >= kRecordHeader)
    {
        RecordHeader header;
        memcpy(&header, buf->peek(), kRecordHeader);
        header.id = sockets::networkToHost16(header.id);
        header.length = sockets::networkToHost16(header.length);
        size_t total = kRecordHeader + header.length + header.padding;
        if (buf->readableBytes() >= total)
        {
            switch (header.type)
            {
            case kFcgiBeginRequest:
                onBeginRequest(header, buf);
                break;
            case kFcgiParams:
                onParams(buf->peek() + kRecordHeader, header.length);
                break;
            case kFcgiStdin:
                onStdin(buf->peek() + kRecordHeader, header.length);
                break;
            case kFcgiData:
                break;
            case kFcgiGetValues:
                break;
            default:
                break;
            }
            buf->retrieve(total);
        }
        else
        {
            break;
        }
    }
    return true;
}

uint16_t readInt16(const void* p)
{
    uint16_t be16 = 0;
    ::memcpy(&be16, p, sizeof be16);
    return sockets::networkToHost16(be16);
}

bool FastCgiCodec::onBeginRequest(const RecordHeader& header, const Buffer* buf)
{
    assert(buf->readableBytes() >= header.length);
    assert(header.type == kFcgiBeginRequest);

    if (header.length >= kRecordHeader)
    {
        uint16_t role = readInt16(buf->peek()+kRecordHeader);
        uint8_t flags = buf->peek()[kRecordHeader + sizeof(int16_t)];
        if (role == kFcgiResponder)
        {
            keepConn_ = flags == kFcgiKeepConn;
            return true;
        }
    }
    return false;
}

