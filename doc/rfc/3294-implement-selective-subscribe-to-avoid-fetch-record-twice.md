# Implement selective subscribe and subscription daemon

* **Status**: In progress
* **Start date**: 25-04-2018
* **Authors**: Konstantin Belyavskiy @kbelyavs k.belyavskiy@tarantool.org, Georgy Kirichenko @georgy georgy@tarantool.org, Konstantin Osipov @kostja kostja@tarantool.org
* **Issues**: [#3294](https://github.com/tarantool/tarantool/issues/3294)

## Summary

Extend SUBSCRIBE command with a list of server UUIDs for which SUBSCRIBE should fetch changes. In a full mesh configuration, only download records originating from the immediate peer. Do not download the records from other peers twice.
Implement subscription daemon, each time a server responsible feeding more than 1 server id is dropped, we need to re-subscribe to some other peer and reassign the dropped ids to that peer. Each time a server is connected again, we need to rebalance again.

## Background and motivation

Currently each Tarantool instance will download from all peers all records in their WAL except records with instance id equal to the self instance id. For example, in a full mesh of 3 replicas all record will be fetched twice. Instead, it could send a subscribe request to its peers with server ids which are not present in other subscribe requests.

## Detailed design

1. Extend IPROTO_SUBSCRIBE command with a list of server UUIDs for which SUBSCRIBE should fetch changes. Store this UUIDs within applier's internal data structure. By default issuing SUBSCRIBE with empty list what means no filtering at all.
2. Implement white-list filtering in relay. After processing SUBSCRIBE request, relay has a list of UUIDs. Extract associated peer ids and fill in a filter. By default transmit all records, unless SUBSCRIBE was done with at least one server UUID. In latter case drop all records except originating from replicas in this list.
3. After issuing REQUEST_VOTE to all peers, subscription daemon knows a map of server ids, their peers and their vclocks. Sort the map by server id. Iterate over each server in the list of peers and assign its id to this server's SUBSCRIBE request. Assign all the remaining ids to the last peer (alternatively, if there are many ids in the remainder, keep going through the list of server and assign "orphan" ids in round-robin fashion).
Issue the subscribe request.
After this feature is implemented, each time a server responsible feeding more than 1 server id is dropped, we need to re-subscribe to some other peer and reassign the dropped ids to that peer. Each time a server is connected again, we need to rebalance again.
4. Rebalancing. Connect/disconnect should trigger daemon to start reassigning process.
 - On disconnect first get a list of all UUIDs, then iterate through appliers to find "orphan" and finally reassigned these UUIDs to last peer by issuing SUBSCRIBE for it.
 - On connect, by iterating through appliers list, find stolen UUIDs, reassign them to correct applier and issue SUBSCRIBE for recently connected applier and for the one from whom we get these UUIDs back.
