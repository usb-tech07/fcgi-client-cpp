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

#ifndef FASTCGICLIENT_H_
#define FASTCGICLIENT_H_

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "asio.hpp"

#include "StreamReader.h"

template<typename Protocol>
class FastCgiClient final
{
    static const std::chrono::seconds DEFAULT_WAIT;

    enum FcgiRecordType
    {
        FCGI_TYPE_BEGIN = 1,
        FCGI_TYPE_ABORT = 2,
        FCGI_TYPE_END = 3,
        FCGI_TYPE_PARAMS = 4,
        FCGI_TYPE_STDIN = 5,
        FCGI_TYPE_STDOUT = 6,
        FCGI_TYPE_STDERR = 7,
        FCGI_TYPE_DATA = 8,
        FCGI_TYPE_GETVALUES = 9,
        FCGI_TYPE_GETVALUES_RESULT = 10,
        FCGI_TYPE_UNKOWNTYPE = 11,
    };

public:

    explicit FastCgiClient(typename Protocol::endpoint const & endpoint);

    ~FastCgiClient();

    bool openConnection();

    bool sendRequest(
        KeyValuePairs const& pairs,
        std::string const& body,
        std::string& response,
        std::chrono::seconds const& timeout = DEFAULT_WAIT);

    void closeConnection();

private:

    FastCgiClient(FastCgiClient const&) = delete;
    FastCgiClient& operator=(FastCgiClient const&) = delete;

    std::string encodeFastCgiRecord(
        FcgiRecordType recType,
        std::string const& content,
        uint16_t requestId
    );

    std::string encodeNameValueParams(
        std::string const& name,
        std::string const& value
    );

    bool decodeFastCgiHeader(std::string const& buf, NameTagPairs& pairs);

    bool decodeFastCgiRecord(
        FcgiRecordType& type,
        uint16_t& requestId,
        std::string& content
    );

    bool waitForResponse(
        uint16_t requestId,
        std::string& response,
        std::chrono::seconds const& timeout
    );

    void run();

    typename Protocol::endpoint m_endpoint;
    asio::io_context m_ioCtx;
    asio::executor_work_guard<asio::io_context::executor_type> m_guard;
    StreamReader<Protocol> m_reader;
    std::thread m_worker;
    std::mutex m_sync;
};

#include "FastCGIClientImpl.h"

#endif /* FASTCGICLIENT_H_ */
