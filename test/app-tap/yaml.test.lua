#!/usr/bin/env tarantool

package.path = "lua/?.lua;"..package.path

local tap = require('tap')
local common = require('serializer_test')

local function is_map(s)
    return s:match("---[\n ]%w+%:") or s:match("---[\n ]{%w+%:")
end

local function is_array(s)
    return s:match("---[\n ]%[") or s:match("---[\n ]- ");
end

local function test_compact(test, s)
    test:plan(9)

    local ss = s.new()
    ss.cfg{encode_load_metatables = true, decode_save_metatables = true}

    test:is(ss.encode({10, 15, 20}), "---\n- 10\n- 15\n- 20\n...\n",
        "block array")
    test:is(ss.encode(setmetatable({10, 15, 20}, { __serialize="array"})),
        "---\n- 10\n- 15\n- 20\n...\n", "block array")
    test:is(ss.encode(setmetatable({10, 15, 20}, { __serialize="sequence"})),
        "---\n- 10\n- 15\n- 20\n...\n", "block array")
    test:is(ss.encode({setmetatable({10, 15, 20}, { __serialize="seq"})}),
        "---\n- [10, 15, 20]\n...\n", "flow array")
    test:is(getmetatable(ss.decode(ss.encode({10, 15, 20}))).__serialize, "seq",
        "decoded __serialize is seq")

    test:is(ss.encode({k = 'v'}), "---\nk: v\n...\n", "block map")
    test:is(ss.encode(setmetatable({k = 'v'}, { __serialize="mapping"})),
        "---\nk: v\n...\n", "block map")
    test:is(ss.encode({setmetatable({k = 'v'}, { __serialize="map"})}),
        "---\n- {'k': 'v'}\n...\n", "flow map")
    test:is(getmetatable(ss.decode(ss.encode({k = 'v'}))).__serialize, "map",
        "decoded __serialize is map")

    ss = nil
end

local function test_output(test, s)
    test:plan(12)
    test:is(s.encode({true}), '---\n- true\n...\n', "encode for true")
    test:is(s.decode("---\nyes\n..."), true, "decode for 'yes'")
    test:is(s.encode({false}), '---\n- false\n...\n', "encode for false")
    test:is(s.decode("---\nno\n..."), false, "decode for 'no'")
    test:is(s.encode({s.NULL}), '---\n- null\n...\n', "encode for nil")
    test:is(s.decode("---\n~\n..."), s.NULL, "decode for ~")
    test:is(s.encode("\x80\x92\xe8s\x16"), '--- !!binary gJLocxY=\n...\n',
        "encode for binary")
    test:is(s.encode("\x08\x5c\xc2\x80\x12\x2f"), '--- !!binary CFzCgBIv\n...\n',
        "encode for binary (2) - gh-354")
    test:is(s.encode("\xe0\x82\x85\x00"), '--- !!binary 4IKFAA==\n...\n',
        "encode for binary (3) - gh-1302")
    -- gh-883: console can hang tarantool process
    local t = {}
    for i=0x8000,0xffff,1 do
        table.insert(t, require('pickle').pack( 'i', i ));
    end
    local _, count = string.gsub(s.encode(t), "!!binary", "")
    test:is(count, 30880, "encode for binary (4) - gh-883")
    test:is(s.encode("фЫр!"), '--- фЫр!\n...\n',
        "encode for utf-8")

    test:is(s.encode("Tutorial -- Header\n====\n\nText"),
        "--- |-\n  Tutorial -- Header\n  ====\n\n  Text\n...\n", "tutorial string");
end

local function test_tagged(test, s)
    test:plan(17)
    --
    -- Test encoding tags.
    --
    local prefix = 'tag:tarantool.io/push,2018'
    local ok, err = pcall(s.encode_tagged)
    test:isnt(err:find('Usage'), nil, "encode_tagged usage")
    ok, err = pcall(s.encode_tagged, 100, {})
    test:isnt(err:find('Usage'), nil, "encode_tagged usage")
    ok, err = pcall(s.encode_tagged, 200, {handle = true, prefix = 100})
    test:isnt(err:find('Usage'), nil, "encode_tagged usage")
    local ret
    ret, err = s.encode_tagged(300, {handle = '!push', prefix = prefix})
    test:is(ret, nil, 'non-usage and non-oom errors do not raise')
    test:is(err, "tag handle must end with '!'", "encode_tagged usage")
    ret = s.encode_tagged(300, {handle = '!push!', prefix = prefix})
    test:is(ret, "%TAG !push! "..prefix.."\n--- 300\n...\n", "encode_tagged usage")
    ret = s.encode_tagged({a = 100, b = 200}, {handle = '!print!', prefix = prefix})
    test:is(ret, "%TAG !print! tag:tarantool.io/push,2018\n---\na: 100\nb: 200\n...\n", 'encode_tagged usage')
    --
    -- Test decoding tags.
    --
    ok, err = pcall(s.decode)
    test:isnt(err:find('Usage'), nil, "decode usage")
    ok, err = pcall(s.decode, false)
    test:isnt(err:find('Usage'), nil, "decode usage")
    local handle, prefix = s.decode(ret, {tag_only = true})
    test:is(handle, "!print!", "handle is decoded ok")
    test:is(prefix, "tag:tarantool.io/push,2018", "prefix is decoded ok")
    local several_tags =
[[%TAG !tag1! tag:tarantool.io/tag1,2018
%TAG !tag2! tag:tarantool.io/tag2,2018
---
- 100
...
]]
    ok, err = s.decode(several_tags, {tag_only = true})
    test:is(ok, nil, "can not decode multiple tags")
    test:is(err, "can not decode multiple tags", "same")
    local no_tags = s.encode(100)
    handle, prefix = s.decode(no_tags, {tag_only = true})
    test:is(handle, nil, "no tag - no handle")
    test:is(prefix, nil, "no tag - no prefix")
    local several_documents =
[[
%TAG !tag1! tag:tarantool.io/tag1,2018
--- 1
...

%TAG !tag2! tag:tarantool.io/tag2,2018
--- 2
...

]]
    handle, prefix = s.decode(several_documents, {tag_only = true})
    test:is(handle, "!tag1!", "tag handle on multiple documents")
    test:is(prefix, "tag:tarantool.io/tag1,2018",
            "tag prefix on multiple documents")
end

tap.test("yaml", function(test)
    local serializer = require('yaml')
    test:plan(11)
    test:test("unsigned", common.test_unsigned, serializer)
    test:test("signed", common.test_signed, serializer)
    test:test("double", common.test_double, serializer)
    test:test("boolean", common.test_boolean, serializer)
    test:test("string", common.test_string, serializer)
    test:test("nil", common.test_nil, serializer)
    test:test("table", common.test_table, serializer, is_array, is_map)
    test:test("ucdata", common.test_ucdata, serializer)
    test:test("compact", test_compact, serializer)
    test:test("output", test_output, serializer)
    test:test("tagged", test_tagged, serializer)
end)
