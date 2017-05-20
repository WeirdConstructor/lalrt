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

#ifndef PROTO_FRAMING_H
#define PROTO_FRAMING_H

#include "utf8buffer.h"

namespace msgpack
{

class ProtoFraming
{
    private:
        UTF8Buffer m_u8RecvBuffer;
        UTF8Buffer *m_u8SendBuffer;

    public:
        ProtoFraming(UTF8Buffer *u8SendBuffer = 0)
            : m_u8SendBuffer(u8SendBuffer)
        { }
        virtual ~ProtoFraming() { }

        void send_msg(const char *csData, size_t iLen, uint16_t iType)
        {
            if (!m_u8SendBuffer)
            {
                UTF8Buffer u8Tmp;
                u8Tmp.append_uint32((uint32_t) iLen);
                u8Tmp.append_uint16(iType);
                u8Tmp.append_bytes(csData, iLen);
                this->handle_recv(u8Tmp.buffer(), u8Tmp.length());
                return;
            }

            m_u8SendBuffer->append_uint32((uint32_t) iLen);
            m_u8SendBuffer->append_uint16(iType);
            m_u8SendBuffer->append_bytes(csData, iLen);
        }

        void handle_recv(const char *csData, size_t iLen)
        {
            m_u8RecvBuffer.append_bytes(csData, iLen);

            uint32_t iPktLen = 0;
            uint16_t iPktType = 0;
            bool bCompletedPacket = true;

            while (bCompletedPacket)
            {
                bCompletedPacket = false;
                if (!m_u8RecvBuffer.read_uint32(iPktLen, false))
                    return;

                if (!m_u8RecvBuffer.read_uint16(iPktType, false))
                    return;

                if (m_u8RecvBuffer.length () < iPktLen)
                    return;

                m_u8RecvBuffer.skip_bytes(6);
                this->handle_recv_msg(m_u8RecvBuffer.buffer(), iPktLen, iPktType);
                m_u8RecvBuffer.skip_bytes(iPktLen);
                bCompletedPacket = true;
            }
        }

        virtual void handle_recv_msg(const char *csPacketData, size_t iLen, uint16_t iType)
        {
            (void) csPacketData;
            (void) iLen;
            (void) iType;
            // subclass!
        }
};

} // namespace msgpack

#endif // PROTO_FRAMING_H
