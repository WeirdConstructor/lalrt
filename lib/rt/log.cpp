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

#define _CRT_SECURE_NO_WARNINGS 1

#include "log.h"
#include <iostream>
#include <sstream>
#include <boost/log/support/date_time.hpp>
#include <boost/log/support/exception.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/attributes/current_process_id.hpp>
#include <boost/log/expressions/formatters/format.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/expressions/formatters/named_scope.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

namespace lal_rt
{

//---------------------------------------------------------------------------

namespace logging  = boost::log;
namespace src      = boost::log::sources;
namespace sinks    = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr     = boost::log::expressions;
namespace attrs    = boost::log::attributes;

BOOST_LOG_GLOBAL_LOGGER_DEFAULT(lal_rt::g_log, src::severity_logger_mt<boost::log::trivial::severity_level>)

//---------------------------------------------------------------------------

lalrt_logfile_backend::lalrt_logfile_backend(const std::string &filepath)
    : m_filepath(filepath), m_filehandle(0)
{
}
//---------------------------------------------------------------------------

void lalrt_logfile_backend::consume(boost::log::record_view const &rec,
                                     const std::string &msg)
{
    if (!m_filehandle)
        m_filehandle = fopen(m_filepath.c_str(), "a+");

    if (!m_filehandle)
    {
        std::cerr << "Konnte logfile nicht öffnen: " << m_filepath << std::endl;
        return;
    }

    std::stringstream s;
    s << msg << std::endl;
    std::string record_string = s.str();
    fwrite(record_string.c_str(), sizeof(char), record_string.size(), m_filehandle);
    fflush(m_filehandle);
    fclose(m_filehandle);
    m_filehandle = NULL;
}
//---------------------------------------------------------------------------

void lalrt_logfile_backend::flush()
{
    if (m_filehandle)
        fclose(m_filehandle);
}
//---------------------------------------------------------------------------

void start_logging(const std::string &logfile)
{
    using namespace std;

    auto fmt =
        expr::stream
        << expr::format_date_time<boost::posix_time::ptime>(
            "TimeStamp", "%Y%m%d %H:%M:%S.%f")
        << " "
        << setw(7)
        << logging::trivial::severity
        << ":"
        << setw(0)
        << expr::attr<attrs::current_thread_id::value_type>("ThreadID")
        << "|"
        << expr::format_named_scope("Scopes",
                keywords::format            = "%c",
                keywords::iteration         = expr::reverse,
                keywords::depth             = 1,
                keywords::incomplete_marker = "")
        << "@"
        << expr::format_named_scope("Scopes",
                keywords::format            = "%F",
                keywords::iteration         = expr::reverse,
                keywords::depth             = 1,
                keywords::incomplete_marker = "")
        << ":"
        << setw(5)
        << setfill(' ')
        << expr::format_named_scope("Scopes",
                keywords::format            = "%l",
                keywords::iteration         = expr::reverse,
                keywords::depth             = 1,
                keywords::incomplete_marker = "")
        << "| "
        << expr::smessage;

    logging::add_console_log
    (
        cerr,
        keywords::auto_flush = true,
        keywords::format     = fmt
    );

    {
        typedef sinks::synchronous_sink<lalrt_logfile_backend> sink_t;
        boost::shared_ptr<lalrt_logfile_backend> backend(
            new lalrt_logfile_backend(logfile));
        boost::shared_ptr<sink_t> sink(new sink_t(backend));
        sink->set_formatter(fmt);
        logging::core::get()->add_sink(sink);
    }

    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::trace
    );

    logging::add_common_attributes();
    logging::core::get()->add_global_attribute("Scopes", attrs::named_scope());
    logging::core::get()->add_global_attribute(
        "ProcessID",
        attrs::current_process_id());
    logging::core::get()->add_global_attribute(
        "ThreadID",
        attrs::current_thread_id());
}
//---------------------------------------------------------------------------

} // namespace lal_rt
