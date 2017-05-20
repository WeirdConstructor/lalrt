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
#include <boost/test/unit_test.hpp>
#include <sstream>
#if defined BZVC
#    include "bz/vval.h"
#    include "bz/vval_util.h"
#else
#    include "base/vval_util.h"
#endif

using namespace VVal;

//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(undef_vval)
{
    VV v(vv_undef());

    BOOST_TEST_CHECK(v->is_undef());
    BOOST_TEST_CHECK(!v->is_defined());
    BOOST_TEST_CHECK(v->is_false());
    BOOST_TEST_CHECK(!v->is_true());
    BOOST_TEST_CHECK(!v->is_boolean());
    BOOST_TEST_CHECK(!v->is_string());
    BOOST_TEST_CHECK(!v->is_bytes());
    BOOST_TEST_CHECK(!v->is_int());
    BOOST_TEST_CHECK(!v->is_double());
    BOOST_TEST_CHECK(!v->is_pointer());
    BOOST_TEST_CHECK(!v->is_map());
    BOOST_TEST_CHECK(!v->is_list());
    BOOST_TEST_CHECK(!v->is_closure());

    BOOST_CHECK_EQUAL(v->s(), "");
    BOOST_CHECK_EQUAL(v->i(), 0);
    BOOST_CHECK_EQUAL(v->d(), 0.0);

    BOOST_CHECK_EQUAL(v, g_vv_undef);

    VV vx(new VariantValue);
    BOOST_TEST_CHECK(vx != g_vv_undef);
}
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(bool_vval)
{
    VV vt(vv_bool(true));
    VV vf(vv_bool(false));

    BOOST_TEST_CHECK(vt->is_defined());
    BOOST_TEST_CHECK(!vt->is_undef());
    BOOST_TEST_CHECK(vt->is_true());
    BOOST_TEST_CHECK(!vt->is_false());
    BOOST_TEST_CHECK(vt->is_boolean());
    BOOST_TEST_CHECK(!vt->is_string());
    BOOST_TEST_CHECK(!vt->is_bytes());
    BOOST_TEST_CHECK(!vt->is_int());
    BOOST_TEST_CHECK(!vt->is_double());
    BOOST_TEST_CHECK(!vt->is_pointer());
    BOOST_TEST_CHECK(!vt->is_map());
    BOOST_TEST_CHECK(!vt->is_list());
    BOOST_TEST_CHECK(!vt->is_closure());

    BOOST_CHECK_EQUAL(vt->s(), "1");
    BOOST_CHECK_EQUAL(vf->s(), "");
    BOOST_CHECK_EQUAL(vt->d(), 1.0);
    BOOST_CHECK_EQUAL(vf->d(), 0.0);
    BOOST_CHECK_EQUAL(vt->i(), 1);
    BOOST_CHECK_EQUAL(vf->i(), 0);

    vt->i_set(0);
    vf->i_set(1);
    BOOST_TEST_CHECK(vt->is_false());
    BOOST_TEST_CHECK(vf->is_true());

    vt->s_set("0");
    vf->s_set("");
    BOOST_TEST_CHECK(vt->is_true());
    BOOST_TEST_CHECK(vf->is_false());
}
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(list_vval)
{
    VV v(vv_list());
    v->push(vv("123"));
    v->push(vv(22.23));
    v->push(vv_bool(true));

    BOOST_TEST_CHECK(v->_(0)->is_string());
    BOOST_CHECK_EQUAL(v->_s(0), "123");
    BOOST_CHECK_EQUAL(v->_d(1), 22.23);
    BOOST_TEST_CHECK(v->_(2)->is_true());

    VV v2(v->clone());

    v->pop();
    BOOST_TEST_CHECK(!v->_(2)->is_true());

    v->unshift(vv("poop"));
    BOOST_TEST_CHECK(v->_(0)->is_string());
    BOOST_TEST_CHECK(v->_(1)->is_string());
    BOOST_CHECK_EQUAL(v->_s(0), "poop");
    BOOST_CHECK_EQUAL(v->_s(1), "123");

    BOOST_CHECK_EQUAL(v2->_s(0), "123");
    BOOST_CHECK_EQUAL(v2->_d(1), 22.23);
    BOOST_TEST_CHECK(v2->_(2)->is_true());

    int d = 0;
    for (auto i : *v)
    {
        if      (d == 0)    BOOST_CHECK_EQUAL(i->s(), "poop");
        else if (d == 1)    BOOST_CHECK_EQUAL(i->s(), "123");
        else if (d == 2)    BOOST_CHECK_EQUAL(i->d(), 22.23);
        d++;
    }
    BOOST_CHECK_EQUAL(d, 3);

    BOOST_CHECK_EQUAL(v->_s("0"), "poop");
    BOOST_CHECK_EQUAL(v->_s("1"), "123");
    BOOST_CHECK_EQUAL(v->_d("2"), 22.23);

    v->set("0", vv("fart"));
    BOOST_CHECK_EQUAL(v->_s(0), "fart");
}
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(map_vval)
{
    VV v(vv_map());

    v->set(0,       vv("BAR"));
    v->set("SXCE",  vv(44.23));
    v->set("99",    vv(1999));
    v->set("MAP",   vv(45));

    BOOST_CHECK_EQUAL(v->_s(0),         "BAR");
    BOOST_CHECK_EQUAL(v->_i(99),        1999);
    BOOST_CHECK_EQUAL(v->_d("SXCE"),    44.23);
    BOOST_CHECK_EQUAL(v->_s("MAP"),     "45");

    int d = 0;
    for (auto i : *v)
    {
        if      (i->_s(0) == "0")    BOOST_CHECK_EQUAL(i->_s(1), "BAR");
        else if (i->_s(0) == "SXCE") BOOST_CHECK_EQUAL(i->_s(1), "44.230000");
        else if (i->_s(0) == "99")   BOOST_CHECK_EQUAL(i->_s(1), "1999");
        else if (i->_s(0) == "MAP")  BOOST_CHECK_EQUAL(i->_s(1), "45");
        d++;
    }
    BOOST_CHECK_EQUAL(d, 4);

    VV cv(v->clone());

    for (auto i : *v)
    {
        BOOST_CHECK_EQUAL(cv->_s(i->_s(0)), i->_s(1));
    }

    cv << vv_kv("SXCE", 123);
    BOOST_CHECK_EQUAL(cv->_i("SXCE"), 123);
    BOOST_CHECK_EQUAL(v->_i("SXCE"),  44);
}
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(int_vval)
{
    VV v(vv(12));

    BOOST_CHECK_EQUAL(v->s(), "12");
    BOOST_CHECK_EQUAL(v->d(), 12.0);
    BOOST_CHECK_EQUAL(v->i(), 12);

    v->s_set("FEIOFEOIW");
    BOOST_CHECK_EQUAL(v->s(), "0");
    BOOST_CHECK_EQUAL(v->i(), 0);
}
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(special_construct_syntax)
{
    VV sv(vv("x"));
    sv << "bla";
    BOOST_CHECK_EQUAL(sv->s(), "bla");
    sv << 1.1234;
    BOOST_CHECK_EQUAL(sv->s(), "1.123400");

    VV iv(vv(12));
    iv << "15";
    BOOST_CHECK_EQUAL(iv->i(), 15);
    iv << 16;
    BOOST_CHECK_EQUAL(iv->i(), 16);

    iv << (int64_t) 9223372036854775807ll;
    BOOST_CHECK_EQUAL(iv->s(), "9223372036854775807");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
    iv << (int64_t) (9223372036854775807ll + 1ll);
#pragma GCC diagnostic pop
    BOOST_CHECK_EQUAL(iv->s(), "-9223372036854775808");

    VV dv(vv(12.32));
    dv << "342.23";
    BOOST_CHECK_EQUAL(dv->d(), 342.23);
    dv << 322.23;
    BOOST_CHECK_EQUAL(dv->d(), 322.23);

    VV mv(vv_map());
    mv << vv_kv("foo", 10)
       << vv_kv("bar", "20");
    BOOST_CHECK_EQUAL(mv->_s("foo"), "10");
    BOOST_CHECK_EQUAL(mv->_i("bar"), 20);

    VV lv(vv_list() << 10 << 20);
    lv << "bar" << 32.3452;
    BOOST_CHECK_EQUAL(lv->_s(0), "10");
    BOOST_CHECK_EQUAL(lv->_s(1), "20");
    BOOST_CHECK_EQUAL(lv->_s(2), "bar");
    BOOST_CHECK_EQUAL(lv->_s(3), "32.345200");
}
//---------------------------------------------------------------------------

VV_CLOSURE(testcls1)
{
    return vv(102 + vv_args->_i(0) * vv_args->_d(1));
}
//---------------------------------------------------------------------------

VV_CLOSURE(testclsstate)
{
    vv_obj->set("foo", vv(vv_args->_i(0) * vv_obj->_d("factor")));
    return vv_obj->_("foo");
}
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(closures_vval)
{
    VV vf1(vvc_new(testcls1)());

    VV r1 = vf1->call(vv_list() << 3 << 5.5);
    BOOST_CHECK_EQUAL(r1->d(), 102 + 16.5);
    BOOST_CHECK_EQUAL(vf1->s(), "102.000000");
    BOOST_CHECK_EQUAL(vf1->i(), 102);

    VV vvo(vv_map() << vv_kv("foo", 10) << vv_kv("factor", 1.5));
    VV vvf2(vvc_new(testclsstate)(vvo));

    VV vr2 = vvf2->call(vv_list() << 120);
    BOOST_CHECK_EQUAL(vvo->_i("foo"), 180);
    BOOST_CHECK_EQUAL(vr2->i(),       180);

    VV vr3 = vvf2->call(vv_list() << 1);
    BOOST_CHECK_EQUAL(vvo->_i("foo"), 1);
    BOOST_CHECK_EQUAL(vr3->d(),       1.5);
}
//---------------------------------------------------------------------------

#ifndef BZVC
BOOST_AUTO_TEST_CASE(dump_serialization)
{
    std::stringstream ss;
    ss << vv(12)                                << ","
       << vv(34.56)                             << ","
       << vv_bool(true)                         << ","
       << vv_bool(false)                        << ","
       << vv_undef();
    BOOST_CHECK_EQUAL(ss.str(), "12,34.560000,true,false,nil");

    ss.str("");
    ss.clear();

    ss << vv("foo \" bar 123!'$&%/()=\r\n§äüß");
    BOOST_CHECK_EQUAL(
        ss.str(),
        "\"foo \\\" bar 123!'$&%/()=\\r\\n"
        "\\xc2\\xa7\\xc3\\xa4\\xc3\\xbc\\xc3\\x9f\"");

    ss.str("");
    ss.clear();
    VV v = vv_list()
         << (vv_list() << 1 << 2 << 3)
         << (vv_map()
             << vv_kv("test", 123)
             << vv_kv("bla", 456)
             << vv_kv("zla", 444)
             << vv_kv("!la", 123)
             << vv_kv("10", 1)
             << vv_kv("1",  2)
             << vv_kv("11", 3)
             << vv_kv("_11@&$", 4)
             << vv_kv("lst", vv_list() << 5 << 6));
    ss << v;
    BOOST_CHECK_EQUAL(ss.str(),
        "[[1 2 3] "
        "{!la: 123 \"1\" 2 \"10\" 1 " "\"11\" 3 _11@&$: 4 "
        "bla: 456 lst: [5 6] test: 123 zla: 444}]");
}
#endif
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(pointer_value)
{
    VV vp(vv_ptr((void *) 0x123, "TestType"));

    BOOST_CHECK_EQUAL(vp->p("TestType"), (void *) 0x123);
    BOOST_CHECK_EQUAL(vp->type(),                 "TestType");
    bool got_exception = false;
    try
    {
        vp->p("FOOBAR");
        got_exception = false;
    }
    catch (VariantValueException &e) { (void) e; got_exception = true; }
    BOOST_TEST_CHECK(got_exception);
    if (sizeof(void *) == 4)
    {
#ifdef __gnu_linux__
        BOOST_CHECK_EQUAL(vp->s(), "#<pointer:TestType:0x123>");
#else
        BOOST_CHECK_EQUAL(vp->s(), "#<pointer:TestType:00000123>");
#endif
    }
    else
    {
        BOOST_CHECK_EQUAL(vp->s(), "#<pointer:TestType:0000000000000123>");
    }

    VV vc = vp->clone();
    BOOST_CHECK_EQUAL(vp->p("TestType"), vc->p("TestType"));
    BOOST_CHECK_EQUAL(vp->type(),        vc->type());

    void *p = vp->P<void *>("TestType");
    BOOST_CHECK_EQUAL(p, (void *) 0x123);
}
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(time_value)
{
    std::time_t t = parse_datetime("2016-09-08 15:15", "%Y-%m-%d %H:%M");
#ifdef __MINGW32__
    BOOST_CHECK_EQUAL(format_datetime(t, "%Y-%m-%d %H:%M:%S"), "2016-09-08 15:15:00");
#else
    BOOST_CHECK_EQUAL(format_datetime(t, "%F %T"), "2016-09-08 15:15:00");
#endif

    VV vd = vv_dt(t);
    BOOST_CHECK_EQUAL(vd->s(), "2016-09-08 15:15:00");
    vd->dt_set(parse_datetime("2016-10-10 15:15", "%Y-%m-%d %H:%M"));
    BOOST_CHECK_EQUAL(vd->s(), "2016-10-10 15:15:00");
    vd->s_set("2016-11-11 11:11:11");
    BOOST_CHECK_EQUAL(vd->dt(), parse_datetime(vd->s(), "%Y-%m-%d %H:%M:%S"));
    BOOST_TEST_CHECK(vd->is_datetime());

    VV vi = vv(120);
    vi->dt_set(t);
    BOOST_CHECK_EQUAL(vi->i(), (int64_t) t);
    BOOST_TEST_CHECK(!vi->is_datetime());
}
//---------------------------------------------------------------------------

const char *json_sample = "{\n"
"   \"results\" : [\n"
"      {\n"
"         \"address_components\" : [\n"
"            {\n"
"               \"long_name\" : \"Hildesheim\",\n"
"               \"short_name\" : \"Hildesheim\",\n"
"               \"types\" : [ \"locality\", \"political\" ]\n"
"            },\n"
"            {\n"
"               \"long_name\" : \"Hildesheim\",\n"
" \"short_name\" : \"HI\",\n"
"               \"types\" : [ \"administrative_area_level_3\", \"political\" ]\n"
" },\n"
"            {\n"
"               \"long_name\" : \"Lower Saxony\",\n"
"               \"short_name\" : \"NDS\",\n"
"               \"types\" : [ \"administrative_area_level_1\", \"political\" ]\n"
"            },\n"
"            {\n"
" \"long_name\" : \"Germany\",\n"
"               \"short_name\" : \"DE\",\n"
"               \"types\" : [ \"country\", \"political\" ]\n"
"            }\n"
"         ],\n"
"         \"formatted_address\" : \"Hildesheim, Germany\",\n"
"         \"geometry\" : {\n"
"            \"bounds\" : {\n"
"               \"northeast\" : {\n"
"                  \"lat\" : 52.1939211,\n"
"                  \"lng\" : 10.042791\n"
"               },\n"
"               \"southwest\" : {\n"
" \"lat\" : 52.0933119,\n"
"                  \"lng\" : 9.8465411\n"
"               }\n"
"            },\n"
" \"location\" : {\n"
"               \"lat\" : 52.154778,\n"
"               \"lng\" : 9.9579652\n"
"            },\n"
"            \"location_type\" : \"APPROXIMATE\",\n"
"            \"viewport\" : {\n"
"               \"northeast\" : {\n"
"                  \"lat\" : 52.1939211,\n"
"                  \"lng\" : 10.042791\n"
"               },\n"
" \"southwest\" : {\n"
"                  \"lat\" : 52.0933119,\n"
"                  \"lng\" : 9.8465411\n"
" }\n"
"            }\n"
"         },\n"
"         \"partial_match\" : true,\n"
"         \"place_id\" : \"ChIJI8rR7qmvukcR0oNx5PXB7o4\",\n"
"         \"types\" : [ \"locality\", \"political\" ]\n"
"      }\n"
"   ],\n"
"   \"status\" : \"OK\"\n"
"}\n"
"";

BOOST_AUTO_TEST_CASE(from_json_test)
{
    VV d = VVal::from_json(json_sample);

    BOOST_CHECK_EQUAL(d->_s("status"), "OK");
    BOOST_TEST_CHECK(d->_("results")->is_list());
    BOOST_CHECK_EQUAL(d->_("results")->_(0)->_("geometry")->_("location")->_d("lat"), 52.154778);
}
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(one_iterator)
{
    VV s(vv("FOOBAR"));

    VV r(vv_undef());
    for (auto i : *s)
    {
        std::cout << "ITER:" << i << std::endl;
        r = i;
    }

    BOOST_CHECK_EQUAL(r->s(), "FOOBAR");
}
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(wstr_handling)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8_utf16_converter;
    std::string s = "/msys32/usr/bin/pwd.exe";
    VV v(vv(s));
    BOOST_CHECK_EQUAL(utf8_utf16_converter.to_bytes(utf8_utf16_converter.from_bytes("/xyz")), "/xyz");
    BOOST_CHECK_EQUAL(vv(v->w())->s(), s);
}
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(hex_conv)
{
    VV s(vv_bytes(std::string("\x00\x01""ABCDEF\xFF", 9)));

    VV x(vv_bytes_from_hex(s->s_hex()));

    BOOST_CHECK_EQUAL(s->s(), x->s());
    BOOST_CHECK_EQUAL("?AB\x01\xFF", vv_bytes_from_hex("XA414201FF")->s());
}
//---------------------------------------------------------------------------

