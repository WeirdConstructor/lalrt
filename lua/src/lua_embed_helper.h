namespace lua_embed_helper
{

class Output
{
    public:
        virtual ~Output() { }
        virtual void write(const char *buf, size_t len) = 0;
        virtual void write(const char *buf) { this->write(buf, strlen(buf)); }
        virtual void flush_error() = 0;
        virtual void flush_print() = 0;
};

} // namespace lua_embed_helper
