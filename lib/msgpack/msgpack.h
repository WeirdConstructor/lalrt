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

#ifndef QLMsgPackH
#define QLMsgPackH 1
#include "utf8buffer.h"

namespace msgpack
{
//---------------------------------------------------------------------------

class Serializer
{
    private:
        UTF8Buffer m_u8Buf;

    public:
        void reset() { m_u8Buf.reset(); }


        void nil() { m_u8Buf.append_byte((char) 0xc0); }
        void boolean(bool bValue) { m_u8Buf.append_byte((char) (bValue ? 0xc3 : 0xc2)); }

        void number(float f) { m_u8Buf.append_byte((char) 0xca); m_u8Buf.append_float(f); }

        void number(double f) { m_u8Buf.append_byte((char) 0xcb); m_u8Buf.append_double(f); }

    private:
        void number(uint64_t i)
        {
            if (i <= UINT8_MAX)
            {
                number((uint8_t) i);
                return;
            }
            else if (i <= UINT16_MAX)
            {
                number((uint16_t) i);
                return;
            }
            else if (i <= UINT32_MAX)
            {
                number((uint32_t) i);
                return;
            }

            m_u8Buf.append_byte((char) 0xcf);
            m_u8Buf.append_uint64(i);
        }

        void number(uint32_t i)
        {
            if (i <= UINT8_MAX)
            {
                number((uint8_t) i);
                return;
            }
            else if (i <= UINT16_MAX)
            {
                number((uint16_t) i);
                return;
            }

            m_u8Buf.append_byte((char) 0xce);
            m_u8Buf.append_uint32(i);
        }

        void number(uint16_t i)
        {
            if (i <= UINT8_MAX)
            {
                number((uint8_t) i);
                return;
            }

            m_u8Buf.append_byte((char) 0xcd);
            m_u8Buf.append_uint16(i);
        }

        void number(uint8_t i)
        {
            if ((i & 0x7f) == i)
            {
                m_u8Buf.append_byte((char) (0x00 ^ i));
                return;
            }

            m_u8Buf.append_byte((char) 0xcc);
            m_u8Buf.append_bytes((char *) &i, 1);
        }

    public:
        void number(int64_t i)
        {
            if (i >= INT8_MIN && i <= INT8_MAX)
            {
                if (i >= 0)
                    number((uint8_t) i);
                else
                    number((int8_t) i);
                return;
            }
            else if (i >= INT16_MIN && i <= INT16_MAX)
            {
                if (i >= 0)
                    number((uint16_t) i);
                else
                    number((int16_t) i);
                return;
            }
            else if (i >= INT32_MIN && i <= INT32_MAX)
            {
                if (i >= 0)
                    number((uint32_t) i);
                else
                    number((int32_t) i);
                return;
            }
            else if (i > 0)
            {
                number((uint64_t) i);
                return;
            }

            m_u8Buf.append_byte((char) 0xd3);
            m_u8Buf.append_uint64((uint64_t) i);
        }

    private:
        void number(int32_t i)
        {
            if (i >= INT8_MIN && i <= INT8_MAX)
            {
                number((int8_t) i);
                return;
            }
            else if (i >= INT16_MIN && i <= INT16_MAX)
            {
                number((int16_t) i);
                return;
            }

            m_u8Buf.append_byte((char) 0xd2);
            m_u8Buf.append_uint32((uint32_t) i);
        }

        void number(int16_t i)
        {
            if (i >= INT8_MIN && i <= INT8_MAX)
            {
                number((int8_t) i);
                return;
            }

            m_u8Buf.append_byte((char) 0xd1);
            m_u8Buf.append_uint16((uint16_t) i);
        }

        void number(int8_t i)
        {
            if (i >= 0 && (i & 0x7f) == i)
            {
                m_u8Buf.append_byte((char) (0x00 | i));
                return;
            }
            else if (i < 0 && i >= -32)
            {
                m_u8Buf.append_byte((char) (0xe0 | (0x1F & i)));
                return;
            }

            m_u8Buf.append_byte((char) 0xd0);
            m_u8Buf.append_bytes((char *) &i, 1);
        }
    public:

        void string(const std::string &s, bool bIsBinary = false)
        {
            if (!bIsBinary && s.size() <= 0x1F)
            {
                m_u8Buf.append_byte((char) (0xa0 | (uint8_t) s.size()));
                m_u8Buf.append_bytes(s.data(), s.size());
            }
            else if (s.size() <= 0xFF)
            {
                m_u8Buf.append_byte((char) (bIsBinary ? 0xc4 : 0xd9));
                m_u8Buf.append_uint8((uint8_t) s.size());
                m_u8Buf.append_bytes(s.data(), s.size());
            }
            else if (s.size() <= 0xFFFF)
            {
                m_u8Buf.append_byte((char) (bIsBinary ? 0xc5 : 0xda));
                m_u8Buf.append_uint16((uint16_t) s.size());
                m_u8Buf.append_bytes(s.data(), s.size());
            }
            else
            {
                m_u8Buf.append_byte((char) (bIsBinary ? 0xc6 : 0xdb));
                m_u8Buf.append_uint32((uint32_t) s.size());
                // XXX: will trim string to 2^32 bytes:
                m_u8Buf.append_bytes(s.data(), s.size());
            }
        }

        void array(uint32_t iElemCnt)
        {
            if (iElemCnt <= 0x0F)
                m_u8Buf.append_byte((char) (0x90 | (uint8_t) iElemCnt));
            else if (iElemCnt <= 0xFFFF)
            {
                m_u8Buf.append_byte((char) 0xdc);
                m_u8Buf.append_uint16(iElemCnt);
            }
            else
            {
                m_u8Buf.append_byte((char) 0xdd);
                m_u8Buf.append_uint32(iElemCnt);
            }
        }

        void map(uint32_t iElemCnt)
        {
            if (iElemCnt <= 0x0F)
                m_u8Buf.append_byte((char) (0x80 | (uint8_t) iElemCnt));
            else if (iElemCnt <= 0xFFFF)
            {
                m_u8Buf.append_byte((char) 0xde);
                m_u8Buf.append_uint16(iElemCnt);
            }
            else
            {
                m_u8Buf.append_byte((char) 0xdf);
                m_u8Buf.append_uint32(iElemCnt);
            }
        }

        std::string asString() { return m_u8Buf.as_string(); }
};
//---------------------------------------------------------------------------

class Deserializer
{
    private:
        UTF8Buffer *m_u8Buf;

    public:
        Deserializer() : m_u8Buf(0) { }
        virtual ~Deserializer() { }

        virtual void onValueBoolean(bool bValue)    { (void) bValue; }
        virtual void onValueNil()                   { }
        virtual void onValueFloat(double fNum, bool bIsDouble) { (void) fNum; (void) bIsDouble; }
        virtual void onValueUInt(uint64_t uiNum)    { (void) uiNum; }
        virtual void onValueInt(int64_t iNum)       { (void) iNum; }
        virtual void onValueString(const std::string &sString, bool bIsBinary) { (void) sString; (void) bIsBinary; }

        virtual void onObjectStart(unsigned int iElemCnt) { (void) iElemCnt; }
        virtual void onObjectEnd()                        { }

        virtual void onArrayStart(unsigned int iElemCnt)  { (void) iElemCnt; }
        virtual void onArrayEnd()                         { }

        virtual void onError(UTF8Buffer *u8Buf, const char *csError) { (void) u8Buf; (void) csError; }

        bool parseFloat(bool bIsDouble)
        {
            m_u8Buf->skip_bytes(1);
            if (bIsDouble)
            {
                double d = 0.0;
                if (!m_u8Buf->read_double(d))
                {
                    this->onError(m_u8Buf, "EOF while reading double");
                    return false;
                }
                this->onValueFloat(d, true);
                return true;
            }
            else
            {
                float f = 0.0;
                if (!m_u8Buf->read_float(f))
                {
                    this->onError(m_u8Buf, "EOF while reading float");
                    return false;
                }
                this->onValueFloat((double) f, false);
                return true;
            }
        }

        bool parseLength(int iLenBytes, uint64_t &iLen, uint8_t iLen0Mask = 0xFF)
        {
            if (iLenBytes == 0)
            {
                uint8_t i = m_u8Buf->first_byte(true);
                iLen = i & ~iLen0Mask;
            }
            else if (iLenBytes == 1)
            {
                m_u8Buf->skip_bytes(1);
                uint8_t i;
                if (!m_u8Buf->read_uint8(i))
                {
                    this->onError(m_u8Buf, "EOF while reading uint8");
                    return false;
                }

                iLen = i;
            }
            else if (iLenBytes == 2)
            {
                m_u8Buf->skip_bytes(1);
                uint16_t i;
                if (!m_u8Buf->read_uint16(i))
                {
                    this->onError(m_u8Buf, "EOF while reading uint16");
                    return false;
                }

                iLen = i;
            }
            else if (iLenBytes == 4)
            {
                m_u8Buf->skip_bytes(1);
                uint32_t i;
                if (!m_u8Buf->read_uint32(i))
                {
                    this->onError(m_u8Buf, "EOF while reading uint32");
                    return false;
                }

                iLen = i;
            }
            else if (iLenBytes == 8)
            {
                m_u8Buf->skip_bytes(1);
                if (!m_u8Buf->read_uint64(iLen))
                {
                    this->onError(m_u8Buf, "EOF while reading uint64");
                    return false;
                }
            }

            return true;
        }

        bool parseUInt(int iLenBytes)
        {
            uint64_t i = 0;
            if (!this->parseLength(iLenBytes, i)) return false;
            this->onValueUInt(i);
            return true;
        }

        bool parseInt(int iLenBytes)
        {
            uint64_t iOut = 0;

            if (iLenBytes == 1)
            {
                m_u8Buf->skip_bytes(1);
                uint8_t i;
                if (!m_u8Buf->read_uint8(i))
                {
                    this->onError(m_u8Buf, "EOF while reading int8");
                    return false;
                }

                iOut = (int8_t) i;
            }
            else if (iLenBytes == 2)
            {
                m_u8Buf->skip_bytes(1);
                uint16_t i;
                if (!m_u8Buf->read_uint16(i))
                {
                    this->onError(m_u8Buf, "EOF while reading int16");
                    return false;
                }

                iOut = (int16_t) i;
            }
            else if (iLenBytes == 4)
            {
                m_u8Buf->skip_bytes(1);
                uint32_t i;
                if (!m_u8Buf->read_uint32(i))
                {
                    this->onError(m_u8Buf, "EOF while reading int32");
                    return false;
                }

                iOut = (int32_t) i;
            }
            else if (iLenBytes == 8)
            {
                m_u8Buf->skip_bytes(1);
                if (!m_u8Buf->read_uint64(iOut))
                {
                    this->onError(m_u8Buf, "EOF while reading int64");
                    return false;
                }
            }

            this->onValueInt(iOut);
            return true;
        }

        bool parseFixInt(bool bIsNegative)
        {
            uint8_t i = m_u8Buf->first_byte(true);
            if (bIsNegative)
            {
                this->onValueInt((int8_t) i);
                return true;
            }
            else
            {
                this->onValueUInt(i);
                return true;
            }
        }

        bool parseString(int iLenBytes, bool bIsBinary)
        {
            uint64_t iLen = 0;
            if (!this->parseLength(iLenBytes, iLen, 0xa0)) return false;

            if (m_u8Buf->length() < iLen)
            {
                this->onError(m_u8Buf, "EOF while reading string");
                return false;
            }

            std::string s(m_u8Buf->buffer(), iLen);
            this->onValueString(s, bIsBinary);
            m_u8Buf->skip_bytes(iLen);

            return true;
        }

        bool skipFixExt(int iLen)
        {
            m_u8Buf->skip_bytes(iLen + 1);

            this->onValueNil();
            return true;
        }

        bool skipExt(int iLenBytes)
        {
            uint64_t iLen = 0;
            if (!this->parseLength(iLenBytes, iLen)) return false;

            iLen++; // + 1 type byte

            if (m_u8Buf->length() < iLen)
            {
                this->onError(m_u8Buf, "EOF while reading string");
                return false;
            }

            m_u8Buf->skip_bytes(iLen);

            this->onValueNil();
            return true;
        }

        bool parseArray(int iLenBytes)
        {
            uint64_t iLen = 0;
            if (!this->parseLength(iLenBytes, iLen, 0x90)) return false;

            this->onArrayStart((unsigned int) iLen);
            for (uint64_t i = 0; i < iLen; i++)
            {
                if (!this->parse(0))
                {
                    this->onError(m_u8Buf, "Couldn't parse array element.");
                    return false;
                }
            }
            this->onArrayEnd();
            return true;
        }

        bool parseMap(int iLenBytes)
        {
            uint64_t iLen = 0;
            if (!this->parseLength(iLenBytes, iLen, 0x80)) return false;

            this->onObjectStart((unsigned int) iLen);
            for (uint64_t i = 0; i < iLen; i++)
            {
                if (!this->parse(0))
                {
                    this->onError(m_u8Buf, "Couldn't parse object key.");
                    return false;
                }

                if (!this->parse(0))
                {
                    this->onError(m_u8Buf, "Couldn't parse object value.");
                    return false;
                }
            }
            this->onObjectEnd();
            return true;
        }

        bool parse(UTF8Buffer *u8Buf)
        {
            if (u8Buf)
                m_u8Buf = u8Buf;

            if (!m_u8Buf)
                return false;

            unsigned char c = m_u8Buf->first_byte();
            switch (c)
            {
                case 0xc0: m_u8Buf->skip_bytes(1); this->onValueNil();          return true;
                case 0xc2: m_u8Buf->skip_bytes(1); this->onValueBoolean(false); return true;
                case 0xc3: m_u8Buf->skip_bytes(1); this->onValueBoolean(true);  return true;
                case 0xc4: return this->parseString(1, true);
                case 0xc5: return this->parseString(2, true);
                case 0xc6: return this->parseString(4, true);
                case 0xc7: return this->skipExt(1);
                case 0xc8: return this->skipExt(2);
                case 0xc9: return this->skipExt(4);
                case 0xca: return this->parseFloat(false);
                case 0xcb: return this->parseFloat(true);
                case 0xcc: return this->parseUInt(1);
                case 0xcd: return this->parseUInt(2);
                case 0xce: return this->parseUInt(4);
                case 0xcf: return this->parseUInt(8);
                case 0xd0: return this->parseInt(1);
                case 0xd1: return this->parseInt(2);
                case 0xd2: return this->parseInt(4);
                case 0xd3: return this->parseInt(8);
                case 0xd4: return this->skipFixExt(1);
                case 0xd5: return this->skipFixExt(2);
                case 0xd6: return this->skipFixExt(4);
                case 0xd7: return this->skipFixExt(8);
                case 0xd8: return this->skipFixExt(16);
                case 0xda: return this->parseString(2, false);
                case 0xd9: return this->parseString(1, false);
                case 0xdb: return this->parseString(4, false);
                case 0xdc: return this->parseArray(2);
                case 0xdd: return this->parseArray(4);
                case 0xde: return this->parseMap(2);
                case 0xdf: return this->parseMap(4);
            }

            if      (c <= 0x7f)              return this->parseFixInt(false);
            else if (c >= 0x80 && c <= 0x8f) return this->parseMap(0);
            else if (c >= 0x90 && c <= 0x9f) return this->parseArray(0);
            else if (c >= 0xa0 && c <= 0xbf) return this->parseString(0, false);
            else if (c >= 0xe0)              return this->parseFixInt(true);
            else
            {
                this->onError(m_u8Buf, "invalid format specifier found");
                return false;
            }
        }
};
//---------------------------------------------------------------------------

};

#endif // QLMsgPackH
