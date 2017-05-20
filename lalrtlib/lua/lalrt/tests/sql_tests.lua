local tc = require 'lal.util.test_case'
require 'lal.util.strict'

local t = tc.TestCase("basic-sql-tests", 1)

function t:test_simple_sql()
    local dbh = sql.session { driver = "sqlite3", file = ":memory:" };
    tc.assert_eq(true, sql.executeM(dbh,
        { "CREATE TABLE test1 (ID INTEGER, name TEXT, nameb BLOB)" }));
    tc.assert_eq(true, sql.executeM(dbh,
        { "INSERT INTO test1 (ID, NAME, NAMEB)",
          "VALUES(",
          { 123 }, ",",
          { "foobaß" }, ",",
          { "\xfffoobaß" },
          ")" }));
    tc.assert_eq(true, sql.executeM(dbh,
        { "INSERT INTO test1 (ID, NAME, NAMEB)",
          "VALUES(",
          { 456 }, ",",
          { "\xffäß" },
          ", x'3434'",
          ")" }));
    tc.assert_eq(true,  sql.executeM(dbh,
        { "SELECT", "nameb,name,", "id", "FROM test1" }));
    local r = sql.row(dbh);
    tc.assert_eq(123, r.id);
    tc.assert_eq("foobaß", r.name);
    tc.assert_eq("\xfffoobaß", r.nameb);
    sql.next(dbh);
    r = sql.row(dbh);
    tc.assert_eq(456, r.id);
    tc.assert_eq("\xFFäß", r.name);
    tc.assert_eq("\xFF44", r.nameb);
    sql.close(dbh);
    sql.destroy(dbh);
end

t:run()
