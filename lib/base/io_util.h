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

#include <cstdio>
#include <cerrno>
#include <cstring>
#include <cstring>
#include <base/vval.h>

namespace io_util
{

class FileCopyWithProgress
{
    private:
        std::mutex  m_files_mutex;
        FILE        *m_in;
        FILE        *m_out;


        void close()
        {
            if (m_in)
            {
                fclose(m_in);
                m_in = nullptr;
            }

            if (m_out)
            {
                fclose(m_out);
                m_out = nullptr;
            }
        }

    public:
        FileCopyWithProgress()
            : m_in(0), m_out(0)
        {
        }

        VVal::VV start(std::function<void(size_t, size_t)> &progress);

        VVal::VV open_in(const VVal::VV &filename)
        {
#ifdef __gnu_linux__
            m_in = fopen(filename->s().c_str(), "rb");
#else
            m_in = _wfopen(filename->w().c_str(), L"rb");
#endif
            if (!m_in)
                return vv_list() << filename << vv(strerror(errno));
            return vv_undef();
        }

        VVal::VV open_out(const VVal::VV &filename)
        {
#ifdef __gnu_linux__
            m_out = fopen(filename->s().c_str(), "wb");
#else
            m_out = _wfopen(filename->w().c_str(), L"wb");
#endif
            if (!m_out)
                return vv_list() << filename << vv(strerror(errno));
            return vv_undef();
        }

        virtual ~FileCopyWithProgress() { close(); }
};

}
