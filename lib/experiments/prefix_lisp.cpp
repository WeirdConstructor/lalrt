/*

Idee: Lisp mit Postfix-List-Notation:

x(1 2 3) <=> [x 1 2 3] <=> {1 x 2 3}

begin (
    let([x(10) y(20)]
        print(+(x y)))

    if ({10 = 29}
        print("foobar")
        print("foo")))

Alles sind strings. Symbole gibts nicht.
Operationen wandeln ggf. intern um für Berechnungen,
is halt langsam aber meist egal, da es nicht für number-crunching
gedacht ist.

*/

#include <iostream>
#include <sstream>
#include <cctype>

namespace slisp
{
//---------------------------------------------------------------------------

enum TokenType
{
    TOK_STR,
    TOK_L,
    TOK_R,
    TOK_LQ,
    TOK_RQ,
    TOK_LC,
    TOK_RC,
    TOK_EOF
};
//---------------------------------------------------------------------------

struct Token
{
    TokenType       m_type;
    std::string     m_value;
    Token(TokenType t, const std::string &s = "")
        : m_type(t), m_value(s)
    {
    }
};
//---------------------------------------------------------------------------

struct Cell;

struct Cell
{
    uint32_t tag; // data types: strings, integers, doubles, vectors/lists (cells are 2 elem vecs)
    uint32_t len;

    union { Cell *c; std::string *s; } val;
    union { Cell *c; std::string *s; } nxt;
    Cell() : tag(0), val.c(0), nxt.c(0) { }

    bool is_black() { return tag & 0x01; }
    void black()
    {
        if (tag & 0x01) return;

        tag |= 0x01;
        Cell *cv = c_val();
        Cell *cn = c_nxt();

        if (cv) cv->black();
        if (cn) cn->black();
    }
    void white()
    {
        if (!(tag & 0x01)) return;

        tag &= ~0x01;

        Cell *cv = c_val();
        Cell *cn = c_nxt();

        if (cv) cv->white();
        if (cn) cn->white();
    }

    Cell *c_val() { if (s_val()) return 0; else return val.c; }
    Cell *c_nxt() { if (s_nxt()) return 0; else return nxt.c; }

    void s_val(std::string *s) { val.s = s; tag |= 0x04; }
    bool s_val()               { return tag & 0x04; }
    void s_nxt(std::string *s) { nxt.s = s; tag |= 0x08; }
    bool s_nxt()               { return tag & 0x08; }
};
//---------------------------------------------------------------------------

class Parser
{
    private:
        istream                    *m_stream;
        int                         m_line;
        int                         m_col;

        // do value semantic strings => completely internal, like lua
        // assign ids or unique pointers. but strings can contain 0,
        // we need some structure that includes the length. but not
        // bloated like std::string. However, we need it to be hashable by std::unordered_map.
        // we will use std::hash specialization for our own hash function.
        // but we will use the lua hash function
        // we will use https://en.wikipedia.org/wiki/Jenkins_hash_function for now
        std::map<std::string, std::string *>  m_strings;
        std::list<Cell *>                     m_cells;
        std::list<Cell *>                     m_free_cells;
        std::list<Cell *>                     m_root;
        bool                                  m_gc_white;

        bool get_char(char &c)
        {
            if (m_stream->get(c))
            {
                if (c == '\n')
                {
                    m_line++;
                    m_col = 0;
                }
                else
                    m_col++;

                return true;
            }
            return false;
        }

        void unget_char()
        {
            m_col--;
            m_stream->unget();
        }

        bool is_id_delim(char c)
        {
            switch (c)
            {
                case '(': return true;
                case ')': return true;
                case '[': return true;
                case ']': return true;
                case '{': return true;
                case '}': return true;
                default:  return false;
            }
        }

    public:
        Parser()
            : m_stream(0), m_line(0), m_col(0), m_gc_white(false)
        {
        }

        Cell *new_cell()
        {
            Cell *nc = 0;
            if (!m_free_cells) nc = new Cell;
            else
            {
                nc = m_free_cells;
                m_free_cells = nc->nxt.c;
            }
            return nc;
        }

        void gc()
        {
            // TODO: Einfach verlinkte liste nehmen!
//            for (std::list<Cell *>::iterator i = m_root.begin();
//                 i != m_root.end(); i++)
//            {
//                Cell *c = *i;
//
//                if (m_gc_white) c->black();
//                else            c->white();
//            }
//
//            for (std::list<Cell *>::iterator i = m_cells.begin();
//                 i != m_cells.end(); i++)
//            {
//                Cell *c = *i;
//
//                if (m_gc_white)
//                {
//                    if (!c->is_black())
//                    {
//                        m_free_cells.push_back(c);
//                    }
//                }
//                else
//                {
//                }
//            }
//
//            m_gc_white = !m_gc_white;
        }

        std::string *new_string(const std::string &str)
        {
            std::map<std::string, std::string *>::iterator it = m_strings.find(str);
            if (it == m_strings.end())
            {
                std::string *newStr = new std::string(str);
                m_strings[str] = newStr;
                return newStr;
            }
            else
                return it->second;
        }

        void set_stream(std::istream *i) { m_stream = i; }
};
//---------------------------------------------------------------------------

class Tokenizer
{
    private:
        istream     *m_stream;
        int          m_line;
        int          m_col;

        bool get_char(char &c)
        {
            if (m_stream->get(c))
            {
                if (c == '\n')
                {
                    m_line++;
                    m_col = 0;
                }
                else
                    m_col++;

                return true;
            }
            return false;
        }

        void unget_char()
        {
            m_col--;
            m_stream->unget();
        }

        bool is_id_delim(char c)
        {
            switch (c)
            {
                case '(': return true;
                case ')': return true;
                case '[': return true;
                case ']': return true;
                case '{': return true;
                case '}': return true;
                default:  return false;
            }
        }

    public:
        Tokenizer()
            : m_stream(0), m_line(0), m_col(0)
        {
        }

        void set_stream(std::istream *i) { m_stream = i; }

        Token string_token(char end)
        {
            std::stringstream id;

            char n;
            while (get_char(n))
            {
                if (n == '\\')
                {
                    if (!get_char(n))
                        return Token(TOK_EOF);

                    switch (n)
                    {
                        case 'n': id << '\n';
                        case 't': id << '\t';
                        case 'v': id << '\v';
                        case 'r': id << '\r';
                        default:  id << n;
                    }
                }
                else if (n == end)
                    break;
                else
                    id << n;
            }

            return Token(TOK_STR, id.str());
        }

        Token identifier_token()
        {
            std::stringstream id;

            char n;
            while (get_char(n))
            {
                if (isspace(n)) break;
                else if (is_id_delim(n))
                {
                    unget_char();
                    break;
                }
                else id << n;
            }

            return Token(TOK_STR, id.str());
        }

        Token next_token()
        {
            char n;
            if (!get_char(n))
                return Token(TOK_EOF);

            switch (n)
            {
                case '(':  return Token(TOK_L,  "(");
                case ')':  return Token(TOK_R,  ")");
                case '[':  return Token(TOK_LQ, "[");
                case ']':  return Token(TOK_RQ, "]");
                case '{':  return Token(TOK_LC, "{");
                case '}':  return Token(TOK_RC, "}");
                case '"':  return string_token('"');
                case '\'': return string_token('\'');
                case ' ':  return next_token();
                case '\t': return next_token();
                case '\v': return next_token();
                case '\r': return next_token();
                case '\n': return next_token();
                default:   return identifier_token();
            };
        }
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------


}
