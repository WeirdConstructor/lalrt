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

#ifndef LALRT_LUA_INSTANCE_H
#define LALRT_LUA_INSTANCE_H
#include <mutex>
#include <string>
#include "base/vval.h"
#include "../../lua/src/lua.h"

namespace Lua
{
//---------------------------------------------------------------------------

void push_vv_to_lua(lua_State *L, const VVal::VV &vv);
VVal::VV lua_to_vv(lua_State *L, int index = -1);

//---------------------------------------------------------------------------

#define LUA_REG(luaInstance, sLib, sFunc, vvObj, closureName) \
    (luaInstance).reg(sLib, sFunc, VVC_NEW_##closureName((vvObj)), VVC_DOC_##closureName);
#define LUA_REG_UD(luaInstance, sLib, sFunc, closureName) \
    (luaInstance).reg(sLib, sFunc, VVC_NEW_##closureName(), VVC_DOC_##closureName);

//---------------------------------------------------------------------------

class InstanceException : public std::exception
{
    private:
        std::string m_msg;

    public:
        InstanceException(const std::string &error) : m_msg(error) {}
        virtual ~InstanceException() noexcept {}

        virtual const char *what() const noexcept { return m_msg.c_str(); }
};
//---------------------------------------------------------------------------

class Instance
{
	private:
		std::list<VVal::VV *> m_leaking_refs;

        VVal::VV eval(const std::string &lua_code, const VVal::VV &vv_args, bool is_file = false, std::string code_name = "");

    public:
        std::recursive_mutex m_mutex;
        lua_State           *m_L;

        Instance();

        virtual ~Instance();
        void init_output_interface();

        void doc(const std::string &libname, const std::string &funcname, const std::string &doc_string = "");
        void reg(const std::string &libname, const std::string &funcname, const VVal::VV &vv_func,
                 const std::string &doc_string = "");

        void error(const std::string &place, const std::string &error);

        VVal::VV get_lua_debug_info();

        VVal::VV call(const char *csMethod, const VVal::VV &vv_args);
        VVal::VV eval_code(const std::string &lua_code, const std::string &name = "") { return this->eval(lua_code, VVal::VV(), false, name); }
        VVal::VV eval_file(const std::string &filename) { return this->eval(filename, VVal::VV(), true, filename); }
        VVal::VV eval_code(const std::string &lua_code, const VVal::VV &args, const std::string &name = "") { return this->eval(lua_code, args, false, name); }
        VVal::VV eval_file(const std::string &filename, const VVal::VV &args) { return this->eval(filename, args, true, filename); }
};
//---------------------------------------------------------------------------

struct LuaFunction
{
    lua_State *L;
    int        ref;

    LuaFunction(lua_State *l, int r);
    VVal::VV call(const VVal::VV &args);
    void push();
    ~LuaFunction();
};

//---------------------------------------------------------------------------

} // namespace qlua

#endif // LALRT_LUA_INSTANCE_H
