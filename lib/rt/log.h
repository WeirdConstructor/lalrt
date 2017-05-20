/******************************************************************************
* Copyright (C) 2017 Weird Constructor
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/

#pragma once

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/frontend_requirements.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/global_logger_storage.hpp>

#define L_ERROR  BOOST_LOG_FUNCTION(); BOOST_LOG_SEV(lal_rt::g_log::get(), boost::log::trivial::error)
#define L_FATAL  BOOST_LOG_FUNCTION(); BOOST_LOG_SEV(lal_rt::g_log::get(), boost::log::trivial::fatal)
#define L_WARN   BOOST_LOG_FUNCTION(); BOOST_LOG_SEV(lal_rt::g_log::get(), boost::log::trivial::warning)
#define L_INFO   BOOST_LOG_FUNCTION(); BOOST_LOG_SEV(lal_rt::g_log::get(), boost::log::trivial::info)
#define L_DEBUG  BOOST_LOG_FUNCTION(); BOOST_LOG_SEV(lal_rt::g_log::get(), boost::log::trivial::debug)
#define L_TRACE  BOOST_LOG_FUNCTION(); BOOST_LOG_SEV(lal_rt::g_log::get(), boost::log::trivial::trace)

namespace lal_rt
{

BOOST_LOG_GLOBAL_LOGGER(g_log, boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level>);

//---------------------------------------------------------------------------

void start_logging(const std::string &logfile);

//---------------------------------------------------------------------------

class lalrt_logfile_backend :
    public boost::log::sinks::basic_formatted_sink_backend<
        boost::log::sinks::combine_requirements<
            boost::log::sinks::synchronized_feeding,
            boost::log::sinks::formatted_records,
            boost::log::sinks::flushing
        >::type
    >
{
private:
    std::string         m_filepath;
    FILE               *m_filehandle;

public:
    typedef char                             char_type;              // Character type.
    typedef std::string                      string_type;            // Formatted string type.
//    typedef base_type::frontend_requirements frontend_requirements;  // Frontend requirements.

    explicit lalrt_logfile_backend(const std::string &filepath);

    void consume(boost::log::record_view const& rec, const std::string &msg);
    void flush();
};
//---------------------------------------------------------------------------

} // namespace lal_rt
