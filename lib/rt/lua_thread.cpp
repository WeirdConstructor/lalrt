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

#include "rt/lua_thread.h"
#include "rt/lua_thread_helper.h"
#include "rt/syslib.h"
#include "rt/httplib.h"
#include "rt/utillib.h"
#include "rt/sqldblib.h"
#include <iostream>
#include "rt/log.h"
#if HAS_QT5
#include "modules/qt/init.h"
#endif

using namespace VVal;
using namespace std;

namespace lal_rt
{

static std::string message_token(const VVal::VV &msg)
{
    std::string token;
    if (msg->is_map())       token = msg->_s("command");
    else if (msg->is_list()) token = msg->_s(1);
    else                     token = msg->s();
    return token;
}
//---------------------------------------------------------------------------
/*

(mp-callback <token>/<token list> <prio> <cb>)
token == ""/undef => catch all
(mp-uninstall)
(mp-stop)


*/

std::string LuaThread::m_prelude =
    "local lalrtlib_path = os.getenv('LALRT_LIB');\n"
    "if (not lalrtlib_path) then lalrtlib_path = './lalrtlib/' end\n"
    "package.path = package.path .. ';' .. lalrtlib_path .. '/lua/?.lua;./lal/lua/?.lua'\n"
//    "require 'lal.util.strict'\n"
    "local args = table.pack(...)\n"
    "args.n = nil\n"
    "local main\n"
    "local function init()\n"
    "    local ret\n"
    "    local function my_main()\n"
    "        ret = main(args)\n"
    "    end\n"
    "    local function create_traceback(errorvalue)\n"
    "       return debug.traceback(errorvalue, 3)\n"
    "    end\n"
    "    local ok, errorvalue = xpcall(my_main, create_traceback)\n"
    "    if (not ok) then error(errorvalue) end\n"
    "    return ret\n"
    "end\n";

std::string LuaThread::m_prelude_end =
    "\nreturn init()\n";

//---------------------------------------------------------------------------

VV_CLOSURE_DOC(proc_pid,
"@proc:rt-proc procedure (proc-pid)\n\n"
"Returns the internal process/port ID of this thread.\n"
"This will be 0 for the main thread.\n"
"\n"
"    (proc-pid) ;=> 0 ; for main/first process/port\n"
)
{
    return vv(LT->m_port.pid());
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(proc_terminated_Q,
"@proc:rt-proc procedure (proc-terminated?)\n\n"
"Returns `#true` if thread should stop processing and terminate itself.\n"
"\n"
"    (do () ((not (proc-terminated)) nil)\n"
"      #;(do iterative thread stuff here))\n"
)
{
    return vv_bool(LT->is_terminated());
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(proc_spawn,
"@proc:rt-proc procecdure (proc-spawn _init-program-text_ [_args-data_])\n\n"
"Creates a new process with the init program text _init-program-text_.\n"
"The started init programm is _args-data_ passed as arguments.\n"
"Returns the `pid` of the newly created process.\n"
"\n"
"    (let ((p (proc-spawn \"(mp-send [foobar:])\")))\n"
"      (mp-wait-infinite foobar:))\n"
)
{
    auto child_lt = new LuaThread(true);
    child_lt->m_port.m_parent_emitter.connect(
        std::bind(&Port::handle, &LT->m_port, std::placeholders::_1));
    child_lt->start(vv_args->_s(0), vv_args->_(1));
    return vv(child_lt->m_port.pid());
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(mp_send,
"@mp:rt-mp procedure (mp-send _pid-number_ _message-data_)\n"
"@mp procedure (mp-send _message-data_)\n\n"
"Sends the _message-data_ to the process with _pid-number_ and\n"
"returns the unique token of the message (see also `mp-wait`).\n"
"If _pid-number_ is omitted, the message is directly emitted to\n"
"the parent process.\n"
"\n"
"    (let ((f (mp-send 0 [ping:]))\n"
"          (r (mp-wait-infinite f)))\n"
"      (assert (eq? (.result r) pong:)))\n"
)
{
    if (vv_args->size() == 2)
        return vv(LT->m_port.emit_message(vv_args->_(1), (int) vv_args->_i(0)));
    else
        return vv(LT->m_port.emit_message(vv_args->_(0)));
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(mp_set_debug_logging,
"@mp:rt-mp procedure (mp-set-debug-logging _bool_)\n\n"
"Enables/Disables extensive message logging of the current process.\n"
)
{
    LT->m_port.set_msg_logging(vv_args->_b(0));
    return vv_undef();
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(mp_wait,
"@mp:rt-mp procedure (mp-wait _token-string_ _wait-ms_)\n"
"Waits for a message to arrive in the time [_wait-ms_] milliseconds.\n"
"The _token-string_ or the elements of the _token-string-list_ are matched\n"
"against the third element of the message if the message is a list.\n"
"If the message is a map, it's matched against the `:command` key.\n"
"You can use `nil` or the empty string \"*\" token as catch all token,\n"
"to receive any message.\n"
"\n"
"There are two types of layouts for messages:\n"
"    - list: (<source pid> <unique token> <message command> <arg>*)\n"
"    - map: { :command <message command> :pid <source pid> :token <unique token> }\n"
"The <unique token> and the <source pid> can be used to reply to a message\n"
"if the sender expects this.\n"
"\n"
"See also `mp-wait-infinite` and `mp-check-available`.\n"
"\n"
"   (begin\n"
"       (proc-spawn \"(mp-send [foo: 123])\")\n"
"       (let ((m (mp-wait foo: 1000)))\n"
"           (@1 m))) ;=> 123\n"
)
{
    return LT->wait(vv_args->_(0), (int) vv_args->_i(1));
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(mp_wait_infinite,
"@mp:rt-mp procedure (mp-wait-infinite _token-string_)\n"
"`mp-wait-infinite` waits infinitely until a message arrived."
"The _token-string_ or the elements of the _token-string-list_ are matched\n"
"against the third element of the message if the message is a list.\n"
"If the message is a map, it's matched against the `:command` key.\n"
"You can use `nil` or the empty string \"*\" token as catch all token,\n"
"to receive any message.\n"
"\n"
"There are two types of layouts for messages:\n"
"    - list: (<source pid> <unique token> <message command> <arg>*)\n"
"    - map: { :command <message command> :pid <source pid> :token <unique token> }\n"
"The <unique token> and the <source pid> can be used to reply to a message\n"
"if the sender expects this.\n"
"\n"
"   (begin\n"
"       (proc-spawn \"(mp-send [foo: 123])\")\n"
"       (let ((m (mp-wait-infinite foo:)))\n"
"           (@1 m))) ;=> 123\n"
)
{
    return LT->wait_infinite(vv_args->_(0));
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(mp_check_available,
"@mp:rt-mp procedure (mp-check-available _token-string_)\n"
"`mp-check-available` returns immediately after checking for matching messages.\n"
"The _token-string_ or the elements of the _token-string-list_ are matched\n"
"against the third element of the message if the message is a list.\n"
"If the message is a map, it's matched against the `:command` key.\n"
"You can use `nil` or the empty string \"*\" token as catch all token,\n"
"to receive any message.\n"
"\n"
"See also `mp-wait-infinite` and `mp-wait`\n"
"\n"
"   (begin\n"
"       (proc-spawn \"(mp-send [foo: 123])\")\n"
"       (let ((m (mp-check-available foo:)))\n"
"           (when m (@1 m)))) ;=> 123 or nil if none available\n"
)
{
    return LT->check_available(vv_args->_(0));
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(mp_add_default_handler,
"@mp:rt-mp procedure (mp-add-default-handler _priority-num_ _callback-function_)\n\n"
"@mp procedure (mp-add-default-handler _callback-function_)\n\n"
"Installs the _callback-function_ as default handler for any unhandled\n"
"messages to the current process/port.\n"
"The numeric value of _priority-num_ decides the order in which the default handlers\n"
"are called. Where lower means earlier, and higher means later. Default should be 0.\n"
)
{
    return vv(LT->install_default_handler(vv_args->_(0)));
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(mp_remove_default_handler,
"@mp:rt-mp procedure (mp-remove-default-handler _token_)\n\n"
"Uninstalls the default handler with the given _token_.\n"
"The _token_ is returned by `mp-add-default-handler` upon registration of"
"a default handler.\n"
)
{
    LT->uninstall_default_handler(vv_args->_i(0));
    return vv_undef();
}
//---------------------------------------------------------------------------


VV_CLOSURE_DOC(mp_token,
"@mp:rt-mp procedure (mp-token)\n"
"Returns a new token, that can be passed to various functions to\n"
"have a unqiue value that can be used to register callbacks or waiting\n"
"points for (mp-redirect) or (mp-wait)\n"
)
{
    return vv(LT->m_port.new_token());
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(lal_dump,
"@lal:rt-lal procedure (lal-dump _data_)\n\n"
"Returns a serialized form of _data_ as LAL-Datastructure.\n"
)
{
    std::stringstream ss;
    ss << vv_args;
    return vv(ss.str());
}
//---------------------------------------------------------------------------

void LuaThread::init_rt_lib(Lua::Instance &lua)
{
    VV obj(vv_list() << vv_ptr(this, "LuaThread"));

    LUA_REG(lua, "proc", "terminatedQ",         obj, proc_terminated_Q);
    LUA_REG(lua, "proc", "pid",                 obj, proc_pid);
    LUA_REG(lua, "proc", "spawn",               obj, proc_spawn);

    LUA_REG(lua, "mp",   "addDefaultHandler",   obj, mp_add_default_handler);
    LUA_REG(lua, "mp",   "removeDefaultHandler",obj, mp_remove_default_handler);
    LUA_REG(lua, "mp",   "wait",                obj, mp_wait);
    LUA_REG(lua, "mp",   "waitInfinite",        obj, mp_wait_infinite);
    LUA_REG(lua, "mp",   "checkAvailable",      obj, mp_check_available);
    LUA_REG(lua, "mp",   "send",                obj, mp_send);
    LUA_REG(lua, "mp",   "setDebugLogging",     obj, mp_set_debug_logging);
    LUA_REG(lua, "mp",   "token",               obj, mp_token);

    LUA_REG(lua, "lal",  "dump",                obj, lal_dump);

    init_syslib(this, lua);
    init_sqldblib(this, lua);
    init_httplib(this, lua);
    init_utillib(this, lua);

#if HAS_QT5
    init_qtlib(this, lua);
#endif
}
//---------------------------------------------------------------------------

bool LuaThreadMessageHandler::match_tokens(const VVal::VV &msg, const VVal::VV &tokens)
{
    if (!tokens)            return false;
    if (tokens->is_undef()) return true;
    if (msg->is_map())
    {
        if (tokens->is_list())
            for (auto t : *tokens)
            {
                if (t->s() == "") return true;

                if (t->s() == msg->_s("command"))
                    return true;
            }
        else if (tokens->s() == "")
            return true;
        else if (tokens->s() == msg->_s("command"))
            return true;
    }
    else
    {
        if (tokens->is_list())
            for (auto t : *tokens)
            {
                if (t->s() == "") return true;

                if (t->s() == msg->_s(2))
                    return true;
            }
        else if (tokens->s() == "")
            return true;
        else if (tokens->s() == msg->_s(2))
            return true;
    }

    return false;
}
//---------------------------------------------------------------------------

bool LuaThreadMessageHandler::match_message(const VVal::VV &msg, const VVal::VV &tokens)
{
    if (tokens && match_tokens(msg, tokens))
        return true;
    else
        call_default_handlers(msg, vv_undef());
    return false;
}
//---------------------------------------------------------------------------

VVal::VV LuaThreadMessageHandler::check_arrived_msgs(const VVal::VV &tokens)
{
    while (!m_queue.empty())
    {
        Optional<VV> v = m_queue.pop_now();
        if (!v.has_value()) break;
        VVal::VV msg = v.value_or(vv_undef());

        if (match_message(msg, VVal::VV()))
            return msg;
    }

    return VVal::VV();
}
//---------------------------------------------------------------------------

void LuaThreadMessageHandler::process_messages_now()
{
    VVal::VV msg =
        check_arrived_msgs(VVal::VV());

    while (msg)
    {
        call_default_handlers(msg, vv_undef());
        msg = check_arrived_msgs(VVal::VV());
    }
}
//---------------------------------------------------------------------------

VVal::VV LuaThreadMessageHandler::wait_infinite(const VVal::VV &tokens)
{
    while (true)
    {
        VVal::VV msg = m_queue.pop_blocking();

        if (match_message(msg, tokens))
            return msg;
    }

    return vv_undef(); // never reached (hopefully)
}
//---------------------------------------------------------------------------

VVal::VV LuaThreadMessageHandler::wait(const VVal::VV &tokens, int wait_time_ms)
{
    auto tp_end =
        std::chrono::high_resolution_clock::now()
        + std::chrono::milliseconds(wait_time_ms);

    while (true)
    {
        // FIXME: re-waiting full wait_time_ms when an unhandled message
        //        arrives is bad! Should decrease wait_time_ms by already
        //        waited time.
        Optional<VVal::VV> omsg = m_queue.pop_waiting(wait_time_ms);
        if (omsg.has_value())
        {
            VVal::VV msg = omsg.value_or(vv_undef());

            if (match_message(msg, tokens))
                return msg;
        }

        if (std::chrono::high_resolution_clock::now() >= tp_end)
            break;
    }

    return vv_undef(); // never reached (hopefully)
}
//---------------------------------------------------------------------------

void LuaThread::notify_msg_arrived_async()
{
    std::lock_guard<std::mutex> lg(m_ev_loop_mutex);
    if (!m_ev_loop)
        return;

    m_ev_loop->unsafe_notify_msg_arrived(this);
}
//---------------------------------------------------------------------------

VVal::VV LuaThread::check_available(const VVal::VV &tokens)
{
    VVal::VV msg = m_msg_handler.check_arrived_msgs(tokens);
    return msg ? msg : vv_undef();
}
//---------------------------------------------------------------------------

VVal::VV LuaThread::call_lua_func_or_emit(const VVal::VV &func, const VVal::VV &args)
{
    if (!func->is_pointer() || func->type() != "LuaFunction")
    {
        VVal::VV msg(vv_list());
        msg << func;
        for (auto v : *args)
            msg << v;
        return vv(m_port.emit_message(msg, m_port.pid()));
    }

    Lua::LuaFunction *lf = (Lua::LuaFunction *) func->p("LuaFunction");
    if (!lf)
    {
        L_FATAL << "Bad LuaFunction value used as callback!" << func;
        return vv_undef();

    }
    return lf->call(args);
}
//---------------------------------------------------------------------------

void LuaThreadMessageHandler::call_default_handlers(const VVal::VV &msg, const VVal::VV &arg)
{
    // Make a copy, so we can safely remove any elements inside the loop:
    std::list<VVal::VV> m_handlers = m_default_handlers;

    for (auto it = m_handlers.begin();
         it != m_handlers.end();
         )
    {
        VVal::VV hdl = *it;
        if (!hdl->_(1)->is_pointer())
        {
            L_ERROR << "Can't install non pointer as default handler: " << hdl;
            it++;
            continue;
        }

        Lua::LuaFunction *lf = (Lua::LuaFunction *) hdl->_(1)->p("LuaFunction");
        if (lf)
        {
            lf->call(vv_list() << msg << arg);
            it++;
        }
        else
        {
            L_ERROR << "Can't call non LuaFunction as default handler: " << hdl;
            it++;
        }
    }
}
//---------------------------------------------------------------------------

};
