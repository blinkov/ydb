# Setting Up Kafka Connect: Step-by-Step Guide

This section provides step-by-step instructions for setting up a Kafka Connect connector to copy data from a {{ ydb-short-name }} topic to a file.

The instructions use:

* `<topic-name>` - topic name. You can specify either the full name (including the database path) or just the topic name.
* `<sasl.username>` - SASL username. For details, see [Authentication](../auth.md).
* `<sasl.password>` - SASL user password. For details, see [Authentication](../auth.md).

1. [Create a consumer](../../ydb-cli/topic-consumer-add.md) with the name `connect-<connector-name>`. The connector name is specified in the configuration file in the `name` field during setup.

2. [Download](https://downloads.apache.org/kafka/) and extract the Apache Kafka® archive:

    ```bash
    wget https://downloads.apache.org/kafka/3.6.1/kafka_2.13-3.6.1.tgz && tar -xvf kafka_2.13-3.6.1.tgz --strip 1 --directory /opt/kafka/
    ```

    This example uses Apache Kafka® version `3.6.1`.

3. Create a directory for worker process settings:

    ```bash
    sudo mkdir --parents /etc/kafka-connect-worker
    ```

4. Create the worker process settings file `/etc/kafka-connect-worker/worker.properties`

    ```ini
    # Main properties
    bootstrap.servers=<ydb-endpoint>

    # AdminAPI properties
    sasl.mechanism=PLAIN
    security.protocol=SASL_SSL
    sasl.jaas.config=org.apache.kafka.common.security.plain.PlainLoginModule required username="<sasl.username>" password="<sasl.password>";

    # Producer properties
    producer.sasl.mechanism=PLAIN
    producer.security.protocol=SASL_SSL
    producer.sasl.jaas.config=org.apache.kafka.common.security.plain.PlainLoginModule required username="<sasl.username>" password="<sasl.password>";

    # Consumer properties
    consumer.sasl.mechanism=PLAIN
    consumer.security.protocol=SASL_SSL
    consumer.sasl.jaas.config=org.apache.kafka.common.security.plain.PlainLoginModule required username="<sasl.username>" password="<sasl.password>";

    consumer.partition.assignment.strategy=org.apache.kafka.clients.consumer.RoundRobinAssignor
    consumer.check.crcs=false

    # Converter properties
    key.converter=org.apache.kafka.connect.storage.StringConverter
    value.converter=org.apache.kafka.connect.storage.StringConverter
    key.converter.schemas.enable=false
    value.converter.schemas.enable=false

    # Worker properties
    plugin.path=/etc/kafka-connect-worker/plugins
    offset.storage.file.filename=/etc/kafka-connect-worker/worker.offset
    ```

5. Create a FileSink connector configuration file `/etc/kafka-connect-worker/file-sink.properties` for transferring data from a {{ ydb-short-name }} topic to a file:

    ```ini
    name=local-file-sink
    connector.class=FileStreamSink
    tasks.max=1
    file=/etc/kafka-connect-worker/file_to_write.json
    topics=<topic-name>
    ```

    Where:

    * `file` - name of the file where the connector will write data.
    * `topics` - name of the topic from which the connector will read data.

6. Start Kafka Connect in Standalone mode:

    ```bash
    cd ~/opt/kafka/bin/ && \
    sudo ./connect-standalone.sh \
            /etc/kafka-connect-worker/worker.properties \
            /etc/kafka-connect-worker/file-sink.properties
    ```