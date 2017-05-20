#include <iostream>
#include <functional>
#include <Poco/ThreadPool.h>
#include "rt/lua_thread.h"
#include "rt/log.h"
#include "base/http.h"
//---------------------------------------------------------------------------

using namespace std;
using namespace VVal;
using namespace lal_rt;

//---------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    // if no args: start repl?!
    // if arg: parse header and execute
    // (either Lua or with preloaded and executing LAL)

    try
    {
        start_logging("lalrt.log");
        http_srv::init_ssl();

        VV args(vv_list());
        for (int i = 0; i < argc; i++)
            args << vv(argv[i]);

        VVQ main_queue;
        LuaThread lt;
        lt.m_port.m_parent_emitter.connect(std::bind(&VVQ::push, &main_queue, std::placeholders::_1));
        lt.start(
            "function main(args)\n"
            "   local as_lal = false;\n"
            "   if (string.match(args[2], '.*%.lal$')) then\n"
            "       as_lal = true;\n"
            "   end\n"
            "   if as_lal then\n"
            "       local lal = require 'lal.lal';\n"
            "       return lal.eval_file(args[2]);\n"
            "   else\n"
            "       return dofile(args[2])\n"
            "   end\n"
            "end\n",
            args);

        VV ret(vv_undef());
        bool quit = false;
        while (!quit)
        {
            VV v = main_queue.pop_blocking();
            if (v->_s(2) == "process::exit")
            {
                // exception alread in process.cpp logged
                if (v->_s(3) != "exception")
                {
                    L_INFO << "*RETURN* 0> " << v->_s(4);
                }
                quit = true;
                ret = v->_(4);
            }
            else
            {
                L_INFO << "*ROOT-MSG* 0> " << v;
            }
        }

        lt.join();
    }
    catch (...)
    {
        L_FATAL << "Caught error.";
    }

    Poco::ThreadPool::defaultPool().joinAll();
    http_srv::shutdown_ssl();

    return 0;
}
//---------------------------------------------------------------------------
