#!/usr/bin/env tarantool
test = require("sqltester")
test:plan(58)

--!./tcltestrunner.lua
-- 2001 September 15
--
-- The author disclaims copyright to this source code.  In place of
-- a legal notice, here is a blessing:
--
--    May you do good and not evil.
--    May you find forgiveness for yourself and forgive others.
--    May you share freely, never taking more than you give.
--
-------------------------------------------------------------------------
-- This file implements regression tests for SQLite library.  The
-- focus of this file is testing the CREATE TABLE statement.
--
-- ["set","testdir",[["file","dirname",["argv0"]]]]
-- ["source",[["testdir"],"\/tester.tcl"]]
-- Create a basic table and verify it is added to sqlite_master
--
test:do_execsql_test(
    "table-1.1",
    [[
        CREATE TABLE test1 (
          one varchar(10) primary key,
          two text
        )
    ]], {
        -- <table-1.1>
        
        -- </table-1.1>
    })

--   execsql {
--     SELECT sql FROM sqlite_master WHERE type!='meta'
--   }
-- } {{CREATE TABLE test1 (
--       one varchar(10),
--       two text
--     )}}
-- # Verify the other fields of the sqlite_master file.
-- #
-- do_test table-1.3 {
--   execsql {SELECT name, tbl_name, type FROM sqlite_master WHERE type!='meta'}
-- } {test1 test1 table}
-- # Close and reopen the database.  Verify that everything is
-- # still the same.
-- #
-- do_test table-1.4 {
--   db close
--   sqlite3 db test.db
--   execsql {SELECT name, tbl_name, type from sqlite_master WHERE type!='meta'}
-- } {test1 test1 table}
-- Drop the database and make sure it disappears.
--
test:do_test(
    "table-1.5",
    function()
        return test:execsql "DROP TABLE test1"
        --execsql {SELECT * FROM sqlite_master WHERE type!='meta'}
    end, {
        -- <table-1.5>
        
        -- </table-1.5>
    })

-- # Close and reopen the database.  Verify that the table is
-- # still gone.
-- #
-- do_test table-1.6 {
--   db close
--   sqlite3 db test.db
--   execsql {SELECT name FROM sqlite_master WHERE type!='meta'}
-- } {}
-- Repeat the above steps, but this time quote the table name.
--
test:do_test(
    "table-1.10",
    function()
        return test:execsql [[CREATE TABLE "create" (f1 int primary key)]]
        --execsql {SELECT name FROM sqlite_master WHERE type!='meta'}
    end, {
        -- <table-1.10>
        
        -- </table-1.10>
    })

--} {create}
test:do_test(
    "table-1.11",
    function()
        return test:execsql [[DROP TABLE "create"]]
        --execsql {SELECT name FROM "sqlite_master" WHERE type!='meta'}
    end, {
        -- <table-1.11>
        
        -- </table-1.11>
    })

test:do_test(
    "table-1.12",
    function()
        return test:execsql [[CREATE TABLE test1("f1 ho" int primary key)]]
        --execsql {SELECT name as "X" FROM sqlite_master WHERE type!='meta'}
    end, {
        -- <table-1.12>
        
        -- </table-1.12>
    })

--} {test1}
test:do_test(
    "table-1.13",
    function()
        return test:execsql [[DROP TABLE "TEST1"]]
        --execsql {SELECT name FROM "sqlite_master" WHERE type!='meta'}
    end, {
        -- <table-1.13>
        
        -- </table-1.13>
    })

-- Verify that we cannot make two tables with the same name
--
test:do_test(
    "table-2.1",
    function()
        test:execsql "CREATE TABLE TEST2(one text primary key)"
        return test:catchsql "CREATE TABLE test2(id primary key, two text default 'hi')"
    end, {
        -- <table-2.1>
        1, "table TEST2 already exists"
        -- </table-2.1>
    })

-- do_test table-2.1b {
--   set v [catch {execsql {CREATE TABLE sqlite_master(two text)}} msg]
--   lappend v $msg
-- } {1 {object name reserved for internal use: sqlite_master}}
-- do_test table-2.1c {
--   db close
--   sqlite3 db test.db
--   set v [catch {execsql {CREATE TABLE sqlite_master(two text)}} msg]
--   lappend v $msg
-- } {1 {object name reserved for internal use: sqlite_master}}
test:do_catchsql_test(
    "table-2.1d",
    [[
        CREATE TABLE IF NOT EXISTS test2(x primary key,y)
    ]], {
        -- <table-2.1d>
        0
        -- </table-2.1d>
    })

test:do_catchsql_test(
    "table-2.1e",
    [[
        CREATE TABLE IF NOT EXISTS test2(x UNIQUE, y TEXT PRIMARY KEY)
    ]], {
        -- <table-2.1e>
        0
        -- </table-2.1e>
    })

test:do_execsql_test(
    "table-2.1f",
    [[
        DROP TABLE test2; --SELECT name FROM sqlite_master WHERE type!='meta'
    ]], {
        -- <table-2.1f>
        
        -- </table-2.1f>
    })

-- Verify that we cannot make a table with the same name as an index
--
test:do_test(
    "table-2.2a",
    function()
        test:execsql "CREATE TABLE test2(id primary key, one text)"
        return test:execsql "CREATE INDEX test3 ON test2(one)"
        --catchsql {CREATE TABLE test3(id primary key, two text)}
    end, {
        -- <table-2.2a>
        
        -- </table-2.2a>
    })

--} {1 {there is already an index named test3}}
-- do_test table-2.2b {
--   db close
--   sqlite3 db test.db
--   set v [catch {execsql {CREATE TABLE test3(two text)}} msg]
--   lappend v $msg
-- } {1 {there is already an index named test3}}
-- do_test table-2.2c {
--   execsql {SELECT name FROM sqlite_master WHERE type!='meta' ORDER BY name}
-- } {test2 test3}
test:do_test(
    "table-2.2d",
    function()
        test:execsql [[DROP INDEX test3 ON test2]]
        return test:catchsql "CREATE TABLE test3(two text primary key)"
    end, {
        -- <table-2.2d>
        0
        -- </table-2.2d>
    })

-- do_test table-2.2e {
--   execsql {SELECT name FROM sqlite_master WHERE type!='meta' ORDER BY name}
-- } {test2 test3}
test:do_test(
    "table-2.2f",
    function()
        return test:execsql "DROP TABLE test2; DROP TABLE test3"
        --execsql {SELECT name FROM sqlite_master WHERE type!='meta' ORDER BY name}
    end, {
        -- <table-2.2f>
        
        -- </table-2.2f>
    })

-- Create a table with many field names
--
local big_table = [[CREATE TABLE big(
  f1 varchar(20),
  f2 char(10),
  f3 varchar(30) primary key,
  f4 text,
  f5 text,
  f6 text,
  f7 text,
  f8 text,
  f9 text,
  f10 text,
  f11 text,
  f12 text,
  f13 text,
  f14 text,
  f15 text,
  f16 text,
  f17 text,
  f18 text,
  f19 text,
  f20 text
)]]
test:do_test(
    "table-3.1",
    function()
        return test:execsql(big_table)
        --execsql {SELECT sql FROM sqlite_master WHERE type=='table'}
    end, {
        -- <table-3.1>
        
        -- </table-3.1>
    })

--} \{$big_table\}
test:do_catchsql_test(
    "table-3.2",
    [[
        CREATE TABLE BIG(xyz foo primary key)
    ]], {
        -- <table-3.2>
        1, "table BIG already exists"
        -- </table-3.2>
    })

test:do_catchsql_test(
    "table-3.3",
    [[
        CREATE TABLE biG(xyz foo primary key)
    ]], {
        -- <table-3.3>
        1, "table BIG already exists"
        -- </table-3.3>
    })

test:do_catchsql_test(
    "table-3.4",
    [[
        CREATE TABLE bIg(xyz foo primary key)
    ]], {
        -- <table-3.4>
        1, "table BIG already exists"
        -- </table-3.4>
    })

-- do_test table-3.5 {
--   db close
--   sqlite3 db test.db
--   set v [catch {execsql {CREATE TABLE Big(xyz foo)}} msg]
--   lappend v $msg
-- } {1 {table Big already exists}}
test:do_test(
    "table-3.6",
    function()
        return test:execsql "DROP TABLE big"
        --execsql {SELECT name FROM sqlite_master WHERE type!='meta'}
    end, {
        -- <table-3.6>
        
        -- </table-3.6>
    })

-- Try creating large numbers of tables
--
local r = {}
for i = 1, 100, 1 do
    table.insert(r, string.format("TEST%03d", i))
end
test:do_test(
    "table-4.1",
    function()
        for i = 1, 100, 1 do
            local sql = "CREATE TABLE "..string.format("test%03d", i).." (id primary key, "
            for k = 1, i-1, 1 do
                sql = sql .. "field"..k.." text,"
            end
            sql = sql .. "last_field text)"
            test:execsql(sql)
        end
        --execsql {SELECT name FROM sqlite_master WHERE type!='meta' ORDER BY name}
        return test:execsql [[SELECT "name" FROM "_space" WHERE "id">500]]
    end, r)

-- do_test table-4.1b {
--   db close
--   sqlite3 db test.db
--   execsql {SELECT name FROM sqlite_master WHERE type!='meta' ORDER BY name}
-- } $r
-- Drop the even numbered tables
--
r = {}
for i = 1, 100, 2 do
    table.insert(r,string.format("TEST%03d", i))
end
test:do_test(
    "table-4.2",
    function()
        for i = 2, 100, 2 do
            -- if {$i==38} {execsql {pragma vdbe_trace=on}}
            local sql = "DROP TABLE "..string.format("TEST%03d", i)..""
            test:execsql(sql)
        end
        --execsql {SELECT name FROM sqlite_master WHERE type!='meta' ORDER BY name}
        return test:execsql([[SELECT "name" FROM "_space" WHERE "id">500]])
    end, r)

--exit
-- Drop the odd number tables
--
test:do_test(
    "table-4.3",
    function()
        for i = 1, 100, 2 do
            local sql = "DROP TABLE "..string.format("test%03d", i)
            test:execsql(sql)
        end
        --execsql {SELECT name FROM sqlite_master WHERE type!='meta' ORDER BY name}
        return test:execsql [[SELECT "name" FROM "_space" WHERE "id">500]]
    end, {
        -- <table-4.3>
        
        -- </table-4.3>
    })

-- Try to drop a table that does not exist
--
test:do_catchsql_test(
    "table-5.1.1",
    [[
        DROP TABLE test009
    ]], {
        -- <table-5.1.1>
        1, "no such table: TEST009"
        -- </table-5.1.1>
    })

test:do_catchsql_test(
    "table-5.1.2",
    [[
        DROP TABLE IF EXISTS TEST009
    ]], {
        -- <table-5.1.2>
        0
        -- </table-5.1.2>
    })

-- # Try to drop sqlite_master
-- #
-- do_test table-5.2 {
--   catchsql {DROP TABLE IF EXISTS sqlite_master}
-- } {1 {table sqlite_master may not be dropped}}

-- Dropping sql_statN tables is OK.
--
test:do_test(
    "table-5.2.1",
    function()
        return test:execsql [[
            ---ANALYZE;
            DROP TABLE IF EXISTS sql_stat1;
            DROP TABLE IF EXISTS sql_stat2;
            DROP TABLE IF EXISTS sql_stat3;
            DROP TABLE IF EXISTS sql_stat4;
        ]]
    end, {
        -- <table-5.2.1>

        -- </table-5.2.1>
    })

test:drop_all_tables()
-- MUST_WORK_TEST
if (0 > 0)
 then
    test:do_test(
        "table-5.2.2",
        function()
            db("close")
            forcedelete("test.db")
            sqlite3("db", "test.db")
            return test:execsql [[
                CREATE TABLE t0(a,b);
                CREATE INDEX t ON t0(a);
                UPDATE sqlite_master SET sql='CREATE TABLE a.b(a UNIQUE';
                --BEGIN;
                --CREATE TABLE t1(x);
                --ROLLBACK;
                DROP TABLE IF EXISTS t99;
            ]]
        end, {
            -- <table-5.2.2>
            
            -- </table-5.2.2>
        })

    db("close")
    forcedelete("test.db")
    sqlite3("db", "test.db")
    X(313, "X!cmd", [=[["Make","sure","an","EXPLAIN","does","not","really","create","a","new","table"]]=])
end
test:do_test(
    "table-5.3",
    function()
        test:execsql "EXPLAIN CREATE TABLE test1(f1 int primary key)"


        return test:execsql [[SELECT "name" FROM "_space" WHERE "id">500]]
        --execsql {SELECT name FROM sqlite_master WHERE type!='meta'}
    end, {
        -- <table-5.3>
        
        -- </table-5.3>
    })

-- Make sure an EXPLAIN does not really drop an existing table
--
test:do_test(
    "table-5.4",
    function()
        test:execsql "CREATE TABLE test1(f1 int primary key)"
        test:execsql "EXPLAIN DROP TABLE test1"


        return test:execsql [[SELECT "name" FROM "_space" WHERE "id">500]]
        --execsql {SELECT name FROM sqlite_master WHERE type!='meta'}
    end, {
        -- <table-5.4>
        "TEST1"
        -- </table-5.4>
    })

-- Create a table with a goofy name
--
--do_test table-6.1 {
--  execsql {CREATE TABLE 'Spaces In This Name!'(x int)}
--  execsql {INSERT INTO 'spaces in this name!' VALUES(1)}
--  set list [glob -nocomplain testdb/spaces*.tbl]
--} {testdb/spaces+in+this+name+.tbl}
-- Try using keywords as table names or column names.
-- 
test:do_catchsql_test(
    "table-7.1",
    [=[
        CREATE TABLE weird(
          id primary key,
          "desc" text,
          "asc" text,
          key int,
          "14_vac" boolean,
          fuzzy_dog_12 varchar(10),
          beginn blob,
          endd clob
        )
    ]=], {
        -- <table-7.1>
        0
        -- </table-7.1>
    })

test:do_execsql_test(
    "table-7.2",
    [[
        INSERT INTO weird VALUES(1, 'a','b',9,0,'xyz','hi','y''all');
        SELECT * FROM weird;
    ]], {
        -- <table-7.2>
        1, "a", "b", 9, 0, "xyz", "hi", "y'all"
        -- </table-7.2>
    })

test:do_execsql2_test(
    "table-7.3",
    [[
        SELECT * FROM weird;
    ]], {
        -- <table-7.3>
        "ID",1,"desc","a","asc","b","KEY",9,"14_vac",0,"FUZZY_DOG_12","xyz","BEGINN","hi","ENDD","y'all"
        -- </table-7.3>
    })

test:do_execsql_test(
    "table-7.3",
    [[
        CREATE TABLE savepoint_t(release_t primary key);
        INSERT INTO savepoint_t(release_t) VALUES(10);
        UPDATE savepoint_t SET release_t = 5;
        SELECT release_t FROM savepoint_t;
    ]], {
        -- <table-7.3>
        5
        -- </table-7.3>
    })

-- Try out the CREATE TABLE AS syntax
--
test:do_execsql2_test(
    "table-8.1",
    [=[
        --CREATE TABLE t2 AS SELECT * FROM weird;
        CREATE TABLE t2(
          id primary key,
          "desc" text,
          "asc" text,
          key int,
          "14_vac" boolean,
          fuzzy_dog_12 varchar(10),
          beginn blob,
          endd clob
        );
        INSERT INTO t2 SELECT * from weird;
        SELECT * FROM t2;
    ]=], {
        -- <table-8.1>
        "ID",1,"desc","a","asc","b","KEY",9,"14_vac",0,"FUZZY_DOG_12","xyz","BEGINN","hi","ENDD","y'all"
        -- </table-8.1>
    })

-- do_test table-8.1.1 {
--   execsql {
--     SELECT sql FROM sqlite_master WHERE name='t2';
--   }
-- } {{CREATE TABLE t2(
--   "desc" TEXT,
--   "asc" TEXT,
--   "key" INT,
--   "14_vac" NUM,
--   fuzzy_dog_12 TEXT,
--   "begin",
--   "end" TEXT
-- )}}
-- do_test table-8.2 {
--   execsql {
--     CREATE TABLE "t3""xyz"(a,b,c);
--     INSERT INTO [t3"xyz] VALUES(1,2,3);
--     SELECT * FROM [t3"xyz];
--   }
-- } {1 2 3}
-- do_test table-8.3 {
--   execsql2 {
--     CREATE TABLE [t4"abc] AS SELECT count(*) as cnt, max(b+c) FROM [t3"xyz];
--     SELECT * FROM [t4"abc];
--   }
-- } {cnt 1 max(b+c) 5}
-- # Update for v3: The declaration type of anything except a column is now a
-- # NULL pointer, so the created table has no column types. (Changed result
-- # from {{CREATE TABLE 't4"abc'(cnt NUMERIC,"max(b+c)" NUMERIC)}}).
-- do_test table-8.3.1 {
--   execsql {
--     SELECT sql FROM sqlite_master WHERE name='t4"abc'
--   }
-- } {{CREATE TABLE "t4""abc"(cnt,"max(b+c)")}}
-- ifcapable tempdb {
--   do_test table-8.4 {
--     execsql2 {
--       CREATE TEMPORARY TABLE t5 AS SELECT count(*) AS [y'all] FROM [t3"xyz];
--       SELECT * FROM t5;
--     }
--   } {y'all 1}
-- }
-- do_test table-8.5 {
--   db close
--   sqlite3 db test.db
--   execsql2 {
--     SELECT * FROM [t4"abc];
--   }
-- } {cnt 1 max(b+c) 5}

-- gh-2166 Tables with TEMP and TEMPORARY were removed before.

test:do_catchsql_test(
	"temp",
	[[
		CREATE TEMP TABLE t1(a INTEGER PRIMARY KEY, b VARCHAR(10));
	]], {
	-- <temp>
	1, "near \"TEMP\": syntax error"
	-- <temp>
	})

test:do_catchsql_test(
	"temporary",
	[[
		CREATE TEMPORARY TABLE t1(a INTEGER PRIMARY KEY, b VARCHAR(10));
	]], {
	-- <temporary>
	1, "near \"TEMPORARY\": syntax error"
	-- <temporary>
	})

test:do_execsql2_test(
    "table-8.6",
    [[
        SELECT * FROM t2;
    ]], {
        -- <table-8.6>
        "ID",1,"desc","a","asc","b","KEY",9,"14_vac",0,"FUZZY_DOG_12","xyz","BEGINN","hi","ENDD","y'all"
        -- </table-8.6>
    })

test:do_catchsql_test(
    "table-8.7",
    [[
        SELECT * FROM t5;
    ]], {
        -- <table-8.7>
        1, "no such table: T5"
        -- </table-8.7>
    })

-- do_test table-8.8 {
--   catchsql {
--     CREATE TABLE t5 AS SELECT * FROM no_such_table;
--   }
-- } {1 {no such table: no_such_table}}
-- do_test table-8.9 {
--   execsql {
--     CREATE TABLE t10("col.1" [char.3]);
--     CREATE TABLE t11 AS SELECT * FROM t10;
--     SELECT sql FROM sqlite_master WHERE name = 't11';
--   }
-- } {{CREATE TABLE t11("col.1" TEXT)}}
-- do_test table-8.10 {
--   execsql {
--     CREATE TABLE t12(
--       a INTEGER,
--       b VARCHAR(10),
--       c VARCHAR(1,10),
--       d VARCHAR(+1,-10),
--       e VARCHAR (+1,-10),
--       f "VARCHAR (+1,-10, 5)",
--       g BIG INTEGER
--     );
--     CREATE TABLE t13 AS SELECT * FROM t12;
--     SELECT sql FROM sqlite_master WHERE name = 't13';
--   }
-- } {{CREATE TABLE t13(
--   a INT,
--   b TEXT,
--   c TEXT,
--   d TEXT,
--   e TEXT,
--   f TEXT,
--   g INT
-- )}}
-- Make sure we cannot have duplicate column names within a table.
--
test:do_catchsql_test(
    "table-9.1",
    [[
        CREATE TABLE t6(a primary key,b,a);
    ]], {
        -- <table-9.1>
        1, "duplicate column name: A"
        -- </table-9.1>
    })

test:do_catchsql_test(
    "table-9.2",
    [[
        CREATE TABLE t6(a varchar(100) primary key, b blob, a integer);
    ]], {
        -- <table-9.2>
        1, "duplicate column name: A"
        -- </table-9.2>
    })

-- Check the foreign key syntax.
--
test:do_catchsql_test(
    "table-10.1",
    [[
        -- there is no t4 table
        --CREATE TABLE t6(a REFERENCES t4(a) NOT NULL primary key);
        CREATE TABLE t6(a REFERENCES t2(id) NOT NULL primary key);
        INSERT INTO t6 VALUES(NULL);
    ]], {
        -- <table-10.1>
        1, "NOT NULL constraint failed: T6.A"
        -- </table-10.1>
    })

test:do_catchsql_test(
    "table-10.2",
    [[
        DROP TABLE t6;
        CREATE TABLE t6(a REFERENCES t4(a) MATCH PARTIAL primary key);
    ]], {
        -- <table-10.2>
        0
        -- </table-10.2>
    })

test:do_catchsql_test(
    "table-10.3",
    [[
        DROP TABLE t6;
        CREATE TABLE t6(a REFERENCES t4 MATCH FULL ON DELETE SET NULL NOT NULL primary key);
    ]], {
        -- <table-10.3>
        0
        -- </table-10.3>
    })

test:do_catchsql_test(
    "table-10.4",
    [[
        DROP TABLE t6;
        CREATE TABLE t6(a REFERENCES t4 MATCH FULL ON UPDATE SET DEFAULT DEFAULT 1 primary key);
    ]], {
        -- <table-10.4>
        0
        -- </table-10.4>
    })

test:do_catchsql_test(
    "table-10.5",
    [[
        DROP TABLE t6;
        CREATE TABLE t6(a NOT NULL NOT DEFERRABLE INITIALLY IMMEDIATE primary key);
    ]], {
        -- <table-10.5>
        0
        -- </table-10.5>
    })

test:do_catchsql_test(
    "table-10.6",
    [[
        DROP TABLE t6;
        CREATE TABLE t6(a NOT NULL DEFERRABLE INITIALLY DEFERRED primary key);
    ]], {
        -- <table-10.6>
        0
        -- </table-10.6>
    })

test:do_catchsql_test(
    "table-10.7",
    [[
        DROP TABLE t6;
        CREATE TABLE t6(a primary key,
          FOREIGN KEY (a) REFERENCES t4(b) DEFERRABLE INITIALLY DEFERRED
        );
    ]], {
        -- <table-10.7>
        0
        -- </table-10.7>
    })

test:do_catchsql_test(
    "table-10.8",
    [[
        DROP TABLE t6;
        CREATE TABLE t6(a primary key,b,c,
          FOREIGN KEY (b,c) REFERENCES t4(x,y) MATCH PARTIAL
            ON UPDATE SET NULL ON DELETE CASCADE DEFERRABLE INITIALLY DEFERRED
        );
    ]], {
        -- <table-10.8>
        0
        -- </table-10.8>
    })

test:do_catchsql_test(
    "table-10.9",
    [[
        DROP TABLE t6;
        CREATE TABLE t6(a primary key,b,c,
          FOREIGN KEY (b,c) REFERENCES t4(x)
        );
    ]], {
        -- <table-10.9>
        1, "number of columns in foreign key does not match the number of columns in the referenced table"
        -- </table-10.9>
    })

test:do_test(
    "table-10.10",
    function()
        test:catchsql "DROP TABLE t6"
        return test:catchsql [[
            CREATE TABLE t6(a primary key,b,c,
              FOREIGN KEY (b,c) REFERENCES t4(x,y,z)
            );
        ]]
    end, {
        -- <table-10.10>
        1, "number of columns in foreign key does not match the number of columns in the referenced table"
        -- </table-10.10>
    })

test:do_test(
    "table-10.11",
    function()
        test:catchsql "DROP TABLE t6"
        return test:catchsql [[
            CREATE TABLE t6(a,b, c REFERENCES t4(x,y));
        ]]
    end, {
        -- <table-10.11>
        1, "foreign key on C should reference only one column of table t4"
        -- </table-10.11>
    })

test:do_test(
    "table-10.12",
    function()
        test:catchsql "DROP TABLE t6"
        return test:catchsql [[
            CREATE TABLE t6(a,b,c,
              FOREIGN KEY (b,x) REFERENCES t4(x,y)
            );
        ]]
    end, {
        -- <table-10.12>
        1, [[unknown column "X" in foreign key definition]]
        -- </table-10.12>
    })

test:do_test(
    "table-10.13",
    function()
        test:catchsql "DROP TABLE t6"
        return test:catchsql [[
            CREATE TABLE t6(a,b,c,
              FOREIGN KEY (x,b) REFERENCES t4(x,y)
            );
        ]]
    end, {
        -- <table-10.13>
        1, [[unknown column "X" in foreign key definition]]
        -- </table-10.13>
    })



-- endif foreignkey
-- Test for the typeof function. More tests for the
-- typeof() function are found in bind.test and types.test.
test:do_execsql_test(
    "table-11.1",
    [[
        CREATE TABLE t7(
           a integer primary key,
           b number(5,10),
           c character varying (8),
           d VARCHAR(9),
           e clob,
           f BLOB,
           g Text,
           h
        );
        INSERT INTO t7(a) VALUES(1);
        SELECT typeof(a), typeof(b), typeof(c), typeof(d),
               typeof(e), typeof(f), typeof(g), typeof(h)
        FROM t7 LIMIT 1;
    ]], {
        -- <table-11.1>
        "integer", "null", "null", "null", "null", "null", "null", "null"
        -- </table-11.1>
    })

test:do_execsql_test(
    "table-11.2",
    [[
        SELECT typeof(a+b), typeof(a||b), typeof(c+d), typeof(c||d)
        FROM t7 LIMIT 1;
    ]], {
        -- <table-11.2>
        "null", "null", "null", "null"
        -- </table-11.2>
    })

-- # Test that when creating a table using CREATE TABLE AS, column types are
-- # assigned correctly for (SELECT ...) and 'x AS y' expressions.
-- do_test table-12.1 {
--   ifcapable subquery {
--     execsql {
--       CREATE TABLE t8 AS SELECT b, h, a as i, (SELECT f FROM t7) as j FROM t7;
--     }
--   } else {
--     execsql {
--       CREATE TABLE t8 AS SELECT b, h, a as i, f as j FROM t7;
--     }
--   }
-- } {}
-- do_test table-12.2 {
--   execsql {
--     SELECT sql FROM sqlite_master WHERE tbl_name = 't8'
--   }
-- } {{CREATE TABLE t8(b NUM,h,i INT,j)}}
----------------------------------------------------------------------
-- Test cases table-13.*
--
-- Test the ability to have default values of CURRENT_TIME, CURRENT_DATE
-- and CURRENT_TIMESTAMP.
--
test:do_execsql_test(
    "table-13.1",
    [[
        CREATE TABLE tablet8(
           a integer primary key,
           tm text DEFAULT CURRENT_TIME,
           dt text DEFAULT CURRENT_DATE,
           dttm text DEFAULT CURRENT_TIMESTAMP
        );
        SELECT * FROM tablet8;
    ]], {
        -- <table-13.1>
        
        -- </table-13.1>
    })

-- MUST_WORK_TEST debug var sqlite_current_time #2646
if (0 > 0) then
-- ["unset","-nocomplain","date","time","seconds"]
local data = {
    {"1976-07-04", "12:00:00", 205329600},
    {"1994-04-16", "14:00:00", 766504800},
    {"2000-01-01", "00:00:00", 946684800},
    {"2003-12-31", "12:34:56", 1072874096},
}
for i ,val in ipairs(data) do
    local date = val[1]
    local time = val[2]
    local seconds = val[3]
    sqlite_current_time = seconds
    test:do_execsql_test(
        "table-13.2."..i,
        string.format([[
            INSERT INTO tablet8(a) VALUES(%s);
            SELECT tm, dt, dttm FROM tablet8 WHERE a=%s;
        ]], i, i), {
            time, date, { date, time }
        })

end
sqlite_current_time = 0
end
----------------------------------------------------------------------
-- Test cases table-14.*
--
-- Test that a table cannot be created or dropped while other virtual
-- machines are active.
-- 2007-05-02:  A open btree cursor no longer blocks CREATE TABLE.
-- But DROP TABLE is still prohibited because we do not want to
-- delete a table out from under a running query.
--
-- db eval {
--   pragma vdbe_trace = 0;
-- }
-- Try to create a table from within a callback:
-- ["unset","-nocomplain","result"]
test:do_test(
    "table-14.1",
    function()
        local rc = pcall(function()
            test:execsql("SELECT * FROM tablet8 LIMIT 1")
            test:execsql("CREATE TABLE t9(a primary key, b, c)")
            end)
        rc = rc == true and 0 or 1
        return { rc }
    end, {
        -- <table-14.1>
        0
        -- </table-14.1>
    })

-- MUST_WORK_TEST database should be locked #2554
if 0>0 then
local function try_drop_t9()
    box.sql.execute("DROP TABLE t9;")
    return 1
end
box.internal.sql_create_function("try_drop_t9", try_drop_t9)
-- Try to drop a table from within a callback:
test:execsql("INSERT into tablet8 values(2, '2000-01-01', '00:00:00', 946684800);")
test:do_test(
    "table-14.2",
    function()
--        set rc [
--              catch {
--                  db eval {SELECT * FROM tablet8 LIMIT 1} {} {
--                      db eval {DROP TABLE t9;}
--                      }
--                  } msg
--          ]
--           set result [list $rc $msg]
--         } {1 {database table is locked}}
        local rc, msg = pcall(function()
            test:execsql("SELECT *, try_drop_t9() FROM tablet8 LIMIT 1")
            --test:execsql("DROP TABLE t9;")
            end)
        rc = rc == true and 0 or 1
        return { rc, msg }
    end, {
        -- <table-14.2>
        1, "database table is locked"
        -- got ok
        -- </table-14.2>
    })
end
-- ifcapable attach {
--   # Now attach a database and ensure that a table can be created in the 
--   # attached database whilst in a callback from a query on the main database.
--   do_test table-14.3 {
--     forcedelete test2.db
--     forcedelete test2.db-journal
--     execsql {
--       ATTACH 'test2.db' as aux;
--     }
--     db eval {SELECT * FROM tablet8 LIMIT 1} {} {
--       db eval {CREATE TABLE aux.t1(a, b, c)}
--     }
--   } {}
--   # On the other hand, it should be impossible to drop a table when any VMs 
--   # are active. This is because VerifyCookie instructions may have already
--   # been executed, and btree root-pages may not move after this (which a
--   # delete table might do).
--   do_test table-14.4 {
--     set rc [
--       catch {
--         db eval {SELECT * FROM tablet8 LIMIT 1} {} {
--           db eval {DROP TABLE aux.t1;}
--         }
--       } msg
--     ] 
--     set result [list $rc $msg]
--   } {1 {database table is locked}}
-- }
-- Create and drop 2000 tables. This is to check that the balance_shallow()
-- routine works correctly on the sqlite_master table. At one point it
-- contained a bug that would prevent the right-child pointer of the
-- child page from being copied to the root page.
--
test:do_test(
    "table-15.1",
    function()
        --test:execsql "BEGIN"
        for i = 0, 2000-1, 1 do
            test:execsql("CREATE TABLE tbl"..i.." (a primary key, b, c)")
        end
        --return test:execsql "COMMIT"
        return
    end, {
        -- <table-15.1>
        
        -- </table-15.1>
    })

test:do_test(
    "table-15.2",
    function()
        -- test:execsql "BEGIN"
        for i = 0, 2000-1, 1 do
            test:execsql("DROP TABLE tbl"..i.."")
        end
        -- return test:execsql "COMMIT"
        return
    end, {
        -- <table-15.2>
        
        -- </table-15.2>
    })

-- # Ticket 3a88d85f36704eebe134f7f48aebf00cd6438c1a (2014-08-05)
-- # The following SQL script segfaults while running the INSERT statement:
-- #
-- #    CREATE TABLE t1(x DEFAULT(max(1)));
-- #    INSERT INTO t1(rowid) VALUES(1);
-- #
-- # The problem appears to be the use of an aggregate function as part of
-- # the default value for a column. This problem has been in the code since
-- # at least 2006-01-01 and probably before that. This problem was detected
-- # and reported on the sqlite-users@sqlite.org mailing list by Zsbán Ambrus. 
-- #
-- do_execsql_test table-16.1 {
--   CREATE TABLE t16(x DEFAULT(max(1)));
--   INSERT INTO t16(x) VALUES(123);
--   SELECT rowid, x FROM t16;
-- } {1 123}
-- do_catchsql_test table-16.2 {
--   INSERT INTO t16(rowid) VALUES(4);
-- } {1 {unknown function: max()}}
-- do_execsql_test table-16.3 {
--   DROP TABLE t16;
--   CREATE TABLE t16(x DEFAULT(abs(1)));
--   INSERT INTO t16(rowid) VALUES(4);
--   SELECT rowid, x FROM t16;
-- } {4 1}
-- do_catchsql_test table-16.4 {
--   DROP TABLE t16;
--   CREATE TABLE t16(x DEFAULT(avg(1)));
--   INSERT INTO t16(rowid) VALUES(123);
--   SELECT rowid, x FROM t16;
-- } {1 {unknown function: avg()}}
-- do_catchsql_test table-16.5 {
--   DROP TABLE t16;
--   CREATE TABLE t16(x DEFAULT(count()));
--   INSERT INTO t16(rowid) VALUES(123);
--   SELECT rowid, x FROM t16;
-- } {1 {unknown function: count()}}
-- do_catchsql_test table-16.6 {
--   DROP TABLE t16;
--   CREATE TABLE t16(x DEFAULT(group_concat('x',',')));
--   INSERT INTO t16(rowid) VALUES(123);
--   SELECT rowid, x FROM t16;
-- } {1 {unknown function: group_concat()}}
-- do_catchsql_test table-16.7 {
--   INSERT INTO t16 DEFAULT VALUES;
-- } {1 {unknown function: group_concat()}}
-- # Ticket [https://www.sqlite.org/src/info/094d39a4c95ee4abbc417f04214617675ba15c63]
-- # describes a assertion fault that occurs on a CREATE TABLE .. AS SELECT statement.
-- # the following test verifies that the problem has been fixed.
-- #
-- do_execsql_test table-17.1 {
--   DROP TABLE IF EXISTS t1;
--   CREATE TABLE t1(a TEXT);
--   INSERT INTO t1(a) VALUES(1),(2);
--   DROP TABLE IF EXISTS t2;
--   CREATE TABLE t2(x TEXT, y TEXT);
--   INSERT INTO t2(x,y) VALUES(3,4);
--   DROP TABLE IF EXISTS t3;
--   CREATE TABLE t3 AS
--     SELECT a AS p, coalesce(y,a) AS q FROM t1 LEFT JOIN t2 ON a=x;
--   SELECT p, q, '|' FROM t3 ORDER BY p;
-- } {1 1 | 2 2 |}
-- # 2015-06-16
-- # Ticket [https://www.sqlite.org/src/tktview/873cae2b6e25b1991ce5e9b782f9cd0409b96063]
-- # Make sure a CREATE TABLE AS statement correctly rolls back partial changes to the
-- # sqlite_master table when the SELECT on the right-hand side aborts.
-- #
-- do_catchsql_test table-18.1 {
--   DROP TABLE IF EXISTS t1;
--   BEGIN;
--   CREATE TABLE t1 AS SELECT zeroblob(2e20);
-- } {1 {string or blob too big}}
-- do_execsql_test table-18.2 {
--   COMMIT;
--   PRAGMA integrity_check;
-- } {ok}
-- # 2015-09-09
-- # Ticket [https://www.sqlite.org/src/info/acd12990885d9276]
-- # "CREATE TABLE ... AS SELECT ... FROM sqlite_master" fails because the row
-- # in the sqlite_master table for the next table is initially populated
-- # with a NULL instead of a record created by OP_Record.
-- #
-- do_execsql_test table-19.1 {
--   CREATE TABLE t19 AS SELECT * FROM sqlite_master;
--   SELECT name FROM t19 ORDER BY name;
-- } {{} savepoint t10 t11 t12 t13 t16 t2 t3 t3\"xyz t4\"abc t7 t8 t9 tablet8 test1 weird}

test:do_test(
    "table-20.1",
    function()
        local columns = {}
        for i = 0, 1000-1, 1 do
            table.insert(columns, "c"..i)
        end
        columns = table.concat(columns, ",")
        test:execsql("CREATE TABLE t(c primary key, "..columns..")")
        return
    end, {
    -- <table-15.1>

    -- </table-15.1>
})

test:do_test(
    "table-20.2",
    function()
        test:execsql("DROP TABLE t")
        return
    end, {
    -- <table-15.1>

    -- </table-15.1>
})
test:finish_test()
