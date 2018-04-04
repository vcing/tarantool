# Tarantool Wire protocol

* **Status**: In progress
* **Start date**: 04-04-2018
* **Authors**: Vladislav Shpilevoy @Gerold103 v.shpilevoy@tarantool.org, Konstantin Osipov @kostja kostja@tarantool.org, Alexey Gadzhiev @alg1973 alg1973@gmail.com
* **Issues**: [#2677](https://github.com/tarantool/tarantool/issues/2677), [#2620](https://github.com/tarantool/tarantool/issues/2620), [#2618](https://github.com/tarantool/tarantool/issues/2618)

## Summary

Tarantool wire protocol is a convention how to encode and send results of execution of SQL, Lua and C stored functions, DML (Data Manipulation Language), DDL (Data Definition Language), DQL (Data Query Language) requests to remote clients via network. The protocol is unified for all request types. For a single request multiple responses of different types can be sent.

## Background and motivation

Tarantool wire protocol is called **IProto**, and is used by database connectors written on different languages and working on remote clients. The protocol describes how to distinguish different message types and what data can be stored in each message. Tarantool has the following response types:
* A response, that finalizes a request, and has just data - tuples array, or scalar values, or mixed. It has no any metadata. This response type incorporates results of any pure Lua and C calls including stored procedures, space and index methods. Such response is single per request;
* A response with just data, but with no request finalization - it is so called push-message. During single request execution multiple pushes can be sent, and they do not finalize the request - a client must be ready to receive more responses;
* A formatted response, that is sent on SQL DQL and does not finalize a request. Such response contains metadata with result set column names, types, flags etc;
* A response with metadata only, that is sent on SQL DDL/DML requests, and contains affected row count, last autoincrement column value, flags;
* A response with error message and code, that finalizes a request.

In supporting this responses set 2 main challenges appear:
1. How to unify responses;
2. How to support multiple messages inside a single request.

To understand how a single request can produce multiple responses, consider the stored procedure (do not pay attention to the syntax - it does not matter here):
```SQL
FUNCTION my_sql_func(a1, a2, a3, a4) BEGIN
    SELECT my_lua_func(a1);
    SELECT * FROM table1;
    SELECT my_c_func(a2);
    INSERT INTO table1 VALUES (1, 2, 3);
    RETURN a4;
END
```
, where `my_lua_func()` is the function, written in Lua and sending its own push-messages:
```Lua
function my_lua_func(arg)
    box.session.push(arg)
    return arg
end
```
and `my_c_func()` is the function, written in C and returning some raw data:
```C
int
my_c_func(box_function_ctx_t *ctx) {
    box_tuple_t *tuple;
    /* Fill a tuple with any data. */
    return box_return_tuple(ctx, tuple);
}
```
Consider each statement:
* `SELECT FROM` can split a big result set in multiple messages;
* `SELECT my_lua_func()` produces 2 messages: one is the push-message generated in `my_lua_func` and another is the result of `SELECT` itself;
* `INSERT` creates 1 message with metadata;
* `RETURN` creates a final response message.

Of course, some of messages, or even all of them can be batched and send as a single TCP packet, but it does not matter for the wire protocol.

In the next section it is described, how the Tarantool wire protocol deals with this mess.

For the protocol details - code values, all header and body keys - see Tarantool [website](tarantool.io).

## Detailed design

Tarantool response consists of a body and a header. Header is used to store response code and some internal metainfo such as schema version, request id (called **sync** in Tarantool). Body is used to store result data and request-dependent metainfo.

### Header

There are 3 response codes in header:
* `IPROTO_OK` - the last response in a request, that is finished successfully;
* `IPROTO_CHUNK` - non-final response. One request can generate multuple chunk messages;
* `IPROTO_ERROR | error code` - the last response in a request, that is finished with an error.

`IPROTO_ERROR` response is trivial, and consists just of code and message. It is no considered further.
`IPROTO_OK` and `IPROTO_CHUNK` have the same body format. But
1. `IPROTO_OK` finalizes a request;
2. `IPROTO_CHUNK` can have `IPROTO_CHUNK_ID` field in the header, that allows to build a chain of chunks with the same `ID`. Absense of this field means, that the chunk is not a part of a chain.

### Body

The common body structure:
```
+----------------------------------------------+
| IPROTO_BODY: {                               |
|     IPROTO_METADATA: [                       |
|         {                                    |
|             IPROTO_FIELD_NAME: string,       |
|             IPROTO_FIELD_TYPE: number,       |
|             IPROTO_FIELD_FLAGS: number,      |
|         },                                   |
|         ...                                  |
|     ],                                       |
|                                              |
|     IPROTO_SQL_INFO: {                       |
|         SQL_INFO_ROW_COUNT: number,          |
|         SQL_INFO_LAST_ID: number,            |
|         ...                                  |
|     },                                       |
|                                              |
|     IPROTO_DATA: [                           |
|         tuple/scalar,                        |
|         ...                                  |
|     ]                                        |
| }                                            |
+----------------------------------------------+
```

Consider, how different responses use the body, and how they can be distinguished.

_A non formatted response_ has only `IPROTO_DATA` key in a body. It is the result of Lua and C DML, DDL, DQL, stored procedures calls, push messages. Such response is never linked with next or previous messages of the same request.

_A non formatted response with metadata_ has only `IPROTO_SQL_INFO` and it is always result of DDL/DML executed via SQL. As well as the previous type, this response is all-independent.

_A formatted response_ always has `IPROTO_DATA`, and can have both `IPROTO_SQL_INFO` and `IPROTO_METADATA`. It can be result of SQL DQL (`SELECT`) or SQL DML (`INSERT` with human readable response like `NN rows inserted/updated/deleted`). The response can be part of a continuous sequence of responses. A first message of the sequence always contains `IPROTO_METADATA` in the body and `IPROTO_CHUNK_ID` in the header, if there are multiple responses. All sequence chunks always contain `IPROTO_CHUNK_ID` with the same value.

On the picture the state machine of the protocol is showed:
![alt text](https://raw.githubusercontent.com/tarantool/tarantool/gh-3328-new-iproto/doc/rfc/3328-wire_protocol_img1.svg?sanitize=true)

For the `FUNCTION my_sql_func` call the following responses are sent:
```
/* Push from my_lua_func(a1). */
+----------------------------------------------+
| HEADER: IPROTO_CHUNK                         |
+- - - - - - - - - - - - - - - - - - - - - - - +
| BODY: {                                      |
|     IPROTO_DATA: [ a1 ]                      |
| }                                            |
+----------------------------------------------+

/* Result of SELECT my_lua_func(a1). */
+----------------------------------------------+
| HEADER: IPROTO_CHUNK                         |
+- - - - - - - - - - - - - - - - - - - - - - - +
| BODY: {                                      |
|     IPROTO_DATA: [ [ a1 ] ],                 |
|     IPROTO_METADATA: [                       |
|         { /* field name, type ... */ }       |
|     ]                                        |
| }                                            |
+----------------------------------------------+

/* First chunk of SELECT * FROM table1. */
+----------------------------------------------+
| HEADER: IPROTO_CHUNK, IPROTO_CHUNK_ID = <id1>|
+- - - - - - - - - - - - - - - - - - - - - - - +
| BODY: {                                      |
|    IPROTO_DATA: [ tuple1, tuple2, ... ]      |
|    IPROTO_METADATA: [                        |
|        { /* field1 name, type ... */ },      |
|        { /* field2 name, type ... */ },      |
|        ...                                   |
|    ]                                         |
| }                                            |
+----------------------------------------------+

        /* From second to next to last chunk. */
	+----------------------------------------------+
	| HEADER: IPROTO_CHUNK, IPROTO_CHUNK_ID = <id1>|
	+- - - - - - - - - - - - - - - - - - - - - - - +
	| BODY: {                                      |
	|    IPROTO_DATA: [ tuple1, tuple2, ... ]      |
	| }                                            |
	+----------------------------------------------+

	/* Last chunk. */
	+----------------------------------------------+
	| HEADER: IPROTO_CHUNK, IPROTO_CHUNK_ID = <id1>|
	+- - - - - - - - - - - - - - - - - - - - - - - +
	| BODY: {                                      |
	|    IPROTO_DATA: [ tuple1, tuple2, ... ]      |
	| }                                            |
	+----------------------------------------------+

/* Result of SELECT my_c_func(a2). */
+----------------------------------------------+
| HEADER: IPROTO_CHUNK                         |
+- - - - - - - - - - - - - - - - - - - - - - - +
| BODY: {                                      |
|     IPROTO_DATA: [ [ tuple ] ],              |
|     IPROTO_METADATA: [                       |
|         { /* field name, type ... */ }       |
|     ]                                        |
| }                                            |
+----------------------------------------------+

/* Result of INSERT INTO table1 VALUES (1, 2, 3). */
+----------------------------------------------+
| HEADER: IPROTO_CHUNK                         |
+- - - - - - - - - - - - - - - - - - - - - - - +
| BODY: {                                      |
|     IPROTO_SQL_INFO: {                       |
|         SQL_INFO_ROW_COUNT: number,          |
|         SQL_INFO_LAST_ID: number,            |
|     }                                        |
| }                                            |
+----------------------------------------------+

/* Result of RETURN a4 */
+----------------------------------------------+
| HEADER: IPROTO_OK                            |
+- - - - - - - - - - - - - - - - - - - - - - - +
| BODY: {                                      |
|     IPROTO_DATA: [ a4 ]                      |
| }                                            |
+----------------------------------------------+
```

## Rationale and alternatives

Another way to link chunks together exists, replacing `IPROTO_CHUNK_ID`.
Chunks can be linked via flag in a header: `IPROTO_FLAG_IS_CHAIN`, that would be stored in `IPROTO_FLAGS` header value. When a multiple messages form a chain, all of them except last one contain this flag. For example:
```
IPROTO_CHUNK
    |
IPROTO_CHUNK, IS_CHAIN
    |
    +--IPROTO_CHUNK, IS_CHAIN
    |
    +--IPROTO_CHUNK, IS_CHAIN
    |
    +--IPROTO_CHUNK
    |
IPROTO_CHUNK
    |
...
    |
IPROTO_OK/ERROR 
```

It is slightly simpler than `CHAIN_ID`, but
1. Does not enable to mix parts of different chains, if it will be needed sometimes;
2. The last response does not contain `IS_CHAIN`, but it is actually a part of chain. `IS_CHAIN` can not be stored in the last response, because else it will not be distinguishable from the next chain. This can be solved by renaming `IS_CHAIN` to `HAS_NEXT_CHAIN` or something, but `CHAIN_ID` seems better - it has no these problems, and is more scalable.
