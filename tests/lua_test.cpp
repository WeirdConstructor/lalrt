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
#include "lua/lua_instance.h"

//---------------------------------------------------------------------------

using namespace std;
using namespace VVal;

//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(basic_usage)
{
    Lua::Instance li;
    li.init_output_interface();

    VV v = li.eval_code("return 10 + 22.2");
    BOOST_CHECK_EQUAL(v->d(), 32.2);

    VV varg(vv_list() << 10 << "foobar");
    VV v2 = li.eval_code("local a, b = ...;"
                         "return (tostring(a) .. b .. 'X')",
                         varg);
    BOOST_CHECK_EQUAL(v2->s(), "10foobarX");
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(testoutcall_closure, "call C++ VV closure")
{
    return vv(vv_args->_s(0) + vv_args->_s(1) + vv_obj->_s(0));
}

BOOST_AUTO_TEST_CASE(callfunc)
{
    Lua::Instance li;
    li.init_output_interface();

    VV vvObj(vv_list() << 123);
    LUA_REG(   li, "test", "outcall", vvObj, testoutcall_closure);
    LUA_REG_UD(li, "test", "outcall2", testoutcall_closure);

    VV v = li.eval_code("return test.outcall(12, 34)");
    BOOST_CHECK_EQUAL(v->s(), "1234123");

    VV v2 = li.eval_code("return test.outcall2(12, 34)");
    BOOST_CHECK_EQUAL(v2->s(), "1234");

    VV vdoc  = li.eval_code("return test_doc.outcall");
    VV vdoc2 = li.eval_code("return test_doc.outcall2");
    BOOST_CHECK_EQUAL(vdoc->s(),  "call C++ VV closure");
    BOOST_CHECK_EQUAL(vdoc2->s(), "call C++ VV closure");
}
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(callfromoutside)
{
    Lua::Instance li;
    li.init_output_interface();

    li.eval_code("function testmain(a, b)\n"
                 "    return (tostring(a) .. 'X' .. tostring(b));\n"
                 "end\n");
    VV v = li.call("testmain", vv_list() << 12 << "möp");
    BOOST_CHECK_EQUAL(v->s(), "12Xmöp");
}
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(pointerpassing)
{
    Lua::Instance li;
    li.init_output_interface();

    VV vp(vv_ptr((void *) 0x54321, "Poop"));

    VV v2 = li.eval_code("local a = ...; return a", vv_list() << vp);

    BOOST_CHECK_EQUAL(v2->p("Poop"), (void *) 0x54321);
}
//---------------------------------------------------------------------------

