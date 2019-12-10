#include <iostream>
#include <sstream>
#include <errno.h> 
#include <unistd.h>
#include "fastcgi.h"
#include "cgicc_lib/Cgicc.h"
#include "backend.h"

/* fastcgi protocol records struct
 * From : http://www.mit.edu/~yandros/doc/specs/fcgi-spec.html
 * typedef struct {
 * unsigned char version;
 * unsigned char type;
 * unsigned char requestIdB1;
 * unsigned char requestIdB0;
 * unsigned char contentLengthB1;
 * unsigned char contentLengthB0;
 * unsigned char paddingLength;
 * unsigned char reserved;
 * unsigned char contentData[contentLength];
 * unsigned char paddingData[paddingLength];
 * } FCGI_Record;
 * version: Identifies the FastCGI protocol version. This specification documents FCGI_VERSION_1.
 * type: Identifies the FastCGI record type, 
 * requestId: Identifies the FastCGI request to which the record belongs.
 * contentLength: The number of bytes in the contentData component of the record.
 * paddingLength: The number of bytes in the paddingData component of the record.
 * contentData: Between 0 and 65535 bytes of data, interpreted according to the record type.
 * paddingData: Between 0 and 255 bytes of data, which are ignored.
 */
#define FCGI_MAX_CONTENT_LENGTH 0xffff
#define FCGI_MAX_PADDING_LENGTH 0xff

struct FastCgiCodec::FcgiRecordHeader 
{
    uint8_t version;
    uint8_t type;
    uint16_t id;
    uint16_t length;
    uint8_t padding;
    uint8_t unused;
};

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

const unsigned FastCgiCodec::kRecordHeader = static_cast<unsigned>(sizeof(FastCgiCodec::FcgiRecordHeader));

uint16_t readInt16(const void* p)
{
    uint16_t be16 = 0;
    ::memcpy(&be16, p, sizeof be16);
    return ntoh16(be16);
}

int FastCgiCodec::readData(int &fd)
{
    int errno;
    int ret = m_buf.readFd(fd, &errno);
    if( ret > 0 ) 
    {
        parseFcgiPackage();
        if (m_gotRequest == true)
        {
            ret = doRequest(fd);
            m_stdin.retrieveAll();
            m_paramsStream.retrieveAll();
            m_params.clear();
            m_gotRequest = false;
            if (!m_keepConn && ret == ERR_OK)
            {
                return ERR_SOCKET_FINISH;
            }
            return ret;
        }
    }
    else
    {
        // accept_routine->SetNonBlock(fd) cause EAGAIN, we should continue
        if (errno == EAGAIN)
        {
            return ERR_SOCKET_EAGAIN;
        }
    }

    return ERR_SOCKET_READ;
}

bool FastCgiCodec::parseFcgiPackage()
{
    while (m_buf.readableBytes() >= kRecordHeader)
    {
        FcgiRecordHeader header;
        memcpy(&header, m_buf.peek(), kRecordHeader);
        header.id = ntoh16(header.id);
        header.length = ntoh16(header.length);
        size_t total = kRecordHeader + header.length + header.padding;
        if (m_buf.readableBytes() >= total)
        {
            switch (header.type)
            {
            case kFcgiBeginRequest:
                onBeginRequest(header, &m_buf);
                break;
            case kFcgiParams:
                onParams(m_buf.peek() + kRecordHeader, header.length);
                break;
            case kFcgiStdin:
                onStdin(m_buf.peek() + kRecordHeader, header.length);
                break;
            case kFcgiData:
                break;
            case kFcgiGetValues:
                break;
            default:
                break;
            }
            m_buf.retrieve(total);
        }
        else
        {
            break;
        }
    }
    return true;
}

/* Fastcgi Protocol Name-Value Pairs Struct
 * From : http://www.mit.edu/~yandros/doc/specs/fcgi-spec.html
 * The contentData component of a FCGI_BEGIN_REQUEST record has the form:
 *   typedef struct {
 *       unsigned char roleB1;
 *       unsigned char roleB0;
 *       unsigned char flags;
 *       unsigned char reserved[5];
 *   } FCGI_BeginRequestBody;
 *
 * The role component sets the role the Web server expects the application to play. The currently-defined roles are:
 *   FCGI_RESPONDER
 *   FCGI_AUTHORIZER
 *   FCGI_FILTER
 * The flags component contains a bit that controls connection shutdown:
 *   flags & FCGI_KEEP_CONN: 
 *     If zero, the application closes the connection after responding to this request. 
 *     If not zero, the application does not close the connection after responding to this request; 
 *     the Web server retains responsibility for the connection.
 */
bool FastCgiCodec::onBeginRequest(const FcgiRecordHeader& header, const muduo::net::Buffer* buf)
{
    assert(buf->readableBytes() >= header.length);
    assert(header.type == kFcgiBeginRequest);

    if (header.length >= kRecordHeader)
    {
        uint16_t role = readInt16(buf->peek()+kRecordHeader);
        uint8_t flags = buf->peek()[kRecordHeader + sizeof(int16_t)];
        if (role == kFcgiResponder)
        {
            m_keepConn = flags == kFcgiKeepConn;
            return true;
        }
    }
    return false;
}

bool FastCgiCodec::onParams(const char* content, uint16_t length)
{
    if (length > 0)
    {
        m_paramsStream.append(content, length);
    }
    else if (!parseAllParams())
    {
        return false;
    }
    return true;
}

void FastCgiCodec::onStdin(const char* content, uint16_t length)
{
    if (length > 0)
    {
        m_stdin.append(content, length);
    }
    else
    {
        m_gotRequest = true;
    }
}

/* Fastcgi Protocol Name-Value Pairs Struct
 * From : http://www.mit.edu/~yandros/doc/specs/fcgi-spec.html
 * FastCGI transmits a name-value pair as the length of the name, followed by the length of the value, 
 * followed by the name, followed by the value. 
 * Lengths of 127 bytes and less can be encoded in one byte, while longer lengths are always encoded in four bytes:
 *   typedef struct {
 *       unsigned char nameLengthB0;  // nameLengthB0  >> 7 == 0
 *       unsigned char valueLengthB0; // valueLengthB0 >> 7 == 0
 *       unsigned char nameData[nameLength];
 *       unsigned char valueData[valueLength];
 *   } FCGI_NameValuePair11;
 *   
 *   typedef struct {
 *       unsigned char nameLengthB0;  // nameLengthB0  >> 7 == 0
 *       unsigned char valueLengthB3; // valueLengthB3 >> 7 == 1
 *       unsigned char valueLengthB2;
 *       unsigned char valueLengthB1;
 *       unsigned char valueLengthB0;
 *       unsigned char nameData[nameLength];
 *       unsigned char valueData[valueLength
 *               ((B3 & 0x7f) << 24) + (B2 << 16) + (B1 << 8) + B0];
 *   } FCGI_NameValuePair14;
 *   
 *   typedef struct {
 *       unsigned char nameLengthB3;  // nameLengthB3  >> 7 == 1
 *       unsigned char nameLengthB2;
 *       unsigned char nameLengthB1;
 *       unsigned char nameLengthB0;
 *       unsigned char valueLengthB0; // valueLengthB0 >> 7 == 0
 *       unsigned char nameData[nameLength
 *               ((B3 & 0x7f) << 24) + (B2 << 16) + (B1 << 8) + B0];
 *       unsigned char valueData[valueLength];
 *   } FCGI_NameValuePair41;
 *   
 *   typedef struct {
 *       unsigned char nameLengthB3;  // nameLengthB3  >> 7 == 1
 *       unsigned char nameLengthB2;
 *       unsigned char nameLengthB1;
 *       unsigned char nameLengthB0;
 *       unsigned char valueLengthB3; // valueLengthB3 >> 7 == 1
 *       unsigned char valueLengthB2;
 *       unsigned char valueLengthB1;
 *       unsigned char valueLengthB0;
 *       unsigned char nameData[nameLength
 *               ((B3 & 0x7f) << 24) + (B2 << 16) + (B1 << 8) + B0];
 *       unsigned char valueData[valueLength
 *               ((B3 & 0x7f) << 24) + (B2 << 16) + (B1 << 8) + B0];
 *   } FCGI_NameValuePair44;
 * The high-order bit of the first byte of a length indicates the length's encoding. 
 * high-order zero implies a one-byte encoding, a one a four-byte encoding.
 */
bool FastCgiCodec::parseAllParams()
{
    while (m_paramsStream.readableBytes() > 0)
    {
        uint32_t nameLen = readLen();
        if (nameLen == static_cast<uint32_t>(-1))
            return false;
        uint32_t valueLen = readLen();
        if (valueLen == static_cast<uint32_t>(-1))
            return false;
        if (m_paramsStream.readableBytes() >= nameLen+valueLen)
        {
            std::string name = m_paramsStream.retrieveAsString(nameLen);
            m_params[name] = m_paramsStream.retrieveAsString(valueLen);
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
    if (m_paramsStream.readableBytes() >= 1)
    {
        uint8_t byte = m_paramsStream.peekInt8();
        if (byte & 0x80)
        {
            if (m_paramsStream.readableBytes() >= sizeof(uint32_t))
            {
                return m_paramsStream.readInt32() & 0x7fffffff;
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return m_paramsStream.readInt8();
        }
    }
    else
    {
        return -1;
    }
}

// Last Package Fill 0
void FastCgiCodec::endStdout(muduo::net::Buffer* buf)
{
    FcgiRecordHeader header =
    {
        1,
        kFcgiStdout,
        hton16(1),
        0,
        0,
        0,
    };
    buf->append(&header, kRecordHeader);
}

void FastCgiCodec::endRequest(muduo::net::Buffer* buf)
{
    FcgiRecordHeader header =
    {
        1,
        kFcgiEndRequest,
        hton16(1),
        hton16(kRecordHeader),
        0,
        0,
    };
    buf->append(&header, kRecordHeader);
    buf->appendInt32(0);
    buf->appendInt32(0);
}

void FastCgiCodec::respond(std::string &out, muduo::net::Buffer* response)
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

    if (out.length() <= FCGI_MAX_CONTENT_LENGTH)
    {
        uint16_t slength = static_cast<uint16_t>(out.length());
        slength=hton16(slength);
        FcgiRecordHeader header =
        {
            1,
            kFcgiStdout,
            hton16(1),
            slength,
            static_cast<uint8_t>(-out.length() & 7),
            0,
        };
        response->append(&header, kRecordHeader);
        response->append(out);
        response->append("\0\0\0\0\0\0\0\0", header.padding);
    }
    else
    {
        size_t numSubstrings = out.length() / FCGI_MAX_CONTENT_LENGTH;
        for (size_t i = 0; i < numSubstrings; i++)
        {
            std::string outSubString = out.substr(i * FCGI_MAX_CONTENT_LENGTH, FCGI_MAX_CONTENT_LENGTH);
            FcgiRecordHeader header =
            {
                1,
                kFcgiStdout,
                hton16(1),
                hton16(FCGI_MAX_CONTENT_LENGTH),
                static_cast<uint8_t>(-FCGI_MAX_CONTENT_LENGTH & 7),
                0,
            };
            response->append(&header, kRecordHeader);
            response->append(outSubString);
            response->append("\0\0\0\0\0\0\0\0", header.padding);
        }

        if (out.length() % FCGI_MAX_CONTENT_LENGTH != 0)
        {
            std::string outSubString = out.substr(FCGI_MAX_CONTENT_LENGTH * numSubstrings);
            uint16_t slength = static_cast<uint16_t>(outSubString.length());
            slength=hton16(slength);
            FcgiRecordHeader header =
            {
                1,
                kFcgiStdout,
                hton16(1),
                slength,
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

int FastCgiCodec::doRequest(int &fd)
{
    std::string queryString = m_params["QUERY_STRING"];
    std::string queryType   = m_params["CONTENT_TYPE"];
    std::string queryCookie = m_params["HTTP_COOKIE"];
    std::string postData;
    if (m_stdin.readableBytes() > 0)
    {
        postData = m_stdin.retrieveAllAsString();
    }

    ParamMap qmap;
    cgicc::Cgicc cgi(queryString, queryCookie, postData, queryType);
    cgicc::const_form_iterator iterElements;
    // Parse Url String
    // Parse Post Data With Standard content type = application/x-www-form-urlencoded 
    for(iterElements = cgi.getElements().begin(); iterElements != cgi.getElements().end(); ++iterElements) 
    {   
        qmap[iterElements->getName()] = iterElements->getValue();
    }
    //printf("qmap size [%lu]\n", cgi.getElements().size());

    // Parse File Uplod, File upload type = multipart/form-data 
    cgicc::const_file_iterator  iterFiles;
    for(iterFiles = cgi.getFiles().begin(); iterFiles != cgi.getFiles().end(); ++iterFiles)
    {
        qmap[iterFiles->getName()] = iterFiles->getData();
    }
    //printf("qmap file size [%lu]\n", cgi.getFiles().size());

    ParamMap header = m_params;
    // Parse Cookie List
    cgicc::const_cookie_iterator iterCookieList;
    for(iterCookieList = cgi.getCookieList().begin(); iterCookieList != cgi.getCookieList().end(); ++iterCookieList)
    {
        header[iterCookieList->getName()] = iterCookieList->getValue();
    }
    //printf("head cookie size [%lu]\n", cgi.getCookieList().size());

    // Call Backend Process
    BackendProc proc;
    std::string res = proc.printRequest(qmap, header);

    // Build Response Packages
    muduo::net::Buffer response;
    respond(res, &response);
    while (response.readableBytes() > 0)
    {
        int n = write(fd, response.peek(), response.readableBytes());
        if (n > 0)
        {
            response.retrieve(n);
        }

        if (n < 0)
        {
            // accept_routine->SetNonBlock(fd) cause EAGAIN, we should continue
            if (errno == EAGAIN)
            {
                return ERR_SOCKET_EAGAIN;
            }
            return ERR_SOCKET_WRITE;
        }
    }
    return ERR_OK;
}

