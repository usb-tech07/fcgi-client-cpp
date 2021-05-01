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

#ifndef INC_ILOGGER_H_
#define INC_ILOGGER_H_

#include <string>

/*
 * simple logger interface adapter
 */
class ILogger
{
public:

    enum class Level
    {
        INFO_LEVEL,
        DEBUG_LEVEL,
        WARN_LEVEL,
        ERROR_LEVEL,
        FATAL_LEVEL
    };

    virtual ~ILogger() = default;

    virtual void setLogLevel(Level logLevel) = 0;

    virtual void info(const char* const fileName, unsigned long line, const char* const fmt...) = 0;

    virtual void debug(const char* const fileName, unsigned long line, const char* const fmt...) = 0;

    virtual void warn(const char* const fileName, unsigned long line, const char* const fmt...) = 0;

    virtual void error(const char* const fileName, unsigned long line, const char* const fmt...) = 0;

    virtual void fatal(const char* const fileName, unsigned long line, const char* const fmt...) = 0;

    static ILogger* getLogger();

    static void emplaceLogger(ILogger* logger);
};

#define INFO(fmt, ...)                                                      \
    do {                                                                    \
        ILogger::getLogger()->info(__FILE__, __LINE__, fmt, ##__VA_ARGS__); \
    } while (0)

#define DEBUG(fmt, ...)                                                      \
    do {                                                                     \
        ILogger::getLogger()->debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__); \
    } while (0)

#define WARN(fmt, ...)                                                      \
    do {                                                                     \
        ILogger::getLogger()->warn(__FILE__, __LINE__, fmt, ##__VA_ARGS__); \
    } while (0)

#define ERROR(fmt, ...)                                                      \
    do {                                                                     \
        ILogger::getLogger()->error(__FILE__, __LINE__, fmt, ##__VA_ARGS__); \
    } while (0)

#define FATAL(fmt, ...)                                                      \
    do {                                                                     \
        ILogger::getLogger()->fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__); \
    } while (0)

#endif /* INC_ILOGGER_H_ */
