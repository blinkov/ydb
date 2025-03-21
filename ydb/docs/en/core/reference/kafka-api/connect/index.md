# Using Kafka Connect

This section provides configuration examples for popular Kafka Connect connectors to work with {{ ydb-short-name }}.

The [Kafka Connect](https://kafka.apache.org/documentation/#connect) tool is designed to move data between Apache Kafka® and other data stores. {{ ydb-short-name }} supports working with topics via the Kafka protocol, so you can use Kafka Connect connectors to work with {{ ydb-short-name }}.

For detailed information about Kafka Connect and its configuration, see the [Apache Kafka®](https://kafka.apache.org/documentation/#connect) documentation.


{% note warning %}

Kafka Connect instances for working with {{ ydb-short-name }} should only be deployed in standalone mode (single worker process). {{ ydb-short-name }} does not support Kafka Connect in distributed mode.

{% endnote %}