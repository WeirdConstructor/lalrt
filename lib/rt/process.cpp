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

#include "rt/process.h"
#include "rt/log.h"

using namespace VVal;

namespace lal_rt
{
//---------------------------------------------------------------------------

std::atomic_int  Port::m_pid_counter;
PortList         Port::m_port_list;

//---------------------------------------------------------------------------

void start_process(Process *p, const VV &args)
{
    VV v_ret;

    L_TRACE << "*PROCESS START* " << p->m_port.pid();
    try
    {
        v_ret = p->execute(args);

        p->m_port.emit_message(vv_list() << "process::exit" << "ok" << v_ret);
    }
    catch (std::exception &ex)
    {
        L_ERROR << "*PROCESS EXCEPTION* (" << p->m_port.pid() << "): " << ex.what();
        p->m_port.emit_message(vv_list() << "process::exit" << "exception" << ex.what());
    }
    catch (std::string &ex)
    {
        L_ERROR << "*PROCESS EXCEPTION* (" << p->m_port.pid() << "): " << ex;
        p->m_port.emit_message(vv_list() << "process::exit" << "exception" << ex);
    }
    catch (...)
    {
        L_ERROR << "*PROCESS EXCEPTION* (" << p->m_port.pid() << ") UNKNOWN";
        p->m_port.emit_message(vv_list() << "process::exit" << "exception");
    }

    L_TRACE << "*PROCESS END* " << p->m_port.pid();

    if (p->should_delete_on_exit())
        delete p;
}
//---------------------------------------------------------------------------

void PortList::reg(Port *p)
{
    std::lock_guard<std::mutex> lg(m_mutex);
    m_list[p->pid()] = p;
}
//---------------------------------------------------------------------------

void PortList::unreg(Port *p)
{
    std::lock_guard<std::mutex> lg(m_mutex);
    m_list.erase(p->pid());
}
//---------------------------------------------------------------------------

} // namespace lal_rt
