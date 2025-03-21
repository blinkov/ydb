# PostgreSQL and {{ ydb-name }} Interoperability

{% include [./_includes/alert.md](./_includes/alert_preview.md) %}

Support for executing queries in {{ ydb-name }} using PostgreSQL syntax is implemented through a compatibility layer:

1. The program sends queries to {{ ydb-short-name }}, where they are processed by a component called `pgwire`. Pgwire implements the PostgreSQL [network protocol](https://www.postgresql.org/docs/16/protocol.html) and passes commands to the query processor.
2. The query processor translates PostgreSQL queries into YQL AST.
3. After processing the queries, results are collected and sent back to the program that sent the query using the PostgreSQL network protocol. During query processing, it can be parallelized and executed on any number of {{ ydb-name }} nodes.

Graphically, PostgreSQL compatibility operation can be represented as follows:
![PostgreSQL compatibility operation scheme](./_includes/ydb_pg_scheme.png)

This PostgreSQL integration architecture allows executing PostgreSQL queries over {{ydb-name}} data types and vice versa, executing YQL queries over PostgreSQL data types, ensuring data interoperability.

Let's illustrate this with the following scenario:

1. Create a {{ ydb-name }} table using YQL syntax

    ```sql
    CREATE TABLE `test_table`(col INT, PRIMARY KEY(col));
    ```

2. Add test data

    ```sql
    INSERT INTO test_table(col) VALUES(1)
    ```

3. Read this data using PostgreSQL syntax

    ```bash
    psql -h <ydb_address> -d <database_name> -U <user_name> -c "SELECT * FROM test_table"
    col
    ---
    1
    (1 row)
    ```

    Where:

    - `<ydb_address>` - address of the {{ ydb-short-name }} cluster to connect to.
    - `<database_name>` - database name in the cluster. Can be a complex name, for example, `mycluster/tenant1/database`.
    - `<user_name>` - user login.

## Data Type Correspondence {#supported_types}

The data type systems of {{ydb-name}} and PostgreSQL are similar but not identical.

### Using Data from Tables Created in YQL Syntax in PostgreSQL Syntax {#topg}

Data types from {{ydb-name}} are automatically converted to their corresponding PostgreSQL types. The conversion is performed implicitly using the [`ToPg`](../yql/reference/udf/list/postgres.md#topg) command.

Example:

1. Create a {{ydb-name}} table using YQL syntax

    ```sql
    CREATE TABLE `test_table`(col INT, PRIMARY KEY(col));
    ```

2. Add test data

    ```sql
    INSERT INTO test_table(col) VALUES(1)
    ```

3. Read this data using PostgreSQL syntax. The YQL data type `INT` was automatically converted to PostgreSQL type `int4`, on which the increment operation was performed.

    ```bash
    psql -c "SELECT col+1 AS col FROM test_table"
    col
    ---
    2
    (1 row)
    ```

Since all calculations are performed inside {{ ydb-short-name }}, a "mirror type" is created in {{ ydb-short-name }} for each PostgreSQL type. For example, the `text` type from PostgreSQL will have the type `pgtext` when processed inside {{ ydb-short-name }}. This is done to ensure exact semantics of PostgreSQL types inside {{ ydb-short-name }}. When converting a type from PostgreSQL to {{ ydb-short-name }}, the rule is that for each such type, the prefix `pg` is added, followed by the original PostgreSQL type name.

Table of YQL data type correspondence when used in PostgreSQL queries:

{% include [topg](../yql/reference/udf/list/_includes/topg.md) %}

### Using Data from Tables Created in PostgreSQL Syntax in YQL Syntax {#frompg}

When using data from tables created in PostgreSQL syntax, YQL interprets the data in these columns as special types from the `pg*` family.

Example:

1. Create a {{ydb-name}} table using PostgreSQL syntax

    ```sql
    CREATE TABLE test_table_pg(numeric INT, PRIMARY KEY(col));
    ```

2. Add test data

    ```sql
    INSERT INTO test_table_pg(col) VALUES(10)
    ```

3. Read this data using YQL syntax

    ```bash
    ydb sql -s "SELECT col+1 AS col FROM test_table_pg"
    col
    ---
    "11" -- pgnumeric
    (1 row)
    ```

The rules for converting PostgreSQL types to YQL types are shown in the table:

|PostgreSQL | YQL|
|---|---|
| `bool` |`pgbool` |
| `int2` |`pgint2` |
| `int4` |`pgint4` |
| `int8` |`pgint8` |
|`numeric` |`pgnumeric` |
| `float4` |`pgfloat4` |
| `float8` |`pgfloat8` |
| `bytea` |`pgbytea` |
| `text` |`pgtext` |
| `bytea` |`pgbytea` |
| `json` |`pgjson` |
| `uuid` |`pguuid` |
| `jsonb` |`pgjsonb` |
| `date` |`pgdate` |
| `timestamp` |`pgtimestamp` |
| `interval` | `pginterval` |
| `text` |`pgtext` |
| `date` | `pgdate`|
| `timestamp` |`pgtimestamp` |
| `interval` |`pginterval` |
| `numeric` |`pgnumeric` |

Built-in YQL functions are oriented towards working with their own data types, for example, `Ip::FromString` takes `Utf8` or `String` data types as input. Therefore, built-in YQL functions cannot work with PostgreSQL data types. To solve the type conversion task, there is the [`FromPg`](../yql/reference/udf/list/postgres.md#frompg) function that performs data conversion from PostgreSQL types to YQL types.

Example:

1. Create a {{ydb-name}} table using PostgreSQL syntax

    ```sql
    CREATE TABLE test_table_pg_ip(col text, PRIMARY KEY(col));
    ```

2. Add test data

    ```sql
    INSERT INTO test_table_pg_ip(col) VALUES('::ffff:77.75.155.3')
    ```

3. Read this data using YQL syntax:

    ```bash
    ydb sql -s "SELECT Ip::ToString(Ip::GetSubnet(Ip::FromString(col))) AS subnet
        FROM test_table_pg_ip"
    Status: GENERIC_ERROR
    Issues:
    <main>: Error: Type annotation, code: 1030
        <main>:1:1: Error: At function: RemovePrefixMembers, At function: PersistableRepr, At function: SqlProject, At function: SqlProjectItem
            <main>:1:12: Error: At function: Apply, Callable is produced by Udf: Ip.FromString
                <main>:1:23: Error: Mismatch type argument #1, type diff: String!=pgtext
    ```

4. For the `Ip::FromString` function to work, you need to first perform data type conversion using the `FromPg` function:

    ```bash
    ydb sql -s "SELECT Ip::ToString(Ip::GetSubnet(Ip::FromString(FromPg(col))) AS subnet
        FROM test_table_pg_ip"
    ┌────────┐
    │ subnet │
    ├────────┤
    │ "::"   │
    └────────┘
    ```

Additionally, when working with PostgreSQL data types, you can [use PostgreSQL functions themselves](../yql/reference/udf/list/postgres.md#callpgfunction) directly from YQL syntax:

1. Create a {{ydb-name}} table using PostgreSQL syntax

    ```sql
    CREATE TABLE test_table_array(col INT, col2 _INT, PRIMARY KEY(col));
    ```

2. Add test data

    ```sql
    INSERT INTO test_table_array(col, col2) VALUES(1, '{1,2,3}')
    ```

3. Call the built-in PostgreSQL function `array_length`:

    ```bash
    ydb sql -s "select Pg::array_length(col2, 1) FROM test_table_array"
    col
    ---
    "3" -- number of elements in the array
    (1 row)
    ```