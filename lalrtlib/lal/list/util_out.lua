if (os.getenv("LALRT_LIB")) then package.path = package.path .. ";" .. os.getenv("LALRT_LIB") .. '\\lal\\?.lua'; end;
local _ENV = { _lal_lua_base_ENV = _ENV, _lal_lua_base_pairs = pairs };
for k, v in _lal_lua_base_pairs(_lal_lua_base_ENV) do _ENV["_lal_lua_base_" .. k] = v end;
local _lal_req4 = _lal_lua_base_require 'lal.lang.builtins';
local push_M = _lal_req4["push!"];
local strip_kw = _lal_req4["strip-kw"];
local strip_sym = _lal_req4["strip-sym"];
--[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:1]]
--[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:1]]
local skip_until; skip_until = (function (f, lst)
    --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:2]]
    do
    --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:2]]
        --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:2]]
        local out; out = {};
        --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:2]]
        local skipping; skipping = true;
        --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:2]]
        --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:4]]
        for _, elem in _lal_lua_base_ipairs(lst) do
        --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:4]]
            --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:5]]
            --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:5]]
            local _lal_and1;
            _lal_and1 = skipping;
            if _lal_and1 then 
            _lal_and1 = f(elem);
            end;
            local _lal_if2;
            if _lal_and1 then
            --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:5]]
                --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:6]]
                skipping = false;
                _lal_if2 = skipping;
            end;
            --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:7]]
            local _lal_if3;
            if not(skipping) then
                _lal_if3 = push_M(out,elem);
            end;
        end;
        return out;
    end;
    --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:1]]
end);
--[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:11]]
local drop; drop = (function (n, lst)
    --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:12]]
    do
    --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:12]]
        --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:12]]
        local out; out = {};
        --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:12]]
        --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:13]]
        for i = n, (#(lst) - 1) do
        --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:13]]
            push_M(out,((lst)[i + 1]));
        end;
        return out;
    end;
    --[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:11]]
end);
return {["skip-until"] = skip_until, ["drop"] = drop, };
--[[C:\Entwicklung\hg\qluart\lalrt\lalrtlib/lal/list/util.lal:1]]
