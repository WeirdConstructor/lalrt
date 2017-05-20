#include "rt/module_init_defs.h"
#include <string>

class QtThreadException : public std::exception
{
    private:
        std::string m_msg;

    public:
        QtThreadException(const std::string &s) { m_msg = s; }
        virtual ~QtThreadException() noexcept {}

        virtual const char *what() const noexcept { return m_msg.c_str(); }
};

void init_qtlib(lal_rt::LuaThread *t, Lua::Instance &lua);
