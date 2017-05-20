local tc = require 'lal.util.test_case'
require 'lal.util.strict'

local t = tc.TestCase("basic-http-tests", 1)

function t:test_a_request()
    proc.spawn([[
        function main(args)
            mp.wait("UNKNOWN NEVER RECEIVED", 500)
            print("Sending request...\n");
            local resp = http.get(args[1]);
            print("Response: " .. lal.dump(resp));
            return resp;
        end
    ]], { "http://localhost:19088/" });

    local f = http.bind(19088);
    local r = mp.waitInfinite(f);
    http.response(f[2], r[2], {
        action="data",
        data="FOOBAR",
        contenttype="text/plain"
    });
    local p = mp.wait("", 10000) -- expect process::
    print("X:" .. lal.dump(p))
    tc.assert_eq("ok", p[4])
    tc.assert_eq("FOOBAR", p[5]["body"])
    http.free(f[2]);
end

t:run()
