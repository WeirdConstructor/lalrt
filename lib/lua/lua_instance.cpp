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

#include "rt/log.h"
#include "lua_instance.h"
#include "../../lua/src/lauxlib.h"
#include "../../lua/src/lualib.h"
#include "../../lua/src/lobject.h"
#include "../../lua/src/lstate.h"
#include "../../lua/src/lua_embed_helper.h"
#include <iostream>
#include <sstream>
#include <boost/format.hpp>

using namespace std;
using namespace VVal;
using boost::format;

namespace Lua
{
//---------------------------------------------------------------------------

class ConsoleLuaOutput : public lua_embed_helper::Output
{
    private:
        stringstream   m_buffer;

    public:
        ConsoleLuaOutput() { }

        virtual ~ConsoleLuaOutput() { }

        virtual void write(const char *csData, size_t len)
        {
            m_buffer << string(csData, len);
        }

        virtual void flush_error()
        {
            cerr << m_buffer.str() << endl;
            m_buffer.str(string());
            m_buffer.clear();
        }

        virtual void flush_print()
        {
            cout << m_buffer.str() << endl;
            m_buffer.str(string());
            m_buffer.clear();
        }
};
//---------------------------------------------------------------------------

static int push_vv_as_lua_args(lua_State *m_L, const VV &vv_args)
{
    int iArgs = 0;
    for (auto i : *vv_args)
    {
        push_vv_to_lua(m_L, i);
        iArgs++;
    }
    return iArgs;
}
//---------------------------------------------------------------------------

std::string lua_to_string(lua_State *L, int index)
{
    size_t l = 0;
    const char *s = luaL_tolstring(L, index, &l);
    lua_pop(L, 1);
    return s ? string(s, (int) l) : string();
}
//---------------------------------------------------------------------------

static VV pop_lua_results_as_vv(lua_State *m_L, int iResults)
{
    VV vv_ret;
    if (iResults == 1)
    {
        vv_ret = lua_to_vv(m_L);
        lua_pop(m_L, 1);
    }
    else if (iResults == 0)
        vv_ret = vv_undef();
    else
    {
        vv_ret = vv_list();
        for (int i = 0; i < iResults; i++)
        {
            vv_ret->unshift(lua_to_vv(m_L));
            lua_pop(m_L, 1);
        }
    }

    return vv_ret;
}
//---------------------------------------------------------------------------

VV call_lua_fun_on_stack(lua_State *L, const VV &vv_args,
                         const std::string &function_name)
{
    int iArgs = push_vv_as_lua_args(L, vv_args);
    int iTop  = lua_gettop(L) - (iArgs + 1);
    int iErr  = lua_pcall(L, iArgs, LUA_MULTRET, 0);
    if (iErr)
    {
        std::string err =
            (format("Error while evaluating lua function lua %1%: %2%\n")
                % string(function_name)
                % lua_to_string(L, -1)).str();
        L_ERROR << err;
        cerr << err << endl;
        lua_pop(L, 1);
        return vv_undef();
    }

    int iResults = lua_gettop(L) - iTop;
    VV ret = pop_lua_results_as_vv(L, iResults);
    return ret;
}
//---------------------------------------------------------------------------

void push_vv_to_lua(lua_State *L, const VV &vv)
{
    if      (vv->is_int()
             || vv->is_datetime())  lua_pushinteger(L, vv->i());
    else if (vv->is_double())       lua_pushnumber (L, vv->d());
    else if (vv->is_boolean())      lua_pushboolean(L, vv->b());
    else if (vv->is_string())
    {
        string tmp = vv->s();
        lua_pushlstring(L, tmp.data(), tmp.size());
    }
    else if (vv->is_bytes())
    {
        string tmp = "\xFF" + vv->s();
        lua_pushlstring(L, tmp.data(), tmp.size());
    }
    else if (vv->is_pointer())
    {
        if (vv->type() == "LuaFunction")
        {
            LuaFunction *f = (LuaFunction *) vv->p("LuaFunction");
            f->push();
        }
        else
        {
            void **ptr = (void **) lua_newuserdata(L, sizeof(void *));
            *ptr = vv->p(vv->type());
            string type = vv->type();
            lua_pushlstring(L, type.data(), type.size());
            lua_setuservalue(L, -2);
        }
    }
    else if (vv->is_map())
    {
        lua_createtable(L, 0, vv->size());
        for (auto i : *vv)
        {
            push_vv_to_lua(L, i->_(0));
            push_vv_to_lua(L, i->_(1));
            lua_rawset(L, -3);
        }
    }
    else if (vv->is_list())
    {
        lua_createtable(L, vv->size(), 0);
        int i = 1;
        for (auto it : *vv)
        {
            push_vv_to_lua(L, it);
            lua_rawseti(L, -2, i++);
        }
    }
    else lua_pushnil(L);
}
//---------------------------------------------------------------------------

LuaFunction::LuaFunction(lua_State *l, int r)
    : L(l), ref(r)
{
}
//---------------------------------------------------------------------------

void LuaFunction::push()
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
}
//---------------------------------------------------------------------------

VVal::VV LuaFunction::call(const VVal::VV &args)
{
    this->push();
    return call_lua_fun_on_stack(L, args, "anonymous closure");
}
//---------------------------------------------------------------------------

LuaFunction::~LuaFunction()
{
    luaL_unref(L, LUA_REGISTRYINDEX, ref);
}
//---------------------------------------------------------------------------

VV lua_to_vv(lua_State *L, int index)
{
    switch (lua_type(L, index))
    {
        case LUA_TNIL:           return vv_undef();
        case LUA_TBOOLEAN:       return vv(lua_toboolean (L, index) != 0);
        case LUA_TNUMBER:
            if (lua_isinteger(L, index))
                return vv((int64_t) lua_tointeger (L, index));
            else
                return vv((double)  lua_tonumber  (L, index));
        case LUA_TSTRING:
        {
            const char *cstr = lua_tostring(L, index);
            int            l = (int) lua_rawlen(L, index);
            if (l > 0 && cstr[0] == '\xFF')
                return vv_bytes(std::string(cstr + 1, l - 1));
            else
                return vv(std::string(cstr, l));
        }
        case LUA_TLIGHTUSERDATA:
        {
            return vv_ptr(lua_touserdata(L, index), "");
        }
        case LUA_TUSERDATA:
        {
            void **ptr = (void **) lua_touserdata(L, index);

            lua_getuservalue(L, index);
            string type = lua_to_string(L, -1);
            lua_pop(L, 1);

            return vv_ptr(*ptr, type);
        }
        case LUA_TTABLE:
        {
            lua_len(L, index);
            int len = (int) lua_tonumber(L, -1);
            lua_pop(L, 1); // pops lua_len

            bool init_vv = false;
            VV table_data;

            lua_pushvalue(L, index);
            lua_pushnil(L);
            while (lua_next(L, -2) != 0)
            {
                VV v = lua_to_vv(L, -1);
                lua_pop(L, 1); // pops lua_to_vv

                if (!init_vv)
                {
                    init_vv = true;
                    if (lua_type(L, -1) != LUA_TNUMBER || len <= 0)
                        table_data = vv_map();
                    else
                        table_data = vv_list();
                }

                if (lua_type(L, -1) == LUA_TNUMBER)
                    table_data->set((int32_t) lua_tonumber(L, -1) - 1, v);
                else
                    table_data->set(lua_to_string(L, -1), v);
            }
            lua_pop(L, 1); // pop table

            if (!init_vv)
                table_data = vv_list();

            return table_data;
        }
        case LUA_TFUNCTION:
        {
            int ref = luaL_ref(L, LUA_REGISTRYINDEX);
            lua_pushnil(L);
            LuaFunction *lf = new LuaFunction(L, ref);
            return
                vv_ptr((void *) lf, "LuaFunction",
                    [L](void *fun) {
                        LuaFunction *sfun = (LuaFunction *) fun;
                        delete sfun;
                    }, true);
        }
        default:
            return vv(lua_to_string(L, index));
    }
    return vv_undef();
}
//---------------------------------------------------------------------------

static int lua_vv_closure_caller(lua_State *L)
{
    int n = lua_gettop(L);
    VV vv_args = vv_list();
    for (int i = 1; i <= n; i++)
        vv_args << lua_to_vv(L, i);

    Instance *liLua        = (Instance *) lua_touserdata(L, lua_upvalueindex(1));
    const char *csFullName =              lua_tostring  (L, lua_upvalueindex(2));
    VV *vv_leaking_ref     = (VV *)       lua_touserdata(L, lua_upvalueindex(3));

    if (!(*vv_leaking_ref)->is_closure())
    {
        push_vv_to_lua(L, vv_undef());
        return 1;
    }

    VV vv_ret;
    try
    {
        vv_ret = (*vv_leaking_ref)->call(vv_args);
    }
    catch (const std::exception &e)
    {
        L_ERROR << "C++ Exception caught in '"
                << csFullName << "' by Lua->C caller: "
                << e.what() << ", args=" << vv_args;
        liLua->error(csFullName, string("C++ Exception: ") + e.what());
    }
    push_vv_to_lua(L, vv_ret);
    VV x = lua_to_vv(L, -1);
    return 1;
}
//---------------------------------------------------------------------------

void Instance::init_output_interface()
{
    m_L->embedOutput = new ConsoleLuaOutput;
}
//---------------------------------------------------------------------------

Instance::Instance() : m_L(luaL_newstate())
{
    luaL_openlibs(m_L);
}
//---------------------------------------------------------------------------

Instance::~Instance()
{
    if (m_L->embedOutput)
        delete m_L->embedOutput;
    if (m_L) lua_close(m_L);

    for (auto i : m_leaking_refs)
        delete i;
}
//---------------------------------------------------------------------------

void Instance::error(const std::string &place, const std::string &error)
{
    luaL_error(m_L, "Error in %s: %s\n", place.c_str(), error.c_str());
}
//---------------------------------------------------------------------------

void Instance::doc(const string &libname, const string &funcname, const string &doc_string)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    lua_pushglobaltable(m_L);
    luaL_getsubtable(m_L, -1, (libname + "_doc").c_str());
    lua_pushstring(m_L, doc_string.c_str());
    lua_setfield(m_L, -2, funcname.c_str());
    lua_pop(m_L, 2);
}
//---------------------------------------------------------------------------

void Instance::reg(const std::string &libname, const std::string &funcname, const VV &vv_func, const std::string &doc_string)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    lua_pushglobaltable(m_L);                   // 1 glob table
    luaL_getsubtable(m_L, -1, libname.c_str()); // 2 sub table

    string full_func_name = libname + "." + funcname;
    VV *vv_leaking_ref(new VV);
    *vv_leaking_ref = vv_func;
    m_leaking_refs.push_back(vv_leaking_ref);

    lua_pushlightuserdata(m_L, this);
    lua_pushstring       (m_L, full_func_name.c_str());
    lua_pushlightuserdata(m_L, vv_leaking_ref);
    lua_pushcclosure(m_L, &lua_vv_closure_caller, 3); // 3 c closure

    lua_setfield(m_L, -2, funcname.c_str());    // 1 and 2 still onstack

    lua_pop(m_L, 2);

    if (doc_string != "")
        this->doc(libname, funcname, doc_string);
}
//---------------------------------------------------------------------------

VV Instance::call(const char *function_name, const VV &vv_args)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    lua_getglobal(m_L, function_name);
    return call_lua_fun_on_stack(m_L, vv_args, function_name);
}
//---------------------------------------------------------------------------

VV Instance::eval(const string &lua_code, const VV &vv_args, bool bEvalFile, string code_name)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (code_name.empty()) code_name = lua_code;

    if (bEvalFile)
    {
        int err_code = luaL_loadfile(m_L, lua_code.c_str());
        if (err_code)
        {
            std::string err = lua_to_string(m_L, -1);
            lua_pop(m_L, 1);
            throw InstanceException(
                (format("Error while compiling lua file [%1%]: %2%\n")
                        % code_name
                        % err).str());
            return vv_undef();
        }
    }
    else
    {
        int err_code = luaL_loadbufferx(m_L, lua_code.c_str(),
                                        lua_code.size(),
                                        code_name.c_str(), "bt");
        if (err_code)
        {
            std::string err = lua_to_string(m_L, -1);
            lua_pop(m_L, 1);
            throw InstanceException(
                (format("Error while compiling lua code [%1%]: %2%\n")
                    % code_name // TODO: take only first 30 chars of it!
                    % err).str());
            return vv_undef();
        }
    }

    int arg_count = 0;
    if (vv_args) arg_count = push_vv_as_lua_args(m_L, vv_args);
    else         arg_count = push_vv_as_lua_args(m_L, vv_undef());
    int stack_top = lua_gettop(m_L) - (arg_count + 1);
    int err_code  = lua_pcall(m_L, arg_count, LUA_MULTRET, 0);
    if (err_code)
    {
        std::string err = lua_to_string(m_L, -1);
        lua_pop(m_L, 1);
        throw InstanceException(
                (format("Error while evaluating lua code [%1%]: %2%\n")
                % code_name // TODO: take only first 30 chars of it!
                % err).str());
        return vv_undef();
    }

    int iResults = lua_gettop(m_L) - stack_top;
    VV ret = pop_lua_results_as_vv(m_L, iResults);
    return ret;
}
//---------------------------------------------------------------------------

VV Instance::get_lua_debug_info()
{
    lua_Debug ar;
    if (!lua_getstack(m_L, 1, &ar))
        return vv_undef();

    lua_getinfo(m_L, "nSl", &ar);

    VV vv_ret = vv_list()
        << (ar.name ? string(ar.name) : string())
        << string(ar.short_src)
        << ar.currentline;
    return vv_ret;
}
//---------------------------------------------------------------------------

} // namespace qlua
