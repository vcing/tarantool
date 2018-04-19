test_run = require('test_run').new()
--
-- gh-2677: box.session.push binary protocol tests.
--

--
-- Usage.
--
box.session.push()
box.session.push(1, 2)

fiber = require('fiber')
messages = {}
test_run:cmd("setopt delimiter ';'")
function do_pushes()
    for i = 1, 5 do
        box.session.push(i)
        fiber.sleep(0.01)
    end
    return 300
end;
test_run:cmd("setopt delimiter ''");

netbox = require('net.box')
box.schema.user.grant('guest', 'read,write,execute', 'universe')

c = netbox.connect(box.cfg.listen)
c:ping()
c:call('do_pushes', {}, {on_push = table.insert, on_push_ctx = messages})
messages

-- Add a little stress: many pushes with different syncs, from
-- different fibers and DML/DQL requests.

catchers = {}
started = 0
finished = 0
s = box.schema.create_space('test', {format = {{'field1', 'integer'}}})
pk = s:create_index('pk')
c:reload_schema()
test_run:cmd("setopt delimiter ';'")
function dml_push_and_dml(key)
    box.session.push('started dml')
    s:replace{key}
    box.session.push('continued dml')
    s:replace{-key}
    box.session.push('finished dml')
    return key
end;
function do_pushes(val)
    for i = 1, 5 do
        box.session.push(i)
        fiber.yield()
    end
    return val
end;
function push_catcher_f()
    fiber.yield()
    started = started + 1
    local catcher = {messages = {}, retval = nil, is_dml = false}
    catcher.retval = c:call('do_pushes', {started},
                            {on_push = table.insert,
                             on_push_ctx = catcher.messages})
    table.insert(catchers, catcher)
    finished = finished + 1
end;
function dml_push_and_dml_f()
    fiber.yield()
    started = started + 1
    local catcher = {messages = {}, retval = nil, is_dml = true}
    catcher.retval = c:call('dml_push_and_dml', {started},
                            {on_push = table.insert,
                             on_push_ctx = catcher.messages})
    table.insert(catchers, catcher)
    finished = finished + 1
end;
-- At first check that a pushed message can be ignored in a binary
-- protocol too.
c:call('do_pushes', {300});
-- Then do stress.
for i = 1, 200 do
    fiber.create(dml_push_and_dml_f)
    fiber.create(push_catcher_f)
end;
while finished ~= 400 do fiber.sleep(0.1) end;

for _, c in pairs(catchers) do
    if c.is_dml then
        assert(#c.messages == 3, 'dml sends 3 messages')
        assert(c.messages[1] == 'started dml', 'started')
        assert(c.messages[2] == 'continued dml', 'continued')
        assert(c.messages[3] == 'finished dml', 'finished')
        assert(s:get{c.retval}, 'inserted +')
        assert(s:get{-c.retval}, 'inserted -')
    else
        assert(c.retval, 'something is returned')
        assert(#c.messages == 5, 'non-dml sends 5 messages')
        for k, v in pairs(c.messages) do
            assert(k == v, 'with equal keys and values')
        end
    end
end;
test_run:cmd("setopt delimiter ''");

#s:select{}

--
-- Ok to push NULL.
--
function push_null() box.session.push(box.NULL) end
messages = {}
c:call('push_null', {}, {on_push = table.insert, on_push_ctx = messages})
messages

--
-- Test binary pushes.
--
ibuf = require('buffer').ibuf()
msgpack = require('msgpack')
messages = {}
resp_len = c:call('do_pushes', {300}, {on_push = table.insert, on_push_ctx = messages, buffer = ibuf})
resp_len
messages
decoded = {}
r = nil
for i = 1, #messages do r, ibuf.rpos = msgpack.decode_unchecked(ibuf.rpos) table.insert(decoded, r) end
decoded
r, _ = msgpack.decode_unchecked(ibuf.rpos)
r

c:close()
s:drop()

box.schema.user.revoke('guest', 'read,write,execute', 'universe')

--
-- Ensure can not push in background.
--
ok = nil
err = nil
f = fiber.create(function() ok, err = box.session.push(100) end)
while f:status() ~= 'dead' do fiber.sleep(0.01) end
ok, err
