# {{ airflow-name }}

Integration of {{ ydb-short-name }} with [{{ airflow-name }}](https://airflow.apache.org) enables automation and management of complex workflows. {{ airflow-name }} provides capabilities for task scheduling, execution monitoring, and dependency management — orchestration. Using Airflow for orchestrating tasks such as data loading into {{ydb-short-name}}, query execution, and transaction management allows you to automate and optimize operational processes. This is particularly important for ETL tasks where large volumes of data require regular extraction, transformation, and loading.

{{ ydb-full-name }} provides an [Apache Airflow provider](https://airflow.apache.org/docs/apache-airflow-providers) package [apache-airflow-providers-ydb](https://pypi.org/project/apache-airflow-providers-ydb/) for working under {{ airflow-name }}. [{{ airflow-name }} tasks](https://airflow.apache.org/docs/apache-airflow/stable/index.html) are Python applications consisting of a set of [{{ airflow-name }} operators](https://airflow.apache.org/docs/apache-airflow/stable/core-concepts/operators.html) and their [dependencies](https://airflow.apache.org/docs/apache-airflow/stable/core-concepts/dags.html) that define the execution order.

## Setup {#setup}

For the `apache-airflow-providers-ydb` package to work correctly, execute the following commands on all {{ airflow-name }} hosts:

```shell
pip install ydb apache-airflow-providers-ydb
```

Python version 3.8 or higher is required.

## Object Model {#object_model}

The `airflow.providers.ydb` package contains a set of components for interacting with {{ ydb-full-name }}:

- [YDBExecuteQueryOperator](#ydb_execute_query_operator) operator for integrating tasks into the {{ airflow-name }} scheduler.
- [YDBHook](#ydb_hook) for direct interaction with {{ ydb-name }}.

### YDBExecuteQueryOperator {#ydb_execute_query_operator}

The `YDBExecuteQueryOperator` {{ airflow-name }} operator is used to execute queries in {{ ydb-full-name }}.

Required arguments:

* `task_id` — {{ airflow-name }} task name.
* `sql` — SQL query text to be executed in {{ ydb-full-name }}.

Optional arguments:

* `ydb_conn_id` — connection identifier with type `YDB` containing connection parameters for {{ ydb-full-name }}. If not specified, the connection named [`ydb_default`](#ydb_default) is used. The `ydb_default` connection is pre-installed with {{ airflow-name }}, you don't need to create it separately.
* `is_ddl` — flag indicating that a [SQL DDL](https://en.wikipedia.org/wiki/Data_definition_language) query is being executed. If the argument is not specified or set to `False`, a [SQL DML](https://en.wikipedia.org/wiki/Data_Manipulation_Language) query will be executed.
* `params` — dictionary of [query parameters](https://airflow.apache.org/docs/apache-airflow/stable/core-concepts/params.html).

Example:

```python
ydb_operator = YDBExecuteQueryOperator(task_id="ydb_operator", sql="SELECT 'Hello, world!'")
```

In this example, an {{ airflow-name }} task with the identifier `ydb_operator` is created, which executes the query `SELECT 'Hello, world!'`.

### YDBHook {#ydb_hook}

The {{ airflow-name }} `YDBHook` class is used to execute low-level commands in {{ ydb-full-name }}.

Optional arguments:

* `ydb_conn_id` — connection identifier with type `YDB` containing connection parameters for {{ ydb-full-name }}. If not specified, the connection named [`ydb_default`](#ydb_default) is used. The `ydb_default` connection is pre-installed with {{ airflow-name }}, you don't need to create it separately.
* `is_ddl` — flag indicating that a [SQL DDL](https://en.wikipedia.org/wiki/Data_definition_language) query is being executed. If the argument is not specified or set to `False`, a [SQL DML](https://en.wikipedia.org/wiki/Data_Manipulation_Language) query will be executed.

`YDBHook` supports the following methods:

- [bulk_upsert](#bulk_upsert);
- [get_conn](#get_conn).

#### bulk_upsert {#bulk_upsert}

Performs [bulk data insertion](../../recipes/ydb-sdk/bulk-upsert.md) into {{ ydb-full-name }} tables.

Required arguments:

* `table_name` — name of the {{ ydb-full-name }} table where data will be inserted.
* `rows` — array of rows to insert.
* `column_types` — column type descriptions.

Example:

```python
hook = YDBHook(ydb_conn_id=...)
column_types = (
        ydb.BulkUpsertColumns()
        .add_column("pet_id", ydb.OptionalType(ydb.PrimitiveType.Int32))
        .add_column("name", ydb.PrimitiveType.Utf8)
        .add_column("pet_type", ydb.PrimitiveType.Utf8)
        .add_column("birth_date", ydb.PrimitiveType.Utf8)
        .add_column("owner", ydb.PrimitiveType.Utf8)
    )

rows = [
    {"pet_id": 3, "name": "Lester", "pet_type": "Hamster", "birth_date": "2020-06-23", "owner": "Lily"},
    {"pet_id": 4, "name": "Quincy", "pet_type": "Parrot", "birth_date": "2013-08-11", "owner": "Anne"},
]
hook.bulk_upsert("pet", rows=rows, column_types=column_types)
```

In this example, a `YDBHook` object is created through which the `bulk_upsert` bulk data insertion operation is performed.

#### get_conn {#get_conn}

Returns a `YDBConnection` object implementing the [`DbApiConnection`](https://peps.python.org/pep-0249/#connection-objects) interface for working with data. The `DbApiConnection` class provides a standardized interface for database interaction, allowing operations such as connecting, executing SQL queries, and managing transactions, regardless of the specific database management system.

Example:

```python
hook = YDBHook(ydb_conn_id=...)

# Execute SQL query and get cursor
connection = hook.get_conn()
cursor = connection.cursor()
cursor.execute("SELECT * from pet;")

# Extract result and column names
result = cursor.fetchall()
columns = [desc[0] for desc in cursor.description]

# Close cursor and connection
cursor.close()
connection.close()
```

In this example, a `YDBHook` object is created, a `YDBConnection` object is requested from the created object, through which data is read and the column list is obtained.

## Connecting to {{ ydb-full-name }} {#ydb_default}

To connect to {{ ydb-full-name }}, you need to create a new or edit an existing [{{ airflow-name }} connection](https://airflow.apache.org/docs/apache-airflow/stable/howto/connection.html) with type `YDB`.

![](_assets/ydb_connection.png)

Where:

- `Connection Id` - {{ airflow-name }} connection name.
- `Host` - protocol and address of the {{ ydb-full-name }} cluster.
- `Port` - port for connecting to the {{ ydb-full-name }} cluster.
- `Database name` - name of the {{ ydb-full-name }} database.

Specify credentials for one of the following authentication methods on the {{ ydb-full-name }} cluster:

- `Login` and `Password` - specify user credentials for [username and password](../../concepts/auth.md#static-credentials) authentication.
- `Service account auth JSON` - specify the [`Service Account Key`](../../concepts/auth.md#iam) value.
- `Service account auth JSON file path` - specify the path to the file containing the `Service Account Key`.
- `IAM token` - specify the [IAM token](../../concepts/auth.md#iam).
- `Use VM metadata` - specify to use [virtual machine metadata](../../concepts/auth.md#iam).

## YQL to Python Type Mapping

Below are the rules for converting YQL types to Python results. Types not listed below are not supported.

### Scalar Types {#scalars-types}

| YQL Type | Python Type | Python Example |
| --- | --- | --- |
| `Int8`, `Int16`, `Int32`, `Uint8`, `Uint16`, `Uint32`, `Int64`, `Uint64` | `int` | `647713` |
| `Bool` | `bool` | True |
| `Float`, `float` | `float`<br/>NaN and Inf are represented as `None` | `7.88731023`<br/>`None` |
| `Decimal` | `Decimal` | `45.23410083` |
| `Utf8` | `str` | `String text` |
| `String` | `str` | `String text` |

### Complex Types {#complex-types}

| YQL Type | Python Type | Python Example |
| --- | --- | --- |
| `Json`, `JsonDocument` | `str` (entire node is inserted as a string) | `{"a":[1,2,3]}` |
| `Date` | `datetime.date` | `2022-02-09` |
| `Datetime`, `Timestamp` | `datetime.datetime` | `2022-02-09 10:13:11` |

### Optional Types {#optional-types}

| YQL Type | Python Type | Python Example |
| --- | --- | --- |
| `Optional` | Original type or None | `1` |

### Containers {#containers}

| YQL Type | Python Type | Python Example |
| --- | --- | --- |
| `List<Type>` | `list` | `[1,2,3,4]` |
| `Dict<KeyType, ValueType>` | `dict` | `{key1: "value1", key2: "value2"}` |
| `Set<KeyType>` | `set` | `set(key_value1, key_value2)` |
| `Tuple<Type1, Type2>` | `tuple` | `(element1, element2)` |
| `Struct<Name:Utf8,Age:Int32>`| `dict` | `{ "Name": "value1", "Age": value2 }` |

### Special Types {#special-types}

| YQL Type | Python Type |
| --- | --- |
| `Void`, `Null` | `None` |
| `EmptyList` | `[]` |
| `EmptyDict` | `{}` |

## Example {#example}

The package includes the {{ airflow-name }} [`YDBExecuteQueryOperator`](https://airflow.apache.org/docs/apache-airflow-providers-ydb/stable/_api/airflow/providers/ydb/operators/ydb/index.html) operator and [`YDBHook`](https://airflow.apache.org/docs/apache-airflow-providers-ydb/stable/_api/airflow/providers/ydb/hooks/ydb/index.html) hook for executing queries in {{ ydb-full-name }}.

In the example below, a task `create_pet_table` is created that creates a table in {{ ydb-full-name }}. After successful table creation, the task `populate_pet_table` is called, which populates the table with data using `UPSERT` commands, and the task `populate_pet_table_via_bulk_upsert`, which populates the table using [`bulk_upsert`](../../recipes/ydb-sdk/bulk-upsert.md) bulk data insertion commands. After data insertion is complete, a read operation is performed using the `get_all_pets` task and a parameterized data reading task `get_birth_date`.

![](_assets/airflow_dag.png)

To execute queries to the {{ ydb-short-name }} database, a pre-created connection to {{ ydb-short-name }} of type [YDB Connection](https://airflow.apache.org/docs/apache-airflow-providers-ydb/stable/connections/ydb.html) with the name `test_ydb_connection` is used.

```python
from __future__ import annotations

import datetime

import ydb
from airflow import DAG
from airflow.decorators import task
from airflow.providers.ydb.hooks.ydb import YDBHook
from airflow.providers.ydb.operators.ydb import YDBExecuteQueryOperator

@task
def populate_pet_table_via_bulk_upsert():
    hook = YDBHook(ydb_conn_id="test_ydb_connection")
    column_types = (
        ydb.BulkUpsertColumns()
        .add_column("pet_id", ydb.OptionalType(ydb.PrimitiveType.Int32))
        .add_column("name", ydb.PrimitiveType.Utf8)
        .add_column("pet_type", ydb.PrimitiveType.Utf8)
        .add_column("birth_date", ydb.PrimitiveType.Utf8)
        .add_column("owner", ydb.PrimitiveType.Utf8)
    )

    rows = [
        {"pet_id": 3, "name": "Lester", "pet_type": "Hamster", "birth_date": "2020-06-23", "owner": "Lily"},
        {"pet_id": 4, "name": "Quincy", "pet_type": "Parrot", "birth_date": "2013-08-11", "owner": "Anne"},
    ]
    hook.bulk_upsert("pet", rows=rows, column_types=column_types)


with DAG(
    dag_id="ydb_demo_dag",
    start_date=datetime.datetime(2020, 2, 2),
    schedule="@once",
    catchup=False,
) as dag:
    create_pet_table = YDBExecuteQueryOperator(
        task_id="create_pet_table",
        sql="""
            CREATE TABLE pet (
            pet_id INT,
            name TEXT NOT NULL,
            pet_type TEXT NOT NULL,
            birth_date TEXT NOT NULL,
            owner TEXT NOT NULL,
            PRIMARY KEY (pet_id)
            );
          """,
        is_ddl=True,  # must be specified for DDL queries
        ydb_conn_id="test_ydb_connection"
    )

    populate_pet_table = YDBExecuteQueryOperator(