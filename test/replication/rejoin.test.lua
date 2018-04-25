env = require('test_run')
test_run = env.new()

test_run:cmd('restart server default with cleanup=1')
test_run:cmd('switch default')
box.schema.user.grant('guest', 'replication')

test_run:cmd("create server replica with rpl_master=default, script='replication/replica.lua'")

_ = box.schema.space.create("test")
_ = box.space.test:create_index("primary")
box.space.test:insert{1}

test_run:cmd("start server replica")
test_run:cmd('switch replica')
box.space.test:select{}
test_run:cmd('switch default')
test_run:cmd("stop server replica")
box.space.test:insert{2}
box.snapshot()
fio = require("fio")
fio.unlink(fio.pathjoin(fio.abspath("."), string.format('master/%020d.xlog', 6)))

test_run:cmd("start server replica")
test_run:cmd('switch replica')
test_run:grep_log("replica", "Missing .xlog file between LSN") ~= nil
fiber = require("fiber")
while box.space.test:count() < 2 do fiber.sleep(0.001) end
box.space.test:select{}

test_run:cmd('switch default')
master_id = test_run:get_server_id('default')
replica_id = test_run:get_server_id('replica')
box.info.replication[master_id].lsn == box.info.replication[replica_id].downstream.vclock[master_id]
box.schema.user.revoke('guest', 'replication')
test_run:cmd("stop server replica")
test_run:cmd("cleanup server replica")
box.space.test:drop()
