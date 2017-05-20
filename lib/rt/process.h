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

#include "rt/log.h"
#include "base/msg_queue.h"
#include "base/vval.h"
#include <atomic>
#include <unordered_set>
#include <iostream>
#include <boost/signals2.hpp>

namespace lal_rt
{

typedef MsgQueue<VVal::VV>  VVQ;

class Process;
void start_process(Process *p, const VVal::VV &args);

//---------------------------------------------------------------------------

class Port;
class PortList
{
    private:
        std::unordered_map<int, Port *> m_list;
        std::mutex                      m_mutex;

    public:
        PortList() { }
        void reg(Port *p);
        void unreg(Port *p);
        Port *get(int pid)
        {
            std::lock_guard<std::mutex> lg(m_mutex);
            if (m_list.find(pid) != m_list.end())
                return m_list[pid];
            return nullptr;
        }
};
//---------------------------------------------------------------------------

class Port
{
    private:
        static std::atomic_int          m_pid_counter;
        static PortList                 m_port_list;
        std::atomic<int64_t>            m_token_counter;
        int                             m_pid;
        bool                            m_msg_logging;

        std::function<bool(const VVal::VV &msg)> m_handler_interception;

    public:
        boost::signals2::signal<void(const VVal::VV &)> m_parent_emitter;
        boost::signals2::signal<void()>                 m_unsafe_msg_arrived;
        VVQ m_queue;

        Port()
            : m_queue(std::bind(&Port::notify_msg_arrived_async, this)),
              m_msg_logging(false),
              m_token_counter(0)
        {
            m_pid = m_pid_counter++;
            m_port_list.reg(this);
        }

        Port(std::function<bool(const VVal::VV &)> interceptor)
            : m_queue(std::bind(&Port::notify_msg_arrived_async, this)),
              m_token_counter(0),
              m_msg_logging(false),
              m_handler_interception(interceptor)
        {
            m_pid = m_pid_counter++;
            m_port_list.reg(this);
        }

        virtual ~Port()
        {
            m_port_list.unreg(this);
        }

        int pid() { return m_pid; }

        int64_t new_token() { return m_token_counter++; }

        void notify_msg_arrived_async() { m_unsafe_msg_arrived(); }

        virtual void set_msg_logging(bool b) { m_msg_logging = b; }

        int64_t emit_message(const VVal::VV &base_msg, int pid = -1)
        {
            VVal::VV msg;
            int64_t token = new_token();

            if (base_msg->is_list())
            {
                msg = VVal::vv_list() << m_pid << token;
                for (auto i : *base_msg)
                    msg << i;
            }
            else if (base_msg->is_map())
            {
                base_msg->set("pid",   VVal::vv(m_pid));
                base_msg->set("token", VVal::vv(token));
                msg = base_msg;
            }
            else
            {
                msg = VVal::vv_list() << m_pid << token;
                msg << base_msg;
            }

            if (m_msg_logging)
            {
                L_TRACE << "(" << m_pid << ") emit(->" << pid << "): " << msg;
            }

            if (pid >= 0)
            {
                Port *dest = nullptr;
                if (pid == m_pid) dest = this;
                else              dest = m_port_list.get(pid);
                dest->handle(msg);
            }
            else
                m_parent_emitter(msg);

            return token;
        }

        virtual void handle(const VVal::VV &msg)
        {
            if (m_msg_logging)
            {
                L_TRACE << "(" << m_pid << ") handle: " << msg;
            }

            if (m_handler_interception
                && m_handler_interception(msg))
            {
                return;
            }

            m_queue.push(msg);
        }
};
//---------------------------------------------------------------------------

class Process
{
    private:
        bool                            m_started;
        std::atomic_bool                m_terminate;
        bool                            m_delete_on_exit;
        std::thread                    *m_thread;

    public:
        Port                            m_port;

        Process(bool delete_on_exit = false)
            : m_started(false),
              m_terminate(false),
              m_delete_on_exit(delete_on_exit),
              m_port(std::bind(&Process::intercept_process_related,
                               this, std::placeholders::_1)),
              m_thread(nullptr)
        {
        }

        void join()
        {
            if (m_thread)
            {
                m_terminate = true;
                m_thread->join();
                delete m_thread;
                m_thread = nullptr;
            }
        }

        virtual ~Process()
        {
            if (m_thread)
            {
                m_terminate = true;
                m_thread->detach();
                delete m_thread;
                m_thread = nullptr;
            }
        }

        virtual void start(const VVal::VV &args)
        {
            if (m_started)
                return;

            m_started   = true;
            m_terminate = false;
            m_thread    = new std::thread(start_process, this, args);
        }

        bool intercept_process_related(const VVal::VV &msg)
        {
            if (msg->_s(2) == "process::terminate")
            {
                m_terminate = true;
                // fall through, because we want to wake up
                // the process (if it is interested)
            }

            return false;
        }

        bool is_terminated() const { return m_terminate; }
        bool should_delete_on_exit() const { return m_delete_on_exit; }

        virtual VVal::VV execute(const VVal::VV &args) = 0;
};
//---------------------------------------------------------------------------

}; // namespace lal_rt
