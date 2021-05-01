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

#include "StreamReader.h"

#include "asio/basic_stream_socket.hpp"
#include "ILogger.h"

template<typename Protocol>
const std::chrono::seconds StreamReader<Protocol>::DEFAULT_WAIT(5);

template<typename Protocol>
StreamReader<Protocol>::StreamReader(asio::io_context& ioCtx)
    : m_sock(ioCtx)
    , m_responseTimer(ioCtx)
{
}

template<typename Protocol>
bool StreamReader<Protocol>::open(typename Protocol::endpoint const& endpoint)
{
    asio::error_code ec;
    m_sock.connect(endpoint, ec);

    if (ec)
    {
        WARN(
            "unable to connect to host, code (=%d), error (=%s).",
            ec.value(),
            ec.message().c_str()
        );
        return false;
    }

    return m_sock.is_open();
}

template<typename Protocol>
ReturnCode StreamReader<Protocol>::write(std::string const& data)
{
    if (!m_sock.is_open())
    {
        WARN("unable to write, socket closed.");
        return ReturnCode::CLOSED;
    }

    asio::error_code ec;
    asio::write(m_sock, asio::const_buffer(data.c_str(), data.length()), ec);

    if (ec)
    {
        WARN("write error, code (=%d), error (=%s).", ec.value(), ec.message().c_str());
        return ReturnCode::IO_ERROR;
    }

    return ReturnCode::OK;
}

template<typename Protocol>
ReturnCode StreamReader<Protocol>::read(
    char* buf,
    size_t len,
    std::chrono::seconds const& expire
)
{
    m_result.reset(new std::promise<ReturnCode>());
    auto f = m_result->get_future();
    if (expire.count() != 0)
    {
        // start a timer if caller expects to return until some limit
        m_responseTimer.expires_after(expire);
        m_responseTimer.async_wait(
            std::bind(
                &StreamReader::timeoutHandler, this,
                std::placeholders::_1 // error code
            )
        );
    }
    asio::async_read(
        m_sock,
        asio::buffer(buf, len),
        std::bind(
            &StreamReader::readHandler, this,
            std::placeholders::_1,  // error code
            std::placeholders::_2   // bytes xferred
        )
    );
    auto rc = f.get();
    asio::error_code ec;

    switch (rc)
    {
        case ReturnCode::OK:
            m_responseTimer.cancel(ec);
            if (ec)
            {
                WARN("cancel timer failed, code (=%d), msg (=%s).", ec.value(), ec.message().c_str());
            }
            break;

        case ReturnCode::TIMEOUT:
            m_sock.cancel(ec);
            if (ec)
            {
                WARN("cancel async read failed, code (=%d), msg (=%s).", ec.value(), ec.message().c_str());
            }
            break;

        default:
            break;
    }

    return rc;
}

template<typename Protocol>
void StreamReader<Protocol>::close()
{
    if (!m_sock.is_open())
        return;

    asio::error_code ec;
    m_sock.cancel(ec);
    if (ec)
    {
        WARN("cancel async read failed, code (=%d), msg (=%s).", ec.value(), ec.message().c_str());
    }
    m_sock.close();
}

template<typename Protocol>
bool StreamReader<Protocol>::isOpen() const
{
    return m_sock.is_open();
}

template<typename Protocol>
void StreamReader<Protocol>::readHandler(
    asio::error_code const& ec,
    std::size_t bytesXferred
)
{
    if (ec == asio::error::operation_aborted)
    {
        // read operation was cancelled
        return;
    }

    if (ec)
    {
        DEBUG("async_read error, code (=%d), msg (=%s).", ec.value(), ec.message().c_str());

        // socket closed
        m_result->set_value(
            (ec == asio::error::eof) ? ReturnCode::CLOSED : ReturnCode::IO_ERROR
        );
    }
    else
    {
        // socket closed
        m_result->set_value(ReturnCode::OK);
    }
}

template<typename Protocol>
void StreamReader<Protocol>::timeoutHandler(asio::error_code const& ec)
{
    if (ec == asio::error::operation_aborted)
    {
        // read operation was cancelled
        return;
    }

    m_result->set_value(ReturnCode::TIMEOUT);
}
