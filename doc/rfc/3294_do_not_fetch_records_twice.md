# Topology Discovering Protocol

* **Status**: In progress
* **Start date**: 25-04-2018
* **Authors**: Konstantin Belyavskiy @kbelyavs k.belyavskiy@tarantool.org, Georgy Kirichenko @georgy georgy@tarantool.org, Konstantin Osipov @kostja kostja@tarantool.org
* **Issues**: [#3294](https://github.com/tarantool/tarantool/issues/3294)

## Summary

Propose a new protocol for dynamic discovering/maintaining a directed graph of connected Tarantool nodes. Based on collected information, implement selective SUBSCRIBE to avoid duplicated data transfer between nodes. Maintain network consistency: each time a server responsible feeding more than 1 server id is dropped, we need to re-subscribe to some other peer and reassign the dropped ids to that peer. Each time a server is connected again, we need to rebalance again.
This Draft covering following topics:
- Discovering and maintaining current network topology. Protocol describing how individual peer can observe topology and defining each node responsibility.
- Selective Subscribe. Extend SUBSCRIBE command with a list of server UUIDs for which SUBSCRIBE should fetch changes. In a full mesh configuration, only download records originating from the immediate peer. Do not download the records from other peers twice.
- Implement subscription daemon, maintaining subscriptions based on known current network topology.

## Background and motivation

Currently each Tarantool instance will download from all peers all records in their WAL except records with instance id equal to the self instance id. For example, in a full mesh of 3 replicas all record will be fetched twice. Instead, it could send a subscribe request to its peers with server ids which are not present in other subscribe requests.
In more complex case, if there is no direct connection between two nodes, to subscribe through intermediate peers we should know network topology. So the first task is to build a topology.

## High-level design

Building such topology is possible based on following principles:
- Each node responsible for notify all his downstream peers (replicas) in case of changes with his upstream subscription configuration (we do not transfer all knowing topology, but only the subset based on node self subscription list, exclude part originating from peer which we are going to notify). Only downstream peers should be notified, as it's a directed graph. In case of master-master each master is a downstream for another, so also topology update should be send.
- Each node on connect responsible to request every upstream peer with a list of its own subscription topology and based on provided data issue subscribe requests.
- The connection with lesser count of intermediate nodes has the highest priority. Lets define the number of edges between two peers as a level. So if A has direct connection with B, then level is 1 and if A connected with C through B, then level is 2.
- In case of equal level connections first win. But if shorter path is found, then node first should reconnect and then notify downstream peers with updated paths.

So peer notifies his downstream peers with topology as a map of {UUID: level}. When transmitting to next level, increment level by one.

Details and open questions.
1. On connect (new client or the old one reconnects).
Who first? Master provide data to replica or replica requests for data? To think about backward compatibilities issues.
How does replica connect to master? Steps? I think after issuing REQUEST_VOTE, upstream can response with a map and then replica subscribes.
2. Map of *{UUID: depth}* (depth is a number of edges). Should I send *{self: 0}* or not? I think no, since we already know direct peers. Also should not send to peer list of UUIDs obtaining from it.
3. Balancing? It is possible to slightly extend topology with number of peers subscribed for Balancing but does it really needed?
4. On network configuration change what first, to notify peers or try to resubscribe?
   a. On shorter path found, first resubscribe, then notify downstream peers.
   b. On disconnect it's more complex. I think we first need to notify then resubscribe, since in a connected subset if one node is disconnected, we can try to reconnect through other nodes, but they also do decisions based on old information resulting to massive resubscribe request.

## Detailed design

1. Extend IPROTO_SUBSCRIBE command with a list of server UUIDs for which SUBSCRIBE should fetch changes. Store this UUIDs within applier's internal data structure. By default issuing SUBSCRIBE with empty list what means no filtering at all.
2. Implement white-list filtering in relay. After processing SUBSCRIBE request, relay has a list of UUIDs. Extract associated peer ids and fill in a filter. By default transmit all records, unless SUBSCRIBE was done with at least one server UUID. In latter case drop all records except originating from replicas in this list.
3. After issuing REQUEST_VOTE to all peers, subscription daemon knows a map of server UUIDs, their peers and their vclocks. For each reachable UUID select shortest path and assign UUIDs to direct peer through it this pass goes. Issue the subscribe requests. Notify downstream peers with new topology.
4. Rebalancing. Connect/disconnect should trigger daemon to start reassigning process.
 - On disconnect first find "orphan" and then reassigned all reachable UUIDs to direct peers through who shortest path goes. Notify downstream peers.
 - On connect, by iterating through appliers list, find UUIDs with shorter path found, reassign them to correct peers and issue SUBSCRIBE for recently connected applier and for the one from whom we get these UUIDs back.
