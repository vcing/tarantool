#!/usr/bin/env tarantool
test = require("sqltester")
test:plan(18)

test:do_catchsql_test(
	"xfer-optimization-1.1",
	[[
		CREATE TABLE t1(a INTEGER PRIMARY KEY, b INTEGER UNIQUE);
		INSERT INTO t1 VALUES (1, 1), (2, 2), (3, 3);
		CREATE TABLE t2(a INTEGER PRIMARY KEY, b INTEGER UNIQUE);
		INSERT INTO t2 SELECT * FROM t1;
	]], {
		-- <xfer-optimization-1.1>
		0
		-- <xfer-optimization-1.1>
	})

test:do_execsql_test(
	"xfer-oprimization-1.2",
	[[
		SELECT * FROM t2;
	]], {
		-- <xfer-oprimization-1.2>
		1, 1, 2, 2, 3, 3
		-- <xfer-oprimization-1.2>
	})

test:do_catchsql_test(
	"xfer-optimization-1.3",
	[[
		DROP TABLE t1;
		DROP TABLE t2;
		CREATE TABLE t1(id INTEGER PRIMARY KEY, b INTEGER);
		CREATE TABLE t2(id INTEGER PRIMARY KEY, b INTEGER);
		CREATE INDEX i1 ON t1(b);
		CREATE INDEX i2 ON t2(b);
		INSERT INTO t1 VALUES (1, 1), (2, 2), (3, 3);
		INSERT INTO t2 SELECT * FROM t1;
	]], {
		-- <xfer-optimization-1.3>
		0
		-- <xfer-optimization-1.3>
	})

test:do_execsql_test(
	"xfer-oprimization-1.4",
	[[
		SELECT * FROM t2;
	]], {
		-- <xfer-optimization-1.4>
		1, 1, 2, 2, 3, 3
		-- <xfer-optimization-1.4>
	})

test:do_catchsql_test(
	"xfer-optimization-1.5",
	[[
		DROP TABLE t1;
		DROP TABLE t2;
		CREATE TABLE t1(a INTEGER PRIMARY KEY, b INTEGER, c INTEGER);
		INSERT INTO t1 VALUES (1, 1, 2), (2, 2, 3), (3, 3, 4);
		CREATE TABLE t2(a INTEGER PRIMARY KEY, b INTEGER);
		INSERT INTO t2 SELECT * FROM t1;
	]], {
		-- <xfer-optimization-1.5>
		1, "table T2 has 2 columns but 3 values were supplied"
		-- <xfer-optimization-1.5>
	})

test:do_execsql_test(
	"xfer-oprimization-1.6",
	[[
		SELECT * FROM t2;
	]], {
		-- <xfer-oprimization-1.6>

		-- <xfer-oprimization-1.6>
	})

test:do_catchsql_test(
	"xfer-optimization-1.7",
	[[
		DROP TABLE t1;
		DROP TABLE t2;
		CREATE TABLE t1(a INTEGER PRIMARY KEY, b INTEGER);
		INSERT INTO t1 VALUES (1, 1), (2, 2), (3, 3);
		CREATE TABLE t2(a INTEGER PRIMARY KEY, b INTEGER);
		INSERT INTO t2 SELECT * FROM t1;
	]], {
		-- <xfer-optimization-1.7>
		0
		-- <xfer-optimization-1.7>
	})

test:do_execsql_test(
	"xfer-oprimization-1.8",
	[[
		SELECT * FROM t2;
	]], {
		-- <xfer-oprimization-1.6>
		1, 1, 2, 2, 3, 3
		-- <xfer-oprimization-1.6>
	})

test:do_catchsql_test(
	"xfer-optimization-1.9",
	[[
		DROP TABLE t1;
		DROP TABLE t2;
		CREATE TABLE t1(a INTEGER PRIMARY KEY, b INTEGER);
		INSERT INTO t1 VALUES (1, 1), (2, 2), (3, 2);
		CREATE TABLE t2(b INTEGER, a INTEGER PRIMARY KEY);
		INSERT INTO t2 SELECT * FROM t1;
	]], {
		-- <xfer-optimization-1.9>
		1, "Duplicate key exists in unique index 'sqlite_autoindex_T2_1' in space 'T2'"
		-- <xfer-optimization-1.9>
	})

test:do_execsql_test(
	"xfer-oprimization-1.10",
	[[
		SELECT * FROM t2;
	]], {
		-- <xfer-oprimization-1.10>

		-- <xfer-oprimization-1.10>
	})

test:do_catchsql_test(
	"xfer-optimization-1.11",
	[[
		DROP TABLE t1;
		DROP TABLE t2;
		CREATE TABLE t1(a INTEGER PRIMARY KEY, b INTEGER);
		INSERT INTO t1 VALUES (1, 1), (2, 2), (3, 2);
		CREATE TABLE t2(b INTEGER PRIMARY KEY, a INTEGER);
		INSERT INTO t2 SELECT * FROM t1;
	]], {
		-- <xfer-optimization-1.11>
		0
		-- <xfer-optimization-1.11>
	})

test:do_execsql_test(
	"xfer-oprimization-1.12",
	[[
		SELECT * FROM t2;
	]], {
		-- <xfer-oprimization-1.12>
		1, 1, 2, 2, 3, 2
		-- <xfer-oprimization-1.12>
	})

test:do_catchsql_test(
	"xfer-optimization-1.13",
	[[
		DROP TABLE t1;
		DROP TABLE t2;
		CREATE TABLE t1(a INTEGER PRIMARY KEY, b UNIQUE ON CONFLICT REPLACE);
		CREATE TABLE t2(a INTEGER PRIMARY KEY, b UNIQUE ON CONFLICT REPLACE);
		INSERT INTO t1 VALUES (3, 3), (4, 4), (5, 5);
		INSERT INTO t2 VALUES (1, 1), (2, 2);
		INSERT INTO t2 SELECT * FROM t1;
	]], {
		-- <xfer-optimization-1.13>
		0
		-- <xfer-optimization-1.13>
	})

test:do_execsql_test(
	"xfer-oprimization-1.14",
	[[
		SELECT * FROM t2;
	]], {
		-- <xfer-optimization-1.14>
		1, 1, 2, 2, 3, 3, 4, 4, 5, 5
		-- <xfer-optimization-1.14>
	})

test:do_catchsql_test(
	"xfer-optimization-1.15",
	[[
		DROP TABLE t1;
		DROP TABLE t2;
		CREATE TABLE t1(a INTEGER PRIMARY KEY, b UNIQUE ON CONFLICT REPLACE);
		CREATE TABLE t2(a INTEGER PRIMARY KEY, b UNIQUE ON CONFLICT REPLACE);
		INSERT INTO t1 VALUES (2, 2), (3, 3), (5, 5);
		INSERT INTO t2 VALUES (1, 1), (4, 4);
		INSERT OR ROLLBACK INTO t2 SELECT * FROM t1;
	]], {
		-- <xfer-optimization-1.15>
		0
		-- <xfer-optimization-1.15>
	})

test:do_execsql_test(
	"xfer-oprimization-1.16",
	[[
		SELECT * FROM t2;
	]], {
		-- <xfer-oprimization-1.16>
		1, 1, 2, 2, 3, 3, 4, 4, 5, 5
		-- <xfer-oprimization-1.16>
	})

test:do_catchsql_test(
	"xfer-optimization-1.17",
	[[
		DROP TABLE t1;
		DROP TABLE t2;
		CREATE TABLE t1(a INTEGER PRIMARY KEY, b UNIQUE ON CONFLICT REPLACE);
		CREATE TABLE t2(a INTEGER PRIMARY KEY, b UNIQUE ON CONFLICT REPLACE);
		INSERT INTO t1 VALUES (1, 2), (3, 3), (5, 5);
		INSERT INTO t2 VALUES (1, 1), (4, 4);
		INSERT OR REPLACE INTO t2 SELECT * FROM t1;
	]], {
		-- <xfer-optimization-1.17>
		0
		-- <xfer-optimization-1.17>
	})

test:do_execsql_test(
	"xfer-oprimization-1.18",
	[[
		SELECT * FROM t2;
	]], {
		-- <xfer-oprimization-1.18>
		1, 2, 3, 3, 4, 4, 5, 5
		-- <xfer-oprimization-1.18>
	})

test:finish_test()
