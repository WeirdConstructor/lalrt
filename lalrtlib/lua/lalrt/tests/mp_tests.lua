local tc = require 'lal.util.test_case'
require 'lal.util.strict'

local t = tc.TestCase("basic-mp-tests", 3)

function t:prepare_spawn()
    self._pid = proc.spawn([[
        function main(args)
            mp.send("started")
            local m = mp.wait("check")
            mp.send(m[1], { m[2], "reply" })
        end
    ]])
end

function t:test_spawn()
    mp.wait("started")
    local f = mp.send(self._pid, {"check"})
    local r = mp.waitInfinite(f)
    tc.assert_eq("reply", r[4], "got reply from spawned process")
end

function t:test_self_msg()
    mp.send(proc.pid(), { "foobar" })
    tc.assert_eq("foobar", mp.wait("foobar")[3], "got message from self")
end

function t:test_def_handlers()
    mp.setDebugLogging(true);
    local ord = {}
    mp.addDefaultHandler(function (msg)
        table.insert(ord, "XXX" .. msg[3]);
    end)
    mp.addDefaultHandler(function (msg)
        table.insert(ord, "YYY" .. msg[3]);
    end)
    mp.addDefaultHandler(function (msg)
        table.insert(ord, "FFF" .. msg[3]);
    end)
    mp.addDefaultHandler(function (msg)
        table.insert(ord, "ZZZ" .. msg[3]);
    end)
    mp.send(proc.pid(), { "FOOBAR" })
    mp.send(proc.pid(), { "BAZ" })
    mp.checkAvailable("BAZ")

    tc.assert_eq(
        "[[\"XXXFOOBAR\" \"YYYFOOBAR\" \"FFFFOOBAR\" \"ZZZFOOBAR\" \"XXXBAZ\" \"YYYBAZ\" \"FFFBAZ\" \"ZZZBAZ\"]]",
        lal.dump(ord),
        "default handlers correctly called");
end

t:run()
