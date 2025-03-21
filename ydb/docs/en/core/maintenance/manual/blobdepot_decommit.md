# Group Decommissioning

Physical groups are a valuable resource in the cluster: groups can be created, but they cannot be deleted without deleting the database using them, as there is no mechanism for guaranteed eviction of tablet data from a group. The number of physical groups is determined by cluster size, and groups cannot be moved from one tenant's pool to another's due to the use of different encryption keys for different tenants.

This can lead to a situation where there aren't enough resources to create a new group for expanding an existing database or creating a new one, and the old group can't be deleted to free up resources because it may contain data.

To solve this problem, you can create a virtual group with channels over the remaining pool groups, copy data from the physical group into it, and then free up the resources occupied by the physical group. This task is solved by the group decommissioning process.

Group decommissioning allows removing redundant VDisks from PDisk while preserving the group's data. This mode is implemented through creating a BlobDepot that begins serving the decommissioned group instead of DS proxy. In parallel, BlobDepot copies data from the physical decommissioned group. Once all data is copied, the physical VDisks are removed and resources are freed, while all data from the decommissioned group is distributed across other groups.

The decommissioning process is completely transparent to tablets and users and consists of several stages:

1. Creating a BlobDepot tablet and distributing group configuration to block writing to physical group disks.
2. Copying lock metadata from the physical group. After this point, the decommissioned group becomes available for operation. Until the locks are copied, working with the group is impossible. However, this process takes a very short time, so it's practically unnoticeable to the client. Requests arriving at this time queue up and wait for the stage to complete.
3. Copying barrier metadata from the physical group.
4. Copying blob metadata from the physical group.
5. Copying blob data from the physical group.
6. Removing VDisks of the physical group.

It's worth noting again that from the moment writing to the physical group is blocked until all locks are read, work with the group is suspended. The suspension time under normal operation is fractions of a second.

## How to Launch

To start decommissioning, a BS_CONTROLLER command is executed where you need to specify the list of groups to decommission and the Hive tablet number that will manage the BlobDepots of the decommissioned groups. You can also specify a list of pools where BlobDepot will store its data. If this list is not specified, BS_CONTROLLER automatically selects the same pools where the decommissioned groups are located for data storage, and the number of data channels is made equal to the number of physical groups in these pools (but not more than 250).

```bash
dstool -e ... --direct group decommit --group-ids 2181038080 --database=/Root/db1 --hive-id=72057594037968897
```

Command line parameters:

* --group-ids GROUP_ID GROUP_ID list of groups that can be decommissioned
* --database=DB specify the tenant where decommissioning needs to be done
* --hive-id=N explicitly specify the Hive tablet number that will manage this BlobDepot; you cannot specify the Hive identifier of the tenant that owns the pools with decommissioned groups, as this Hive may store its data over a group managed by BlobDepot, which would lead to a circular dependency; it's recommended to specify the root Hive
* --log-channel-sp=POOL_NAME name of the pool where channel 0 of the BlobDepot tablet will be placed
* --snapshot-channel-sp=POOL_NAME name of the pool where channel 1 of the BlobDepot tablet will be placed; if not specified, the value from --log-channel-sp is used
* --data-channel-sp=POOL_NAME[*COUNT] name of the pool where data channels are placed; if COUNT parameter is specified (after the asterisk), COUNT data channels are created in the specified pool

If neither --log-channel-sp, nor --snapshot-channel-sp, nor --data-channel-sp are specified, the storage pool to which the decommissioned group belongs is automatically found, and channel zero and first of BlobDepot are created in it, as well as N data channels, where N is the number of remaining physical groups in this pool.

## How to Check That Everything Started

You can view the decommissioning result similarly to creating virtual groups. For decommissioned groups, an additional DecommitStatus field appears, which can take one of the following values:

* NONE — decommissioning is not being performed for the specified group
* PENDING — group decommissioning is expected but not yet being performed (BlobDepot is being created)
* IN_PROGRESS — group decommissioning is in progress (all writes already go to BlobDepot, reads go to both BlobDepot and the old group)
* DONE — decommissioning is completely finished

```bash
$ dstool --cluster=$CLUSTER --direct group list --virtual-groups-only
┌────────────┬──────────────┬───────────────┬────────────┬────────────────┬─────────────────┬──────────────┬───────────────────┬──────────────────┬───────────────────┬─────────────┬────────────────┐
│ GroupId    │ BoxId:PoolId │ PoolName      │ Generation │ ErasureSpecies │ OperatingStatus │ VDisks_TOTAL │ VirtualGroupState │ VirtualGroupName │ BlobDepotId       │ ErrorReason │ DecommitStatus │
├────────────┼──────────────┼───────────────┼────────────┼────────────────┼─────────────────┼──────────────┼───────────────────┼──────────────────┼───────────────────┼─────────────┼────────────────┤
│ 2181038080 │ [1:1]        │ /Root:ssd     │ 2          │ block-4-2      │ FULL            │ 8            │ WORKING           │                  │ 72075186224038160 │             │ IN_PROGRESS    │
│ 2181038081 │ [1:1]        │ /Root:ssd     │ 2          │ block-4-2      │ FULL            │ 8            │ WORKING           │                  │ 72075186224038161 │             │ IN_PROGRESS    │
│ 4261412864 │ [1:2]        │ /Root:virtual │ 0          │ none           │ DISINTEGRATED   │ 0            │ WORKING           │ vg1              │ 72075186224037888 │             │ NONE           │
│ 4261412865 │ [1:2]        │ /Root:virtual │ 0          │ none           │ DISINTEGRATED   │ 0            │ WORKING           │ vg2              │ 72075186224037890 │             │ NONE           │
│ 4261412866 │ [1:2]        │ /Root:virtual │ 0          │ none           │ DISINTEGRATED   │ 0            │ WORKING           │ vg3              │ 72075186224037889 │             │ NONE           │
│ 4261412867 │ [1:2]        │ /Root:virtual │ 0          │ none           │ DISINTEGRATED   │ 0            │ WORKING           │ vg4              │ 72075186224037891 │             │ NONE           │
└────────────┴──────────────┴───────────────┴────────────┴────────────────┴─────────────────┴──────────────┴───────────────────┴──────────────────┴───────────────────┴─────────────┴────────────────┘
```

## How to Assess Progress {#decommit-progress}

For evaluating decommissioning time and progress, graphs are provided that show:

* whether decommissioning is in progress (Decommit/GetBytes)
* whether data writing is occurring (Decommit/PutOkBytes)
* how much data is left to decommission (BytesToDecommit)

If everything is executing successfully, then the Decommit/GetBytes rate roughly corresponds to Decommit/PutOkBytes. Minor discrepancies are acceptable due to the fact that decommissioned data may become outdated and be deleted by the tablet storing data in it.

To estimate the remaining decommissioning time, simply divide BytesToDecommit by the average Decommit/PutOkBytes rate.