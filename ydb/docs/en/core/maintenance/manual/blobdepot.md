# BlobDepot

BlobDepot extends the storage subsystem functionality by adding virtual group capabilities.

A virtual group, like a physical group, is a fault-tolerance unit of the storage subsystem in the cluster. However, a virtual group stores its data in other groups (unlike a physical group, which stores data on VDisks).

This method of data storage enables more flexible use of the {{ ydb-name }} storage subsystem, specifically:

* Using "heavier" tablets in conditions where the size of a single physical group is limited
* Providing tablets with a wider write bandwidth by balancing writes across all groups over which BlobDepot is running
* Ensuring transparent data migration between different groups for client tablets

BlobDepot can also be used for group decommissioning, i.e., for removing a VDisk within a physical group while preserving all data written to this group. In this usage scenario, data from the physical group is transparently transferred to a virtual group, after which all VDisks of the physical group are removed to free up their resources.

## Virtual Group Mode

In virtual group mode, BlobDepot allows combining multiple groups into a unified space for storing large volumes of data. It provides space usage balancing and increased throughput by distributing writes across different groups. Background data transfer (which is completely transparent to the client) is also possible.

Virtual groups are created within Storage Pools, just like physical groups, but it's recommended to create a separate pool for virtual groups using the `dstool pool create virtual` command. These pools can be specified in Hive for creating other tablets. However, to avoid latency degradation, it's recommended to place channels 0 and 1 of tablets on physical groups, and only place data channels on virtual groups with BlobDepot.

### How to Launch {#vg-create-params}

A virtual group is created through BS_CONTROLLER by sending a special command. The virtual group creation command is idempotent, so to prevent creating unnecessary BlobDepots, each virtual group is assigned a name. The name must be unique within the entire cluster. If the command is executed again, an error will be returned with the `Already: true` field set and indicating the number of the previously created virtual group.

```bash
dstool -e ... --direct group virtual create --name vg1 vg2 --hive-id=72057594037968897 --storage-pool-name=/Root:virtual --log-channel-sp=/Root:ssd --data-channel-sp=/Root:ssd*8
```

Command line parameters:

* --name unique name for the virtual group (or multiple virtual groups with similar parameters)
* --hive-id=N tablet number of the Hive that will manage this BlobDepot; you must specify the Hive of the tenant within which the BlobDepot is being launched
* --storage-pool-name=POOL_NAME name of the Storage Pool within which to create the BlobDepot
* --storage-pool-id=BOX:POOL alternative to `--storage-pool-name`, where you can specify an explicit numeric pool identifier
* --log-channel-sp=POOL_NAME name of the pool where channel 0 of the BlobDepot tablet will be placed
* --snapshot-channel-sp=POOL_NAME name of the pool where channel 0 of the BlobDepot tablet will be placed; if not specified, the value from --log-channel-sp is used
* --data-channel-sp=POOL_NAME[*COUNT] name of the pool where data channels are placed; if COUNT parameter is specified (after the asterisk), COUNT data channels are created in the specified pool; it's recommended to create a large number of data channels for BlobDepot in virtual group mode (64..250) to use storage most effectively
* --wait wait for BlobDepot creation to complete; if this option is not specified, the command completes immediately after responding to the BlobDepot creation request, without waiting for the tablets themselves to be created and started

### How to Check That Everything Started {#vg-check-running}

You can view the result of virtual group creation in the following ways:

* through the BS_CONTROLLER monitoring page
* through the `dstool group list --virtual-groups-only` command

In both cases, you need to check the VirtualGroupName field, which should match what was passed in the --name parameter. If the `dstool group virtual create` command completed successfully, the virtual group unconditionally appears in the group list, but the VirtualGroupState field can take one of the following values:

* NEW — group awaiting initialization (tablet creation through Hive, its configuration and startup is in progress)
* WORKING — group is created and working, ready to handle user requests
* CREATE_FAILED — an error occurred during group creation, its text description can be seen in the ErrorReason field

```
$ dstool --cluster=$CLUSTER --direct group list --virtual-groups-only
┌────────────┬──────────────┬───────────────┬────────────┬────────────────┬─────────────────┬──────────────┬───────────────────┬──────────────────┬───────────────────┬─────────────┬───────��────────┐
│ GroupId    │ BoxId:PoolId │ PoolName      │ Generation │ ErasureSpecies │ OperatingStatus │ VDisks_TOTAL │ VirtualGroupState │ VirtualGroupName │ BlobDepotId       │ ErrorReason │ DecommitStatus │
├────────────┼──────────────┼───────────────┼────────────┼────────────────┼─────────────────┼──────────────┼───────────────────┼──────────────────┼───────────────────┼─────────────┼────────────────┤
│ 4261412864 │ [1:2]        │ /Root:virtual │ 0          │ none           │ DISINTEGRATED   │ 0            │ WORKING           │ vg1              │ 72075186224037888 │             │ NONE           │
│ 4261412865 │ [1:2]        │ /Root:virtual │ 0          │ none           │ DISINTEGRATED   │ 0            │ WORKING           │ vg2              │ 72075186224037890 │             │ NONE           │
│ 4261412866 │ [1:2]        │ /Root:virtual │ 0          │ none           │ DISINTEGRATED   │ 0            │ WORKING           │ vg3              │ 72075186224037889 │             │ NONE           │
│ 4261412867 │ [1:2]        │ /Root:virtual │ 0          │ none           │ DISINTEGRATED   │ 0            │ WORKING           │ vg4              │ 72075186224037891 │             │ NONE           │
└────────────┴──────────────┴───────────────┴────────────┴────────────────┴─────────────────┴──────────────┴───────────────────┴──────────────────┴───────────────────┴─────────────┴────────────────┘
```

## Architecture

BlobDepot is a tablet that, in addition to two system channels (0 and 1), also contains a set of additional channels where the actual data written to BlobDepot is stored. Client data is written to these additional channels.

BlobDepot as a tablet can be run on any cluster node.

When BlobDepot operates in virtual group mode, agents (BlobDepotAgent) are used to access it. These are actors that perform functions similar to DS proxy — they are launched on each node that uses a virtual group with BlobDepot. These same actors transform storage requests into commands for BlobDepot and ensure data exchange with it.

## Diagnostic Mechanisms

The following mechanisms are provided for diagnosing BlobDepot functionality:

* [BS_CONTROLLER monitoring page](#diag-bscontroller)
* [BlobDepot monitoring page](#diag-blobdepot)
* [internal viewer](#diag-viewer)
* [event log](#diag-log)
* [graphs](#diag-sensors)

### BS_CONTROLLER Monitoring Page {#diag-bscontroller}

The BS_CONTROLLER monitoring page has a special Virtual groups tab that shows all groups using BlobDepot:

![Virtual groups](_assets/virtual-groups.png "Virtual groups")

The table has the following columns:

Field | Description
---- | --------
GroupId | Group number
StoragePoolName | Name of the pool where the group is located
Name | Virtual group name; it's unique across the entire cluster. For decommissioned groups, this will be null
BlobDepotId | Number of the BlobDepot tablet responsible for serving this group
State | [BlobDepot state](#vg-check-running); can be NEW, WORKING, CREATED_FAILED
HiveId | Number of the Hive tablet within which the specified BlobDepot was created
ErrorReason | When state is CREATE_FAILED, contains text description of the creation error reason
DecommitStatus | [Group decommission state](#decommit-check-running); can be NONE, PENDING, IN_PROGRESS, DONE

### BlobDepot Monitoring Page {#diag-blobdepot}

The BlobDepot monitoring page shows the main tablet operation parameters, grouped into tabs available via the "Contained data" link:

* [data](#mon-data)
* [refcount](#mon-refcount)
* [trash](#mon-trash)
* [barriers](#mon-barriers)
* [blocks](#mon-blocks)
* [storage](#mon-storage)

Additionally, the main page shows brief information about BlobDepot state:

![BlobDepot stats](_assets/blobdepot-stats.png "BlobDepot stats")

This table shows the following data:

* Data, bytes — amount of stored data bytes ([TotalStoredDataSize](#diag-sensors))
* Trash in flight, bytes — amount of unnecessary data bytes waiting for transaction completion to become garbage ([InFlightTrashSize](#diag-sensors))
* Trash pending, bytes — amount of garbage bytes not yet passed to garbage collection ([TotalStoredTrashSize](#diag-sensors))
* Data in GroupId# XXX, bytes — amount of data bytes in group XXX (both useful data and not yet collected garbage)

![BlobDepot main](_assets/blobdepot-main.png "BlobDepot main")

Parameter purposes are as follows:

* Loaded — boolean value showing whether all metadata from the tablet's local database is loaded into memory
* Last assimilated blob id — BlobId of the last read blob (metadata copying during decommissioning)
* Data size, number of keys — number of stored data keys
* RefCount size, number of blobs — number of unique data blobs that BlobDepot stores in its namespace
* Total stored data size, bytes — same as "Data, bytes" from the table above
* Keys made certain, number of keys — number of unwritten keys that were then confirmed by reading

The "Uncertainty resolver" section relates to the component that works with data written to but not confirmed in BlobDepot.

#### data {#mon-data}

![data tab](_assets/blobdepot-data.png "data tab")

The data table contains the following columns:

* key — key identifier (BlobId in client namespace)
* value chain — key value formed by concatenating blob fragments from BlobDepot namespace (this field lists these blobs)
* keep state — value of keep flags for this blob from client perspective (Default, Keep, DoNotKeep)
* barrier — field showing under which barrier this blob falls (S — under soft barrier, H — under hard barrier; in practice, H never occurs since blobs are synchronously deleted from the table when a hard barrier is set)

Given the potentially large size of the table, only a part of it is shown on the monitoring page. To find a needed blob, you can fill in the "seek" field by entering the BlobId of the blob you're looking for, then specify the number of interesting rows before and after this blob and click the "Show" button.

#### refcount {#mon-refcount}

![refcount tab](_assets/blobdepot-refcount.png "refcount tab")

The refcount table contains two columns: "blob id" and "refcount". Blob id is the identifier of the stored blob written on behalf of BlobDepot in storage. Refcount is the number of references to this blob from the data table (from the value chain column).

The TotalStoredDataSize metric is formed from the sum of sizes of all blobs in this table, each counted exactly once, regardless of the refcount field.

#### trash {#mon-trash}

![trash tab](_assets/blobdepot-trash.png "trash tab")

The table contains three columns: "group id", "blob id", and "in flight". Group id is the number of the group where the no longer needed blob is stored. Blob id is the identifier of the blob itself. In flight is a flag indicating that the blob is still going through a transaction, only after which it can be passed to the garbage collector.

The TotalStoredTrashSize and InFlightTrashSize metrics are formed from this table by summing the sizes of blobs without and with the in flight flag, respectively.

#### barriers {#mon-barriers}

![barriers tab](_assets/blobdepot-barriers.png "barriers tab")

The barriers table contains information about client barriers that were passed to BlobDepot. It consists of columns "tablet id" (tablet number), "channel" (channel number for which the barrier is recorded), and barrier values: "soft" and "hard". The value has the format gen:counter => collect_gen:collect_step, where gen is the tablet generation number in which this barrier was set, counter is the sequential number of the garbage collection command, collect_gen:collect_step is the barrier value (all blobs with generation and step within generation less than or equal to the specified barrier are deleted).

#### blocks {#mon-blocks}

![blocks tab](_assets/blobdepot-blocks.png "blocks tab")

The blocks table contains a list of client tablet locks and consists of columns "tablet id" (tablet number) and "blocked generation" (number of this tablet's generation in which nothing can be written anymore).

#### storage {#mon-storage}

![storage tab](_assets/blobdepot-storage.png "storage tab")

The storage table shows statistics for stored data for each group where BlobDepot stores data. This table contains the following columns:

* group id — number of the group where data is stored
* bytes stored in current generation — volume of data written to this group in the current tablet generation (only useful data is counted, without garbage)
* bytes stored total — volume of all data saved by this BlobDepot in the specified group
* status flag — color flags for group state
* free space share — group fill indicator (value 0 corresponds to a group completely filled by space, 1 — completely free)

### Internal viewer {#diag-viewer}

On the Internal viewer monitoring page shown below, BlobDepots can be seen in the Storage sections and as BD tablets.

In the Nodes section, you can see BD tablets running on different system nodes:

![Nodes](_assets/viewer-nodes.png "Nodes")

In the Storage section, you can see virtual groups working through BlobDepot. They can be distinguished by the BlobDepot link in the Erasure column. The link in this column leads to the tablet monitoring page. Otherwise, virtual groups are displayed the same way, except they have no PDisk and VDisk. However, decommissioned groups will look the same as virtual ones but have PDisk and VDisk until decommissioning is complete.

![Storage](_assets/viewer-storage.png "Storage")

### Event Log {#diag-log}

The BlobDepot tablet writes events to the log with the following component names:

* BLOB_DEPOT — BlobDepot tablet component
* BLOB_DEPOT_AGENT — BlobDepot agent component
* BLOB_DEPOT_TRACE — special component for debug tracing of all data-related events

BLOB_DEPOT and BLOB_DEPOT_AGENT are output as structured records containing fields that allow identifying the BlobDepot and the group it serves. For BLOB_DEPOT, this is the Id field with format {TabletId:GroupId}:Generation, where TabletId is the BlobDepot tablet number, GroupId is the number of the group it serves, Generation is the generation in which the running BlobDepot writes messages to the log. For BLOB_DEPOT_AGENT, this field is called AgentId and has the format {TabletId:GroupId}.

At DEBUG level, most occurring events will be written to the log, both on the tablet side and the agent side. This mode is used for debugging and is not recommended in production environments due to the large number of generated events.

### Graphs {#diag-sensors}

Each BlobDepot tablet provides the following graphs:

Graph | Type | Description
-------------------- | ------------ | --------
TotalStoredDataSize | simple | Amount of stored user data net (if there are multiple references to one blob, it is counted once)
TotalStoredTrashSize | simple | Number of bytes in garbage data that is no longer needed but not yet passed to garbage collection
InFlightTrashSize | simple | Number of garbage bytes still waiting for write confirmation to local database (they can't even begin to be collected yet)
BytesToDecommit | simple | Number of data bytes left to [decommission](#decommit-progress) (if this BlobDepot is working in group decommission mode)
Puts/Incoming | cumulative | Rate of incoming write requests (in pieces per unit time)
Puts/Ok | cumulative | Number of successfully completed write requests
Puts/Error | cumulative | Number of write requests completed with error
Decommit/GetBytes | cumulative | Data read rate during [decommissioning](#decommit-progress)
Decommit/PutOkBytes | cumulative | Data write rate during [decommissioning](#decommit-progress) (only successfully completed writes are counted)