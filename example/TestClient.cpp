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

#include "asio.hpp"

#include "Common.h"
#include "FastCGIClient.h"

#include <iostream>

const KeyValuePairs FCGI_FIXED_HEADERS {
    { "GATEWAY_INTERFACE", "FastCGI/1.0" },
    { "SERVER_SOFTWARE",   "automation" },
    { "REMOTE_ADDR",       "10.17.1.100" },
    { "REMOTE_PORT",       "9501" },
    { "SERVER_ADDR",       "10.17.1.101" },
    { "SERVER_PORT",       "80" },
    { "SERVER_NAME",       "httpd" },
    { "SERVER_PROTOCOL",   "HTTP/1.1" },
    { "CONTENT_TYPE",      "application/xml; charset=utf-8" },
};

int main()
{
    auto params = FCGI_FIXED_HEADERS;
    params.push_back({ "REQUEST_METHOD", "GET" });
    params.push_back({ "REQUEST_URI", "/login" });

    const std::string body(
        R"(<html> Hello </html>)"
    );

    params.push_back({ "CONTENT_LENGTH", std::to_string(body.length()) });

    asio::ip::tcp::endpoint destEndpoint(asio::ip::address::from_string("127.0.0.1"), 9000);
    FastCgiClient<asio::ip::tcp> cli(destEndpoint);

    auto rc = cli.openConnection();

    if (rc)
    {
        std::string resp;
        rc = cli.sendRequest(params, body, resp);
        std::cout << "response received => " << resp << std::endl;
        cli.closeConnection();
    }

    return 0;
}


