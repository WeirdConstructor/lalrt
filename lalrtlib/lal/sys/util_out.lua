if (os.getenv("LALRT_LIB")) then package.path = package.path .. ";" .. os.getenv("LALRT_LIB") .. '\\lal\\?.lua'; end;
local _ENV = { _lal_lua_base_ENV = _ENV, _lal_lua_base_pairs = pairs };
for k, v in _lal_lua_base_pairs(_lal_lua_base_ENV) do _ENV["_lal_lua_base_" .. k] = v end;
local _lal_req7 = _lal_lua_base_require 'lal.lang.builtins';
local nil_Q = _lal_req7["nil?"];
local strip_sym = _lal_req7["strip-sym"];
local str = _lal_req7["str"];
local error = _lal_req7["error"];
local strip_kw = _lal_req7["strip-kw"];
local lua_io_open = _lal_lua_base_io.open;
--[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:1]]
--[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:3]]
local read_file; read_file = (function (file)
    --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:4]]
    do
    --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:4]]
        --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:4]]
        local fh; fh = lua_io_open(file,"r");
        --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:4]]
        --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:5]]
        local _lal_if5;
        if nil_Q(fh) then
            _lal_if5 = error(str("couldn't open: ",file));
        end;
        --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:6]]
        do
        --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:6]]
            --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:6]]
            local contents; contents = (fh):read("a");
            --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:6]]
            --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:7]]
            (fh):close();
            return contents;
        end;
    end;
    --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:3]]
end);
--[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:10]]
local write_file; write_file = (function (file, contents)
    --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:11]]
    do
    --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:11]]
        --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:11]]
        local fh; fh = lua_io_open(file,"w");
        --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:11]]
        --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:12]]
        local _lal_if6;
        if nil_Q(fh) then
            _lal_if6 = error(str("couldn't open: ",file));
        end;
        --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:13]]
        (fh):write(contents);
        return (fh):close();
    end;
    --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:10]]
end);
return {["read-file"] = read_file, ["write-file"] = write_file, };
--[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/sys/util.lal:1]]
