test_run = require('test_run').new()

fiber = require 'fiber'

log = require('log')

math.randomseed(os.time())

s = box.schema.space.create('test', {engine = 'vinyl'})
_ = s:create_index('pk', {parts = {1, 'unsigned'}})
_ = s:create_index('i1', {unique = false, parts = {2, 'unsigned', 3, 'unsigned'}})
_ = s:create_index('i2', {unique = false, parts = {2, 'unsigned', 4, 'unsigned'}})

--
-- If called from a transaction, i1:select({k}) and i2:select({k})
-- must yield the same result. Let's check that under a stress load.
--

MAX_KEY = 100
MAX_VAL = 10

test_run:cmd("setopt delimiter ';'")

function gen_insert()
    local t = {math.random(MAX_KEY), math.random(MAX_VAL),
                        math.random(MAX_VAL), math.random(MAX_VAL), 1}
    pcall(s.replace, s, t)
end;

function gen_delete()
    pcall(s.delete, s, math.random(MAX_KEY))
end;

function gen_update()
    pcall(s.update, s, math.random(MAX_KEY), {{'+', 5, 1}})
end;

function dml_loop()
    while not stop do
        gen_insert()
--gen_update()
--gen_delete()
        fiber.sleep(0)
    end
    ch:put(true)
end;

function snap_loop()
    while not stop do
        box.snapshot()
        fiber.sleep(0.1)
    end
    ch:put(true)
end;

stop = false;
ch = fiber.channel(3);

_ = fiber.create(dml_loop);
_ = fiber.create(dml_loop);
_ = fiber.create(snap_loop);

failed = {};

for i = 1, 10000 do
    local val = math.random(MAX_VAL)
    log.info('start select')
    box.begin()
    local res1 = s.index.i1:select({val})
    local res2 = s.index.i2:select({val})
    box.commit()
    log.info('finish select')
    local equal = true
    if #res1 == #res2 then
        for _, t1 in ipairs(res1) do
            local found = false
            for _, t2 in ipairs(res2) do
                if t1[1] == t2[1] then
                    found = true
                    break
                end
            end
            if not found then
                equal = false
                break
            end
        end
    else
        equal = false
    end
    if not equal then
        table.insert(failed, {res1, res2})
        table.insert(failed, {#res1, #res2})
        break
    end
    fiber.sleep(0)
end;

stop = true;
for i = 1, ch:size() do
    ch:get()
end;

function compare(pk, sk)
    for _, pk_tuple in pairs(pk) do
        local found = false
        for _, sk_tuple in pairs(sk) do
            if sk_tuple[1] == pk_tuple[1] then
                found = true
                break
            end
        end
        if not found then
            log.info(pk_tuple)
        end
    end
end;

test_run:cmd("setopt delimiter ''");

failed

-- #failed == 0 or failed
log.info('cmp pk and sk')
compare(failed[1][1], failed[1][2])
log.info('cmp sk and pk')
compare(failed[1][2], failed[1][1])

-- s:drop()
