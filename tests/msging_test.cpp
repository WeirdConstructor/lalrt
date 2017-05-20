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
#include <atomic>
#if defined BZVC
#    include "bz/vval.h"
#    include "bz/msg_queue.h"
#else
#    include "base/vval.h"
#    include "base/msg_queue.h"
#endif

using namespace std;
using namespace VVal;

typedef MsgQueue<VV> VVQ;

//---------------------------------------------------------------------------

VVQ s_queue1;
std::atomic<int> cntr1;

//---------------------------------------------------------------------------

void increment(int time_offs)
{
    for (int i = 0; i < 10; i++)
    {
        int tout = 10 * ((i * 2) + time_offs);
        std::this_thread::sleep_for(std::chrono::milliseconds(tout));
        s_queue1.push(vv_list() << cntr1++ << vv(to_string(tout) + "/" + to_string(time_offs)));
    }
}

BOOST_AUTO_TEST_CASE(basic_usage)
{
    // - messages: uniq id, absender pid, ziel pid, inhalt
    // - spawnen von lua threads muss einfach sein
    //   (lal-code-strutkuren als basis für die "execute" methode des threads?!)
    // - jegliche funktionalität wird via lua-API-binding angeboten
    //
    std::thread t1(increment, 1);
    std::thread t3(increment, 2);

    VV lvals(vv_list());
    for (int i = 0; i < 20; i++)
    {
        VV x = s_queue1.pop_blocking();
        lvals->push(vv(x->_s(0) + ":" + x->_s(1)));
        std::cout << x->_s(0) << ":" << x->_s(1) << std::endl;
    }

    int i = 0;
    BOOST_CHECK_EQUAL(lvals->_s(i++), "0:10/1");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "1:20/2");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "2:30/1");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "3:40/2");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "4:50/1");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "5:60/2");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "6:70/1");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "7:80/2");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "8:90/1");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "9:100/2");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "10:110/1");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "11:120/2");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "12:130/1");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "13:140/2");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "14:150/1");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "15:160/2");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "16:170/1");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "17:180/2");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "18:190/1");
    BOOST_CHECK_EQUAL(lvals->_s(i++), "19:200/2");

    t1.join();
    t3.join();
}
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(emptyqueue)
{
    BOOST_TEST_CHECK(s_queue1.empty());
    std::thread t1(increment, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    BOOST_TEST_CHECK(!s_queue1.empty());
    t1.join();
    BOOST_TEST_CHECK(!s_queue1.empty());
    s_queue1.clear();
    BOOST_TEST_CHECK(s_queue1.empty());
}
//---------------------------------------------------------------------------

struct Blanotifier
{
    bool m_notified;
    Blanotifier() : m_notified(false) { }
    void operator()() { m_notified = true; }
};

BOOST_AUTO_TEST_CASE(notifier)
{
    Blanotifier n;
    MsgQueue<VV> mq(std::ref(n));

    mq.push(vv(123));
    BOOST_TEST_CHECK(n.m_notified);

    BOOST_CHECK_EQUAL(mq.pop_now().value_or(vv_undef())->s(), "123");

    mq.clear();

    BOOST_CHECK_EQUAL(mq.pop_waiting(100).value_or(vv(666123))->i(), 666123);
}
//---------------------------------------------------------------------------

