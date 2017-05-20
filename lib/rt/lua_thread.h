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
#include "lua/lua_instance.h"
#include "rt/process.h"
#include <queue>
#include <map>

namespace lal_rt
{
class LuaThreadException : public std::exception
{
    private:
            std::string m_err;
    public:
        LuaThreadException(const std::string &e) : m_err(e) { }
        virtual const char *what() const noexcept { return m_err.c_str(); }
};

//---------------------------------------------------------------------------

class LuaThread;

class LuaThreadMessageHandler
{
    private:
        VVQ                             &m_queue;
        std::list<VVal::VV>              m_default_handlers;

        bool redirect_to_callback(const VVal::VV &msg);
        bool match_tokens(const VVal::VV &msg, const VVal::VV &tokens);
        bool match_message(const VVal::VV &msg, const VVal::VV &tokens);
        void call_default_handlers(const VVal::VV &msg, const VVal::VV &arg);

    public:
        LuaThreadMessageHandler(VVQ &queue)
            : m_queue(queue)
        {
        }

        VVal::VV check_arrived_msgs(const VVal::VV &tokens);
        VVal::VV wait(const VVal::VV &tokens, int wait_time_ms = 0);
        VVal::VV wait_infinite(const VVal::VV &tokens);

        void process_messages_now();

        int64_t install_default_handler(const VVal::VV &callback, int64_t token)
        {
            m_default_handlers.push_back(VVal::vv_list() << token << callback);
            return token;
        }

        void uninstall_default_handler(int64_t token)
        {
            m_default_handlers.remove_if(
                [token](const VVal::VV &a) { return a->_i(0) == token; });
        }

        void clear() { m_default_handlers.clear(); }
};
//---------------------------------------------------------------------------

class LuaResourceManager
{
    private:
        std::unordered_set<void *>       m_allocatedResources;

    public:
        void register_resource(void *p)
        {
            m_allocatedResources.insert(p);
        }
        bool has_registered_resource(void *p)
        {
            return m_allocatedResources.find(p) != m_allocatedResources.end();
        }
        void delete_resource(void *p)
        {
            m_allocatedResources.erase(p);
        }
        void check_resource(const VVal::VV &v, const std::string &type)
        {
            if (!this->has_registered_resource(v->p(type)))
            {
                std::string err =
                    "Access to deleted/unallocated resource with type ";
                err += v->type();
                throw LuaThreadException(err);
            }
        }
};
//---------------------------------------------------------------------------

class EventLoop
{
    protected:
        std::function<void()>   m_handle_messages_synchronized;
        std::function<void()>   m_on_destroy;

    public:
        EventLoop() { }
        virtual ~EventLoop()
        {
            if (m_on_destroy)
                m_on_destroy();
        }

        void setDestroyHandler(const std::function<void()> &handler)
        {
            m_on_destroy = handler;
        }
        void setSynchronizedHandler(const std::function<void()> &handler)
        {
            m_handle_messages_synchronized = handler;
        }
        virtual void unsafe_notify_msg_arrived(Process *proc)
        {
            // post an event into the event loop to call m_handle_messages_synchronized
        }
};
//---------------------------------------------------------------------------

//#include <boost/asio.hpp>
/*
    - io_service läuft in jedem prozess
    - neue nachrichten in queue machen einen io_service.post()
    - io_service optional! => wenn, dann msg-queue entsprechend abgefragt.
*/
//class AsioEventLoop : public EventLoop
//{
//    private:
//        boost::asio::io_service *m_io_service;
//
//    public:
//        AsioEventLoop(boost::asio::io_service *io_service)
//            : m_io_service(io_service)
//        { }
//        virtual AsioEventLoop() { }
//
//        virtual void unsafe_notify_msg_arrived()
//        {
//            m_io_service->post([this]()
//            {
//                m_handle_messages_synchronized();
//
//                if (this->is_terminated())
//                    m_io_service->stop();
//            });
//        }
//}
//---------------------------------------------------------------------------

class LuaThread : public Process
{
    private:
        static std::string               m_prelude;
        static std::string               m_prelude_end;
        std::string                      m_lua_init_code;
        Lua::Instance                   *m_lua;

        LuaThreadMessageHandler          m_msg_handler;
        LuaResourceManager               m_rm;

        std::mutex                       m_ev_loop_mutex;
        EventLoop                       *m_ev_loop;

        void init_rt_lib(Lua::Instance &lua);

    public:
        LuaThread(bool delete_on_exit = false)
            : Process(delete_on_exit),
              m_msg_handler(m_port.m_queue),
              m_ev_loop(nullptr)
        {
            m_port.m_unsafe_msg_arrived.connect(
                std::bind(&LuaThread::notify_msg_arrived_async, this));
        }
        virtual void notify_msg_arrived_async();

        int64_t install_default_handler(const VVal::VV &callback)
        { return m_msg_handler.install_default_handler(callback, m_port.new_token()); }
        void uninstall_default_handler(int64_t token)
        { m_msg_handler.uninstall_default_handler(token); }

        void register_resource(void *p)
        { m_rm.register_resource(p); }
        bool has_registered_resource(void *p)
        { return m_rm.has_registered_resource(p); }
        void delete_resource(void *p)
        { m_rm.delete_resource(p); }
        void check_resource(const VVal::VV &v, const std::string &type)
        { m_rm.check_resource(v, type); }

        void start(const std::string &lua_code, const VVal::VV &args)
        {
            m_lua_init_code = m_prelude
                            + "\n------------------------------\n"
                            + lua_code
                            + m_prelude_end;
            this->Process::start(args);
        }

        void set_event_loop(EventLoop *ev_loop)
        {
            std::lock_guard<std::mutex> lg(m_ev_loop_mutex);
            m_ev_loop = ev_loop;
            ev_loop->setSynchronizedHandler(
                std::bind(&LuaThreadMessageHandler::process_messages_now, &m_msg_handler));
            ev_loop->setDestroyHandler([this]() {
                std::lock_guard<std::mutex> lg(m_ev_loop_mutex);
                m_ev_loop = 0;
            });
        }

        VVal::VV call_lua_func_or_emit(const VVal::VV &func, const VVal::VV &args);

        VVal::VV check_available(const VVal::VV &tokens);
        VVal::VV wait(const VVal::VV &tokens, int wait_time_ms)
        { return m_msg_handler.wait(tokens, wait_time_ms); }
        VVal::VV wait_infinite(const VVal::VV &tokens)
        { return m_msg_handler.wait_infinite(tokens); }

        virtual VVal::VV execute(const VVal::VV &args)
        {
            VVal::VV res;
            try
            {
                Lua::Instance *lua = new Lua::Instance;
                m_lua = lua;
                m_lua->init_output_interface();
                this->init_rt_lib(*m_lua);
                std::string code_name = (format("prelude(pid %1%)") % this->m_port.pid()).str();
                res = m_lua->eval_code(m_lua_init_code, args, code_name);
            }
            catch (const std::exception &)
            {
                m_msg_handler.clear();
                m_port.m_queue.clear();
                delete m_lua;
                m_lua = 0;
                throw;
            }

            m_msg_handler.clear();
            m_port.m_queue.clear();
            delete m_lua;
            m_lua = 0;

            return res;
        }
};
//---------------------------------------------------------------------------

}; // namespace lal_rt
