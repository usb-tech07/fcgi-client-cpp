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

#include "ILogger.h"

#include <cstdarg>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>

namespace
{

class DefaultLogger : public ILogger
{
public:

    void setLogLevel(Level logLevel) override;

    void info(const char* const fileName, unsigned long line, const char* const fmt...) override;

    void debug(const char* const fileName, unsigned long line, const char* const fmt...) override;

    void warn(const char* const fileName, unsigned long line, const char* const fmt...) override;

    void error(const char* const fileName, unsigned long line, const char* const fmt...) override;

    void fatal(const char* const fileName, unsigned long line, const char* const fmt...) override;

private:

    bool isLoggable(Level level);

    void format(
        Level logLevel,
        const char* const fileName,
        unsigned long line
    );

    void log(
        Level logLevel,
        const char* const fileName,
        unsigned long line,
        const char* const fmt,
        va_list& arg
    );

    std::mutex m_sync;
    ILogger::Level m_level = Level::INFO_LEVEL;
};

static std::map<ILogger::Level, const std::string> LEVEL_STR_MAP {
    { ILogger::Level::INFO_LEVEL,  "info"  },
    { ILogger::Level::DEBUG_LEVEL, "debug" },
    { ILogger::Level::WARN_LEVEL,  "warn"  },
    { ILogger::Level::ERROR_LEVEL, "error" },
    { ILogger::Level::FATAL_LEVEL, "fatal" },
};

static const size_t MAX_TIME_BUF_SIZE = 20;
static const int MAX_LINE_SZ = 200;

void DefaultLogger::setLogLevel(Level logLevel)
{
    std::lock_guard<std::mutex> lock(m_sync);
    m_level = logLevel;
}

void DefaultLogger::info(const char* const fileName, unsigned long line, const char* const fmt...)
{
    std::lock_guard<std::mutex> lock(m_sync);

    if (isLoggable(Level::INFO_LEVEL))
    {
        va_list args;
        va_start(args, fmt);
        log(Level::INFO_LEVEL, fileName, line, fmt, args);
        va_end(args);
    }
}

void DefaultLogger::debug(const char* const fileName, unsigned long line, const char* const fmt...)
{
    std::lock_guard<std::mutex> lock(m_sync);

    if (isLoggable(Level::DEBUG_LEVEL))
    {
        va_list args;
        va_start(args, fmt);
        log(Level::DEBUG_LEVEL, fileName, line, fmt, args);
        va_end(args);
    }
}

void DefaultLogger::warn(const char* const fileName, unsigned long line, const char* const fmt...)
{
    std::lock_guard<std::mutex> lock(m_sync);

    if (isLoggable(Level::WARN_LEVEL))
    {
        va_list args;
        va_start(args, fmt);
        log(Level::WARN_LEVEL, fileName, line, fmt, args);
        va_end(args);
    }
}

void DefaultLogger::error(const char* const fileName, unsigned long line, const char* const fmt...)
{
    std::lock_guard<std::mutex> lock(m_sync);

    if (isLoggable(Level::ERROR_LEVEL))
    {
        va_list args;
        va_start(args, fmt);
        log(Level::ERROR_LEVEL, fileName, line, fmt, args);
        va_end(args);
    }
}

void DefaultLogger::fatal(const char* const fileName, unsigned long line, const char* const fmt...)
{
    std::lock_guard<std::mutex> lock(m_sync);

    if (isLoggable(Level::FATAL_LEVEL))
    {
        va_list args;
        va_start(args, fmt);
        log(Level::FATAL_LEVEL, fileName, line, fmt, args);
        va_end(args);
    }
}

bool DefaultLogger::isLoggable(Level logLevel)
{
    switch (logLevel)
    {
        case Level::INFO_LEVEL:
        {
            // INFO LEVEL log everything
            return (logLevel != Level::DEBUG_LEVEL);
        }

        case Level::DEBUG_LEVEL:
        {
            return (logLevel != Level::INFO_LEVEL);
        }

        case Level::WARN_LEVEL:
        {
            return (logLevel != Level::INFO_LEVEL) && (logLevel != Level::DEBUG_LEVEL);
        }

        case Level::ERROR_LEVEL:
        {
            return (logLevel == Level::ERROR_LEVEL) || (logLevel == Level::FATAL_LEVEL);
        }

        case Level::FATAL_LEVEL:
        {
            return (logLevel == Level::FATAL_LEVEL);
        }
    }

    return false;

}

void DefaultLogger::format(
    Level logLevel,
    const char* const fileName,
    unsigned long line
)
{
    static char tbuf[MAX_TIME_BUF_SIZE] = { 0 };
    auto now = time(NULL);
    auto nowTm = localtime(&now);
    auto pNameNoPath = std::strrchr(fileName, '/');
    std::strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", nowTm);
    std::cout << tbuf << "[ " << LEVEL_STR_MAP[logLevel] << " ]  [ "
        << (pNameNoPath == nullptr ? fileName : pNameNoPath + 1)
        << ":" << line << " ] ";
}

void DefaultLogger::log(
    Level logLevel,
    const char* const fileName,
    unsigned long line,
    const char* const fmt,
    va_list& arg
)
{
    static char lbuf[MAX_LINE_SZ] = { 0 };
    format(logLevel, fileName, line);
    va_list argCopy;
    va_copy(argCopy, arg);

    // get printed log statement length with first try on lbuf,
    // which is 200 byte-long.
    auto written = std::vsnprintf(lbuf, sizeof(lbuf), fmt, arg);

    if (written < 0)
    {
        // out of memory
        return;
    }

    if (written <= MAX_LINE_SZ)
    {
        // lbuf has complete formatted logging
        std::cout << lbuf;
    }
    else
    {
        // lbuf has partial formatted logging
        std::unique_ptr<char[]> pBuf(new char[written]);

        written = std::vsnprintf(pBuf.get(), written, fmt, argCopy);

        if (written > 0)
        {
            std::cout << pBuf.get();
        }
    }
    std::cout << std::endl;
}

} // sealed namespace

static std::mutex gMutex;
static std::unique_ptr<ILogger> gLogger;

ILogger* ILogger::getLogger()
{
    if (!gLogger)
    {
        std::lock_guard<std::mutex> lock(gMutex);

        if (!gLogger)
        {
            gLogger.reset(new DefaultLogger());
        }
    }
    return gLogger.get();
}

void ILogger::emplaceLogger(ILogger* logger)
{
    std::lock_guard<std::mutex> lock(gMutex);
    // only set when none-ptr passed
    if (logger)
        gLogger.reset(logger);
}

