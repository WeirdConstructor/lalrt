if (os.getenv("LALRT_LIB")) then package.path = package.path .. ";" .. os.getenv("LALRT_LIB") .. '\\lal\\?.lua'; end;
local _ENV = { _lal_lua_base_ENV = _ENV, _lal_lua_base_pairs = pairs };
for k, v in _lal_lua_base_pairs(_lal_lua_base_ENV) do _ENV["_lal_lua_base_" .. k] = v end;
local _lal_req343 = _lal_lua_base_require 'lal.lang.builtins';
local strip_kw = _lal_req343["strip-kw"];
local strip_sym = _lal_req343["strip-sym"];
--[[./test_output_macro_def.lal:1]]
--[[./test_output_macro_def.lal:1]]
--[[./test_output_macro_def.lal:2]]
local macro_add; macro_add = {["func"] = (function (a, b)
    --[[./test_output_macro_def.lal:2]]
    local _lal_ql340 = {};
    local _lal_qi341 = 1;
    _lal_ql340[_lal_qi341] = "\xFE+";
    _lal_qi341 = _lal_qi341 + 1;
    _lal_ql340[_lal_qi341] = a;
    _lal_qi341 = _lal_qi341 + 1;
    local _lal_qspl342 = b;
    if (_lal_lua_base_type(_lal_qspl342) == 'table') then
        _lal_l4 = #_lal_ql340;
        for _lal_i3 = 1, #_lal_qspl342 do _lal_ql340[_lal_l4 + _lal_i3] = _lal_qspl342[_lal_i3]; end
        _lal_qi341 = _lal_qi341 + #_lal_qspl342;
    else
        _lal_ql340[_lal_qi341] = _lal_qspl342;
        _lal_qi341 = _lal_qi341 + 1;
    end;
    return _lal_ql340;
    --[[./test_output_macro_def.lal:2]]
end), ["sType"] = "primitive-macro", };
return {["func-mul"] = (function (a, b)
    return (a * b * 10);
    --[[./test_output_macro_def.lal:3]]
end), ["macro-add"] = macro_add, };
--[[./test_output_macro_def.lal:1]]
