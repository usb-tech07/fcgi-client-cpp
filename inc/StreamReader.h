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

#ifndef INC_STREAMREADER_H_
#define INC_STREAMREADER_H_

#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <string>

#include "asio.hpp"

#include "Common.h"

/*
 * no thread-safe class
 */
template<typename Protocol>
class StreamReader final
{
	static const std::chrono::seconds DEFAULT_WAIT;

public:

	/**
	 * Constructor
	 *
	 * @param ioCtx passed io context reference
	 */
	explicit StreamReader(asio::io_context& ioCtx);

	/**
	 * @brief open the socket for read
	 *
	 * @param dest peer ip address
	 * @param port peer listen port
	 *
	 * @return true if socket opened successfully,
	 * false if failed to open socket.
	 */
	bool open(typename Protocol::endpoint const& endpoint);

	/**
	 * @brief write data to network
	 *
	 * @param data binary data to be written
	 *
	 * @return OK if write data to socket successfully
	 * @return IO_ERROR if read error occurs
	 * @return CLOED if peer socket closed
	 */
	ReturnCode write(std::string const& data);

	/**
	 * @brief read up to len of bytes data into buffer provided by caller
	 *
	 * @param buf read buffer supplied by caller
	 * @param len buffer capacity
	 * @param expire maximum wait period
	 *
	 * @return OK if read data from socket successfully
	 * @return IO_ERROR if read error occurs
	 * @return CLOED if peer socket closed
	 */
	ReturnCode read(
        char* buf,
		size_t len,
	    std::chrono::seconds const& expire = DEFAULT_WAIT
    );

	/**
	 * @brief close socket
	 */
	void close();

	/**
	 * @brief test if stream is open or not
	 */
	bool isOpen() const;

private:

    void readHandler(asio::error_code const& ec, std::size_t bytesXferred);
    void timeoutHandler(asio::error_code const& ec);

	asio::basic_stream_socket<Protocol> m_sock;
	asio::steady_timer m_responseTimer;
	std::unique_ptr<std::promise<ReturnCode>> m_result;
};

#include "StreamReaderImpl.h"

#endif /* INC_STREAMREADER_H_ */
