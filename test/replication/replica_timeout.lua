#!/usr/bin/env tarantool

local TIMEOUT = tonumber(arg[1])

box.cfg({
    listen              = os.getenv("LISTEN"),
    replication         = os.getenv("MASTER"),
    memtx_memory        = 107374182,
    replication_timeout = TIMEOUT,
    replication_connect_timeout = TIMEOUT * 3,
})

require('console').listen(os.getenv('ADMIN'))
