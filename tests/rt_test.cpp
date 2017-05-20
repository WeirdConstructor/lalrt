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
#define BOOST_TEST_MAIN
//#define BOOST_TEST_ALTERNATIVE_INIT_API
#include <boost/test/unit_test.hpp>
#include "base/vval.h"
#include "rt/process.h"
#include "rt/lua_thread.h"
#include "rt/log.h"
#include <sstream>
#include <functional>
//---------------------------------------------------------------------------
using namespace VVal;
using namespace std::placeholders;
//---------------------------------------------------------------------------
class MyProcess : public lal_rt::Process
{
    public:
        MyProcess(lal_rt::VVQ &emit_queue)
        {
            m_port.m_parent_emitter.connect(
                std::bind(&lal_rt::VVQ::push, &emit_queue, std::placeholders::_1));
        }
        int64_t sum;
        virtual VV execute(const VV &args)
        {
            L_TRACE << "EXEC";
            sum = args->_i(0);
            VV m;
            m = m_port.m_queue.pop_blocking();
            sum += m->_i(2);
            m = m_port.m_queue.pop_blocking();
            sum += m->_i(2);
            m = m_port.m_queue.pop_blocking();
            sum += m->_i(2);
            return vv(sum);
        }
};

BOOST_AUTO_TEST_CASE(undef_vval)
{
    L_TRACE << "X";
    lal_rt::VVQ m_main_q;
    MyProcess my_proc(m_main_q);
    my_proc.m_port.set_msg_logging(true);
    my_proc.start(vv_list() << 12);
    my_proc.m_port.handle(vv_list() << 0 << "count" << 20);
    my_proc.m_port.handle(vv_list() << 0 << "count" << 50);
    my_proc.m_port.handle(vv_list() << 0 << "count" << 70);
    VV m = m_main_q.pop_blocking();
    L_TRACE << m;
    BOOST_CHECK_EQUAL(m->_s(2), "process::exit");
    BOOST_CHECK_EQUAL(m->_s(3), "ok");
    BOOST_CHECK_EQUAL(m->_s(4), "152");
}
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(lua_proc)
{
    L_TRACE << "X2";
    lal_rt::VVQ m_main_q2;
    lal_rt::LuaThread lt;
    lt.m_port.m_parent_emitter.connect(std::bind(&lal_rt::VVQ::push, &m_main_q2, std::placeholders::_1));
    lt.start(
            "function main(args)\n"
            "local a, b, c = table.unpack(args)\n"
            "return (tostring(a) .. tostring(c['x']))\nend\n",
            vv_list()
            << 12
            << "\xFF""123ABC"
            << (vv_map() << vv_kv("x", 120)));
    VV m = m_main_q2.pop_blocking();
    L_TRACE << m;
    BOOST_CHECK_EQUAL(m->_s(2), "process::exit");
    BOOST_CHECK_EQUAL(m->_s(3), "ok");
    BOOST_CHECK_EQUAL(m->_i(4), 12120);
}
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(lua_proc_lib)
{
    L_TRACE << "X3";
    lal_rt::VVQ m_main_q2;
    lal_rt::LuaThread lt;
    lt.m_port.m_parent_emitter.connect(std::bind(&lal_rt::VVQ::push, &m_main_q2, std::placeholders::_1));
    lt.start(
        "function main(args)\n"
        "while not proc.terminatedQ()\n"
        "do\n"
        "   print('XXX\\n')\n"
        "end\nreturn 99\nend\n",
        vv_list());
    lt.m_port.handle(vv_list() << -1 << 0 << "process::terminate");
    VV m = m_main_q2.pop_blocking();
    L_TRACE << m;
    BOOST_CHECK_EQUAL(m->_s(2), "process::exit");
    BOOST_CHECK_EQUAL(m->_s(3), "ok");
    BOOST_CHECK_EQUAL(m->_i(4), 99);
}
//---------------------------------------------------------------------------
