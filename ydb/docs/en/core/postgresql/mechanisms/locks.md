# Locks

Locks are used to order concurrent access of multiple processes to the same resources. A resource can be an object that the DBMS works with:

1. Lists of primary keys, conditions, or an entire datashard (if there are too many locks);
2. A structure in memory, such as a hash table, buffer, etc. (identified by a pre-assigned number);
3. Abstract resources that have no physical meaning (identified simply by a unique number).

For a process to access a resource, it must acquire a lock associated with that resource. A lock is a shared memory segment that stores information about whether the lock is free or acquired. A lock itself is also a resource that can be accessed concurrently. Special synchronization primitives are used to access locks: semaphores or mutexes. Their purpose is to ensure that code accessing a shared resource is executed in only one process at a time. If the resource is busy and locking is not possible, the process will terminate with an error.

Locks can be classified by their implementation principle:

- **Pessimistic locking**: before data modification, a lock is placed on all potentially affected rows. This prevents them from being modified by other sessions until the current operation is complete. After modification, it is guaranteed that the data writing will be consistent;
- **Optimistic locking**: does not restrict access to data during operation but uses a special attribute (for example: `VERSION`) to control changes. The attribute is a field from the row metadata that is not visible to users; it relates to the implementation details of the locking mechanism. Before committing changes, the set attribute is checked. If it hasn't changed – the changes will be committed (`COMMIT`), otherwise the transaction will be rolled back (`ROLLBACK`).

In {{ ydb-short-name }}'s PostgreSQL compatibility, optimistic locks are used – this means that transactions check lock conditions at the end of their operation. If during the transaction execution the lock was violated – such a transaction will end with an error:

```text
Error: Transaction locks invalidated. Table: <table name>, code: 2001
```

Transactions that execute both SQL read and write instructions may end with an error. Transactions that execute only SQL read or write instructions complete correctly. Here's an example of a transaction that will end with an error if data changes are made to the table by a parallel transaction:

```sql
BEGIN;
SELECT * FROM people;
-- If an INSERT is executed here in another transaction, this transaction will end with an error
UPDATE people SET age = 27
WHERE name = 'JOHN';
COMMIT;
```

As a result, the transaction will end with an `Error: Transaction locks invalidated` error and will be rolled back (`ROLLBACK`). In case of an `Error: Transaction locks invalidated` error – you can try to execute the transaction again.