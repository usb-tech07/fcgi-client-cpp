/*
 * Copyright (c) 2020-2021 Purple Hyacinth Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission from
 *    the author.
 *
 * 4. Products derived from this software may not be called "Purple Hyacinth"
 *    nor may "Purple Hyacinth" appear in their names without specific prior
 *    written permission from the author.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "FastCGIClient.h"

#include <cassert>
#include <ctime>
#include <sstream>

#include "ILogger.h"

namespace
{
    enum FcgiRole
    {
        FCGI_ROLE_RESPONDER = 1,
        FCGI_ROLE_AUTHORIZER = 2,
        FCGI_ROLE_FILTER = 3,
    };

}

static const int FCGI_VERSION = 1;
static const int FCGI_HEADER_SIZE = 8;

static const std::string VER_TOKEN("version");
static const std::string TYPE_TOKEN("type");
static const std::string REQ_ID_TOKEN("requestId");
static const std::string CONT_LEN_TOKEN("contentLength");
static const std::string PADDING_LEN_TOKEN("paddingLength");

static const char KEEP_ALIVE = 0x01;

static const char FCGI_HDR[] = {
    0x00,
	static_cast<char>(FcgiRole::FCGI_ROLE_RESPONDER),
	KEEP_ALIVE,
	0x00, 0x00, 0x00, 0x00, 0x00
};
static const std::string FCGI_HDR_STR(FCGI_HDR, sizeof(FCGI_HDR));
static const std::string EMPTY_MARK;

template<typename Protocol>
const std::chrono::seconds FastCgiClient<Protocol>::DEFAULT_WAIT(300);

template<typename Protocol>
FastCgiClient<Protocol>::FastCgiClient(typename Protocol::endpoint const& endpoint)
    : m_endpoint(endpoint)
    , m_guard(m_ioCtx.get_executor())
    , m_reader(m_ioCtx)
{
    std::srand(std::time(nullptr));
    m_worker = std::thread(std::bind(&FastCgiClient::run, this));
}

template<typename Protocol>
FastCgiClient<Protocol>::~FastCgiClient()
{
	m_guard.reset();
	m_ioCtx.stop();

	if (m_worker.joinable())
	{
        m_worker.join();
	}
}

template<typename Protocol>
bool FastCgiClient<Protocol>::openConnection()
{
	std::lock_guard<std::mutex> lock(m_sync);

	if (m_reader.isOpen())
	{
		INFO("stream reader already opened.");
		return true;
	}

    if (!m_reader.open(m_endpoint))
    {
        WARN("open stream reader failed.");
        return false;
    }

    return true;
}

template<typename Protocol>
bool FastCgiClient<Protocol>::sendRequest(
    KeyValuePairs const& pairs,
    std::string const& body,
    std::string& response,
    std::chrono::seconds const& timeout
)
{
	std::lock_guard<std::mutex> lock(m_sync);

    if (!m_reader.isOpen())
    {
        WARN("stream reader not opened yet.");
        return false;
    }

    const uint16_t requestId = (std::rand() & 0x7fff);
    auto request = encodeFastCgiRecord(FCGI_TYPE_BEGIN, FCGI_HDR_STR, requestId);

    if (!pairs.empty())
    {
        std::string paramsRecord;

        std::for_each(pairs.begin(), pairs.end(), [&, this] (auto& pair) {
            paramsRecord.append(encodeNameValueParams(pair.first, pair.second));
        });

        if (!paramsRecord.empty())
        {
            request.append(encodeFastCgiRecord(FCGI_TYPE_PARAMS, paramsRecord, requestId));
        }
    }

    // mark the end of params
    request.append(encodeFastCgiRecord(FCGI_TYPE_PARAMS, EMPTY_MARK, requestId));

    if (!body.empty())
    {
        request.append(encodeFastCgiRecord(FCGI_TYPE_STDIN, body, requestId));
    }

    // mark the end of content
    request.append(encodeFastCgiRecord(FCGI_TYPE_STDIN, EMPTY_MARK, requestId));

    if (m_reader.write(request) != ReturnCode::OK)
    {
        WARN("write error");
        return false;
    }

    return waitForResponse(requestId, response, timeout);
}

template<typename Protocol>
void FastCgiClient<Protocol>::closeConnection()
{
	std::lock_guard<std::mutex> lock(m_sync);
	m_reader.close();
}

template<typename Protocol>
std::string FastCgiClient<Protocol>::encodeFastCgiRecord(
    FcgiRecordType recType,
    std::string const& content,
    uint16_t requestId
)
{
    std::string rtnRecord;

    const auto contentLen = content.length();
    std::ostringstream ostr;
    ostr << static_cast<char>(FCGI_VERSION)
         << static_cast<char>(recType)
         << static_cast<char>((requestId >> 8) & 0xFF)
         << static_cast<char>(requestId & 0xFF)
         << static_cast<char>((contentLen >> 8) & 0xFF)
         << static_cast<char>(contentLen & 0xFF)
         << static_cast<char>(0)
         << static_cast<char>(0)
         << content;

    rtnRecord = ostr.str();
    return rtnRecord;
}

template<typename Protocol>
std::string FastCgiClient<Protocol>::encodeNameValueParams(
    std::string const& name,
    std::string const& value
)
{
    std::string rtnRecord;

    if (!name.empty() && !value.empty())
    {
        const auto nameLen = name.length();
        const auto valueLen = value.length();
        std::ostringstream ostr;

        if (nameLen < 128)
        {
            ostr << static_cast<char>(nameLen);
        }
        else
        {
            ostr << static_cast<char>((nameLen >> 24) | 0x80)
                 << static_cast<char>((nameLen >> 16) & 0xFF)
                 << static_cast<char>((nameLen >> 8) & 0xFF)
                 << static_cast<char>(nameLen & 0xFF);
        }

        if (valueLen < 128)
        {
            ostr << static_cast<char>(valueLen);
        }
        else
        {
            ostr << static_cast<char>((valueLen >> 24) | 0x80)
                 << static_cast<char>((valueLen >> 16) & 0xFF)
                 << static_cast<char>((valueLen >> 8) & 0xFF)
                 << static_cast<char>(valueLen & 0xFF);
        }
        ostr << name << value;
        rtnRecord = ostr.str();
    }
    return rtnRecord;
}

template<typename Protocol>
bool FastCgiClient<Protocol>::decodeFastCgiHeader(
    std::string const& buf,
    NameTagPairs& pairs
)
{
    pairs.clear();
    pairs.insert({ VER_TOKEN,  buf[0] });
    pairs.insert({ TYPE_TOKEN, buf[1] });
    const uint16_t requestId = ((buf[2] & 0xFF) << 8) + (buf[3] & 0xFF);
    pairs.insert({ REQ_ID_TOKEN, requestId });
    const uint16_t len = ((buf[4] & 0xFF) << 8) + (buf[5] & 0xFF);
    pairs.insert({ CONT_LEN_TOKEN, len });
    pairs.insert({ PADDING_LEN_TOKEN, buf[6] });
    return true; 
}

template<typename Protocol>
bool FastCgiClient<Protocol>::decodeFastCgiRecord(
    FcgiRecordType& type,
    uint16_t& requestId,
    std::string& content
)
{
    static const auto ns = std::chrono::seconds(4);
    char hdr[FCGI_HEADER_SIZE] = { 0 };

    if (m_reader.read(hdr, sizeof(hdr), ns) != ReturnCode::OK)
    {
        WARN("read fcgi header error");
        return false;
    }

    NameTagPairs hdrPairs;
    decodeFastCgiHeader(std::string(hdr, sizeof(hdr)), hdrPairs);
    requestId = hdrPairs[REQ_ID_TOKEN];
    type = static_cast<FcgiRecordType>(hdrPairs[TYPE_TOKEN]);
    const auto contentLen = hdrPairs[CONT_LEN_TOKEN];
    const auto paddingLen = hdrPairs[PADDING_LEN_TOKEN];

    if (contentLen > 0)
    {
        std::unique_ptr<char[]> pContentBuf(new char[contentLen]);

        if (m_reader.read(
                pContentBuf.get(), contentLen, ns
            ) != ReturnCode::OK)
        {
            WARN("read content error");
            return false;
        }

        content.assign(pContentBuf.get(), contentLen);
    }

    if (paddingLen > 0)
    {
        std::unique_ptr<char[]> pPadding(new char[paddingLen]);

        if (m_reader.read(
                pPadding.get(), paddingLen, ns
            ) != ReturnCode::OK)
        {
            WARN("read padding error");
            return false;
        }
    }

    return true;
}

template<typename Protocol>
bool FastCgiClient<Protocol>::waitForResponse(
    uint16_t requestId,
    std::string& response,
    std::chrono::seconds const& timeout
)
{
    FcgiRecordType type;
    uint16_t rcvdReqId;
    std::string content;
    bool ret(false);
    const auto expire = std::chrono::steady_clock::now() + timeout;

    while (true)
    {
        if (!decodeFastCgiRecord(type, rcvdReqId, content))
        {
            WARN("recv fcgi record failed");
        }
        else
        {
            if ((type == FCGI_TYPE_STDOUT) || (type == FCGI_TYPE_STDERR))
            {
                if (rcvdReqId != requestId)
                {
                    WARN("fcgi record request id does not match, expected (=%d).", rcvdReqId);
                }
                else
                {
                    response = content;

                    if (type == FCGI_TYPE_STDOUT)
                    {
                        // received error or response
                        ret = true;
                    }
                }
            }

            if ((type == FCGI_TYPE_END) && (rcvdReqId == requestId))
            {
                break;
            }
        }

        if (std::chrono::steady_clock::now() > expire)
        {
            WARN("request time out.");
            break;
        }
    }

    return ret;
}

template<typename Protocol>
void FastCgiClient<Protocol>::run()
{
	INFO("fcgi-client worker started");
	try
	{
        m_ioCtx.run();
	}
	catch (...)
	{
		// ignore
	}
	INFO("fcgi-client worker stopped");
}
