env = require('test_run')
test_run = env.new()

SERVERS = { 'master1', 'master2', 'master3' }
-- Start servers
test_run:create_cluster(SERVERS)
-- Check connection status
-- first on master 1
test_run:cmd("switch master1")
fiber = require('fiber')
while box.info.replication.status ~= 'follow' do fiber.sleep(0.001) end
box.info.replication.status
-- and then on master 2
test_run:cmd("switch master2")
fiber = require('fiber')
while box.info.replication.status ~= 'follow' do fiber.sleep(0.001) end
box.info.replication.status
-- and finally on master 3
test_run:cmd("switch master3")
fiber = require('fiber')
while box.info.replication.status ~= 'follow' do fiber.sleep(0.001) end
box.info.replication.status

test_run:cmd("switch master1")
box.snapshot()
box.space.test:insert({1})
box.space.test:count()

test_run:cmd("switch master3")
box.space.test:count()
test_run:cmd("switch master2")
box.space.test:count()
test_run:cmd("stop server master1")
fio = require('fio')
fio.unlink(fio.pathjoin(fio.abspath("."), string.format('master1/%020d.xlog', 0)))
test_run:cmd("start server master1")

test_run:cmd("switch master1")
box.space.test:count()

test_run:cmd("switch default")
test_run:cmd("stop server master1")
test_run:cmd("stop server master2")
test_run:cmd("stop server master3")
test_run:cmd("cleanup server master1")
test_run:cmd("cleanup server master2")
test_run:cmd("cleanup server master3")
