#include "utf8buffer.h"

namespace micro_lisp
{

//---------------------------------------------------------------------------

inline bool charClass(char c, const char *cs, bool bInvert = false)
{
    if (bInvert)
    {
        for (unsigned int i = 0; i < strlen(cs); i++)
            if (cs[i] != c) return true;
    }
    else
    {
        for (unsigned int i = 0; i < strlen(cs); i++)
            if (cs[i] == c) return true;
    }
    return false;
}
//---------------------------------------------------------------------------

inline bool u8BufParseNumber(UTF8Buffer &u8P, double &dVal, int64_t &iVal,
                             bool &bIsDouble, bool &bBadNumber)
{
    try
    {
        if (!charClass(u8P.first_byte(), "+-0123456789"))
            return false;

        int iBase = 10;
        bool bNeg = u8P.first_byte() == '-';
        if (charClass(u8P.first_byte(), "+-"))
            u8P.skip_bytes(1);

        bIsDouble = false;

        UTF8Buffer u8Num;
        while (u8P.length() > 0 && charClass(u8P.first_byte(), "0123456789"))
            u8Num.append_byte(u8P.first_byte(true));

        if (u8P.length() > 0 && u8P.first_byte() == 'r')
        {
            u8P.skip_bytes(1);

            iBase = u8Num.length() > 0 ? stoi(u8Num.as_string()) : 10;
            u8Num.reset();

            while (u8P.length() > 0
                   && charClass(u8P.first_byte(),
                                "0123456789abcdefghijklmnopqrstuvwxyz"
                                "ABCDEFGHIJKLMNOPQRSTUVW"))
                u8Num.append_byte(u8P.first_byte(true));
        }

        while (u8P.length() > 0)
        {
            if (!charClass(u8P.first_byte(), "0123456789"))
                bIsDouble = true;

            u8Num.append_byte(u8P.first_byte(true));
        }

        if (bIsDouble)
        {
            if (iBase != 10) return bBadNumber = true;
            dVal = (bNeg ? -1LL : 1LL) * stod(u8Num.as_string());
        }
        else
        {
            if (iBase < 2 || iBase > 36) return bBadNumber = true;
            iVal = (bNeg ? -1LL : 1LL) * stoll(u8Num.as_string(), 0, iBase);
        }
    }
    catch (const std::exception &)
    {
        return bBadNumber = true;
    }

    return true;
}
//---------------------------------------------------------------------------

inline std::string printString(const std::string &sValue)
{
    std::string os;

    int iSize = sValue.size();
    os.reserve(iSize + 2);

    os.append(1, '"');
    for (int i = 0; i < iSize; i++)
    {
        if (sValue[i] == '"' || sValue[i] == '\\')
            os.append(1, '\\');
        os.append(1, sValue[i]);
    }
    os.append(1, '"');

    return os;
}
//---------------------------------------------------------------------------

enum TokenType
{
    T_EOF,
    T_CHR_TOK,
    T_STR_BODY,
    T_DBL,
    T_INT,
    T_BAD_NUM
};
//---------------------------------------------------------------------------

struct Token
{
    TokenType       m_iTokenID;
    std::string     m_sText;
    int             m_iLine;
    union {
        double  d;
        int64_t i;
    } m_num;


    Token() : m_iLine(0), m_iTokenID(T_EOF), m_sText("") { }

    Token(double d)  : m_iLine(0), m_iTokenID(T_DBL) { m_num.d = d; }
    Token(int64_t i) : m_iLine(0), m_iTokenID(T_INT) { m_num.i = i; }

    Token(char c) : m_iLine(0), m_iTokenID(T_CHR_TOK)
    {
        m_sText = std::string(&c, 1);
    }
    Token(const char *csStr) : m_iLine(0), m_iTokenID(T_CHR_TOK)
    {
        m_sText = std::string(csStr);
    }
    Token(UTF8Buffer &u8Tmp) : m_iLine(0), m_iTokenID(T_CHR_TOK)
    {
        m_sText = u8Tmp.as_string();
    }
    Token(const std::string &sStrBody)
        : m_iLine(0), m_iTokenID(T_STR_BODY)
    {
        m_sText = sStrBody;
    }

    char nth(unsigned int i)
    {
        if (m_sText.size() <= i) return '\0';
        return m_sText[i];
    }

    std::string dump()
    {
        std::string s;

        if (m_iTokenID == T_DBL)
            s = "T_DBL=" + std::to_string(m_num.d);
        else if (m_iTokenID == T_INT)
            s = "T_INT=" + std::to_string(m_num.i);
        else if (m_iTokenID == T_EOF)
            s = "T_EOF";
        else if (m_iTokenID == T_CHR_TOK)
            s = "T_CHR_TOK[" + m_sText + "]";
        else if (m_iTokenID == T_STR_BODY)
            s = "T_STR_BODY\"" + m_sText + "\"";
        else
            s = "T_BAD_NUM";

        s += "@" + std::to_string(m_iLine);

        return s;
    }
};
//---------------------------------------------------------------------------

class Tokenizer
{
    private:
        UTF8Buffer           m_u8Buf;
        UTF8Buffer           m_u8Tmp;
        std::vector<Token>   m_vTokens;
        unsigned int         m_iTokenPos;
        int                  m_iCurLine;

    protected:
        void push(Token t)
        {
            t.m_iLine = m_iCurLine;
            m_vTokens.push_back(t);
//            LOG_TRACE(Qs("TOKEN %1 @ %2\n")
//                      .arg(QString::fromStdString(t.dump()))
//                      .arg(m_iCurLine));
        }

    public:
        Tokenizer()
        {
        }

        bool checkEOF()
        {
            if (m_u8Buf.length() <= 0)
            {
                push(Token());
                return true;
            }
            return false;
        }

        void tokenize(const std::string &sCode)
        {
            m_vTokens.clear();
            m_iTokenPos = 0;
            m_iCurLine  = 1;

            m_u8Buf.reset();
            m_u8Buf.append_bytes(sCode.data(), sCode.size());

            while (m_u8Buf.length() > 0)
            {
                char c = m_u8Buf.first_byte(true);

                if (charClass(c, " ,\t\r\n\v\f"))
                {
                    if (c == '\n') m_iCurLine++;
                }
                else if (c == '~' && m_u8Buf.first_byte() == '@')
                {
                    m_u8Buf.skip_bytes(1);
                    push(Token("~@"));
                }
                else if (charClass(c, "[]{}()'`~^@"))
                {
                    push(Token(c));
                }
                else if (c == '"')
                {
                    push(Token(c));
                    m_u8Tmp.reset();

                    while (m_u8Buf.length() > 0)
                    {
                        c = m_u8Buf.first_byte(true);
                        if (c == '\\')
                        {
                            if (checkEOF()) return;
                            if (charClass(m_u8Buf.first_byte(), "\\\""))
                                m_u8Tmp.append_byte(m_u8Buf.first_byte(true));
                            else
                            {
                                m_u8Tmp.append_byte(c);
                                c = m_u8Buf.first_byte(true);
                                if (c == '\n') m_iCurLine++;
                                m_u8Tmp.append_byte(c);
                            }
                        }
                        else if (c == '"')
                        {
                            push(Token(m_u8Tmp.as_string()));
                            push(Token(c));
                            break;
                        }
                        else
                        {
                            if (c == '\n') m_iCurLine++;
                            m_u8Tmp.append_byte(c);
                        }
                    }

                    if (checkEOF()) return;
                }
                else if (c == ';')
                {
                    while (m_u8Buf.length() > 0
                           && m_u8Buf.first_byte(true) != '\n')
                        ;

                    if (checkEOF()) return;

                    m_iCurLine++;
                }
                else
                {
                    m_u8Tmp.reset();
                    m_u8Tmp.append_byte(c);

                    while (m_u8Buf.length() > 0)
                    {
                        char peekC = m_u8Buf.first_byte();
                        if (charClass(peekC, " \t\r\n\v\f[]{}()'\"`,;"))
                            break;

                        m_u8Tmp.append_byte(m_u8Buf.first_byte(true));
                    }

                    if (m_u8Tmp.length() > 0)
                        pushAtom(m_u8Tmp);

                    if (checkEOF()) return;
                }
            }

            checkEOF();
        }

        Token peek()
        {
            if (m_vTokens.size() <= m_iTokenPos) return Token();
            return m_vTokens[m_iTokenPos];
        }
        Token next()
        {
            if (m_vTokens.size() <= m_iTokenPos) return Token();
            return m_vTokens[m_iTokenPos++];
        }

        void pushAtom(UTF8Buffer &u8)
        {
            UTF8Buffer u8P(u8);
            if (u8P.length() == 1 && charClass(u8P.first_byte(), "+-"))
            {
                push(Token(u8));
                return;
            }

            bool    bIsDouble   = false;
            bool    bBadNumber  = false;
            double  dVal        = 0;
            int64_t iVal        = 0;
            if (u8BufParseNumber(u8P, dVal, iVal, bIsDouble, bBadNumber))
            {
                if (bBadNumber)
                {
                    Token t;
                    t.m_iTokenID = T_BAD_NUM;
                    push(t);
                }
                else
                {
                    if (bIsDouble)
                        push(Token(dVal));
                    else
                        push(Token(iVal));
                }
            }
            else
                push(Token(u8));
        }

        ~Tokenizer()
        {
        }
};


struct Cell;
struct Cell
{
    Cell *car;
    Cell *cdr;
    Cell() : car(0), cdr(0) { }

    bool is_string()  { return car == 0x1; }
    bool is_integer() { return car == 0x2; }
    bool is_double()  { return car == 0x3; }

    void set_string(const std::string &s)
    {
        if (is_string()) delete ((std::string *) cdr);
        std::string *snew = new std::string(s);
        cdr = (Cell *) snew;
        car = 0x1;
    }
    void set_integer(long i)
    {
        if (is_string()) delete ((std::string *) cdr);
        car = 0x2;
        cdr = (Cell *) i;
    }
    void set_double(double d)
    {
        if (is_string()) delete ((std::string *) cdr);
        car = 0x3;
        cdr = (Cell *) d;
    }
    void clear()
    {
        if (is_string()) delete ((std::string *) cdr);
        car = 0x0;
        cdr = 0x0;
    }

    std::string as_string()
    {
        if (is_string())       return *((std::string *) cdr);
        else if (is_integer()) return std::to_string(as_integer());
        else if (is_double())  return std::to_string(as_double());
        else                   return std::string();
    }

    long as_integer()
    {
        if (is_integer())     { return (long) cdr; }
        else if (is_double()) { return (long) (double) cdr; }
        else if (is_string()) { return atol(as_string().c_str()); }
        else
            return 0;
    }

    double as_double()
    {
        if (is_integer())     { return (double) (long) cdr; }
        else if (is_double()) { return (double) cdr; }
        else if (is_string()) { return atof(as_string().c_str()); }
        else
            return 0;
    }

    ~Cell()
    {
        if (is_string())
            delete ((std::string *) cdr);
    }
};

class Compiler
{
    private:
        std::list<Cell *>   alloc_cells;

        Cell *new_cell()
        {
            Cell *c = new Cell;
            alloc_cells.push_back(c);
            return c;
        }

        void free_cells()
        {
            for (std::list<Cell *>::iterator i = alloc_cells.begin();
                 i != alloc_cells.end();
                 i++)
            {
                i->car = 0;
                i->cdr = 0;
                delete *i;
            }
            alloc_cells.clear();
        }

    public:
        ~Compiler()
        {
            free_cells();
        }

        Cell *parse(const std::string &sCode);

//        Cell *eval(Cell *form);
};

};
