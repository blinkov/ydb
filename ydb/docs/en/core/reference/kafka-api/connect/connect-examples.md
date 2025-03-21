# Connector Configuration Examples

This section provides examples of Kafka Connect connector configuration files for working with {{ ydb-short-name }} via the Kafka protocol.

## From File to {{ ydb-short-name }}

Example of a FileSource connector configuration file `/etc/kafka-connect-worker/file-sink.properties` for transferring data from a file to a topic:

```ini
name=local-file-source
connector.class=FileStreamSource
tasks.max=1
file=/etc/kafka-connect-worker/file_to_read.json
topic=<topic-name>
```

## From {{ ydb-short-name }} to PostgreSQL

Example of a JDBCSink connector configuration file `/etc/kafka-connect-worker/jdbc-sink.properties` for transferring data from a topic to a PostgreSQL table. Uses the [Kafka Connect JDBC Connector](https://github.com/confluentinc/kafka-connect-jdbc).

```ini
name=postgresql-sink
connector.class=io.confluent.connect.jdbc.JdbcSinkConnector

connection.url=jdbc:postgresql://<postgresql-host>:<postgresql-port>/<db>
connection.user=<pg-user>
connection.password=<pg-user-pass>

topics=<topic-name>
batch.size=2000
auto.commit.interval.ms=1000

transforms=wrap
transforms.wrap.type=org.apache.kafka.connect.transforms.HoistField$Value
transforms.wrap.field=data

auto.create=true
insert.mode=insert
pk.mode=none
auto.evolve=true
```

## From PostgreSQL to {{ ydb-short-name }}

Example of a JDBCSource connector configuration file `/etc/kafka-connect-worker/jdbc-source.properties` for transferring data from a PostgreSQL table to a topic. Uses the [Kafka Connect JDBC Connector](https://github.com/confluentinc/kafka-connect-jdbc).

```ini
name=postgresql-source
connector.class=io.confluent.connect.jdbc.JdbcSourceConnector

connection.url=jdbc:postgresql://<postgresql-host>:<postgresql-port>/<db>
connection.user=<pg-user>
connection.password=<pg-user-pass>

mode=bulk
query=SELECT * FROM "<topic-name>";
topic.prefix=<topic-name>
poll.interval.ms=1000
validate.non.null=false
```

## From {{ ydb-short-name }} to S3

Example of an S3Sink connector configuration file `/etc/kafka-connect-worker/s3-sink.properties` for transferring data from a topic to S3. Uses the [Aiven's S3 Sink Connector for Apache Kafka](https://github.com/Aiven-Open/s3-connector-for-apache-kafka).

```ini
name=s3-sink
connector.class=io.aiven.kafka.connect.s3.AivenKafkaConnectS3SinkConnector
topics=<topic-name>
aws.access.key.id=<s3-access-key>
aws.secret.access.key=<s3-secret>
aws.s3.bucket.name=<bucket-name>
aws.s3.endpoint=<s3-endpoint>
format.output.type=json
file.compression.type=none
```