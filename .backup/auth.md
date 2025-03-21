# Kafka API Authentication

In Kafka API, authentication is performed through `SASL_PLAINTEXT/PLAIN` or `SASL_SSL/PLAIN` mechanisms.

Authentication requires:

* `<user-name>` - username. For user management details, see [Access Management](../../security/access-management.md).
* `<password>` - user password. For user management details, see [Access Management](../../security/access-management.md).
* `<database>` - [database path](../../concepts/connect#database).

These parameters are used to form:

* `<sasl.username>` = `<user-name>@<database>`
* `<sasl.password>` = `<password>`

{% note warning %}

Please note that the logic for forming `<sasl.username>` and `<sasl.password>` in cloud installations of {{ ydb-short-name }} may differ from what is shown here.

{% endnote %}

For authentication examples, see [Reading and Writing](./read-write.md).