# Примеры использования Kafka API

В этой статье приведены примеры использования Kafka API для работы с [топиками {{ ydb-short-name }}](../../concepts/topic.md).

Перед выполнением примеров [создайте топик](../ydb-cli/topic-create.md) и [добавьте читателя](../ydb-cli/topic-consumer-add.md).

## Примеры работы с топиками

В примерах используются:

* `ydb:9093` — имя хоста и порт.
* `/Root/Database` — имя базы данных.
* `/Root/Database/Topic-1` — имя топика. Допускается указывать как полное имя (вместе с базой данных), так и только имя топика.
* `user@/Root/Database` — имя пользователя. Имя пользователя включает имя базы данных, которое указывается после `@`.
* `*****` — пароль пользователя.
* `consumer-1` — имя читателя.


## Запись данных в топик

### Запись через Kafka Java SDK

В этом примере приведен фрагмент кода для записи данных в топик через [Kafka API](https://kafka.apache.org/documentation/).

```java
String HOST = "ydb:9093";
String TOPIC = "/Root/Database/Topic-1";
String USER = "user@/Root/Database";
String PASS = "*****";

Properties props = new Properties();
props.put("bootstrap.servers", HOST);
props.put("acks", "all");

props.put("key.serializer", StringSerializer.class.getName());
props.put("key.deserializer", StringDeserializer.class.getName());
props.put("value.serializer", StringSerializer.class.getName());
props.put("value.deserializer", StringDeserializer.class.getName());

props.put("security.protocol", "SASL_SSL");
props.put("sasl.mechanism", "PLAIN");
props.put("sasl.jaas.config", PlainLoginModule.class.getName() + " required username=\"" + USER + "\" password=\"" + PASS + "\";");

props.put("compression.type", "none");

Producer<String, String> producer = new KafkaProducer<>(props);
producer.send(new ProducerRecord<String, String>(TOPIC, "msg-key", "msg-body"));
producer.flush();
producer.close();
```

### Запись через Logstash

Для настройки [Logstash](https://github.com/elastic/logstash) используйте следующие параметры:

```ruby
output {
  kafka {
    codec => json
    topic_id => "/Root/Database/Topic-1"
    bootstrap_servers => "ydb:9093"
    compression_type => none
    security_protocol => SASL_SSL
    sasl_mechanism => PLAIN
    sasl_jaas_config => "org.apache.kafka.common.security.plain.PlainLoginModule required username='user@/Root/Database' password='*****';"
  }
}
```

### Запись через Fluent Bit

Для настройки [Fluent Bit](https://github.com/fluent/fluent-bit) используйте следующие параметры:

```ini
[OUTPUT]
  name                          kafka
  match                         *
  Brokers                       ydb:9093
  Topics                        /Root/Database/Topic-1
  rdkafka.client.id             Fluent-bit
  rdkafka.request.required.acks 1
  rdkafka.log_level             7
  rdkafka.security.protocol     SASL_SSL
  rdkafka.sasl.mechanism        PLAIN
  rdkafka.sasl.username         user@/Root/Database
  rdkafka.sasl.password         *****
```

## Чтение данных из топика

### Чтение данных из топика через Kafka Java SDK

В этом примере приведен фрагмент кода для чтения данных из топика через Kafka Java SDK.

```java
String HOST = "ydb:9093";
String TOPIC = "/Root/Database/Topic-1";
String USER = "user@/Root/Database";
String PASS = "*****";
String CONSUMER = "consumer-1";

Properties props = new Properties();

props.put("bootstrap.servers", HOST);

props.put("security.protocol", "SASL_SSL");
props.put("sasl.mechanism", "PLAIN");
props.put("sasl.jaas.config", PlainLoginModule.class.getName() + " required username=\"" + USER + "\" password=\"" + PASS + "\";");

props.put("key.deserializer", StringDeserializer.class.getName());
props.put("value.deserializer", StringDeserializer.class.getName());

props.put("check.crcs", false);
props.put("partition.assignment.strategy", RoundRobinAssignor.class.getName());

props.put("group.id", CONSUMER);
Consumer<String, String> consumer = new KafkaConsumer<>(props);
consumer.subscribe(Arrays.asList(new String[] {TOPIC}));

while (true) {
    ConsumerRecords<String, String> records = consumer.poll(10000); // timeout 10 sec
    for (ConsumerRecord<String, String> record : records) {
        System.out.println(record.key() + ":" + record.value());
    }
}
```

### Чтение данных из топика через Kafka Java SDK без группы читателей

В этом примере приведен фрагмент кода для чтения данных из топика через Kafka API без группы читателей (Manual Partition Assignment).
Для такого режима чтения ��е нужно создавать читателя.

```java
String HOST = "ydb:9093";
String TOPIC = "/Root/Database/Topic-1";
String USER = "user@/Root/Database";
String PASS = "*****";

Properties props = new Properties();

props.put("bootstrap.servers", HOST);

props.put("security.protocol", "SASL_SSL");
props.put("sasl.mechanism", "PLAIN");
props.put("sasl.jaas.config", PlainLoginModule.class.getName() + " required username=\"" + USER + "\" password=\"" + PASS + "\";");

props.put("key.deserializer", StringDeserializer.class.getName());
props.put("value.deserializer", StringDeserializer.class.getName());

props.put("check.crcs", false);
props.put("auto.offset.reset", "earliest"); // чтение с начала

Consumer<String, String> consumer = new KafkaConsumer<>(props);

List<PartitionInfo> partitionInfos = consumer.partitionsFor(TOPIC);
List<TopicPartition> topicPartitions = new ArrayList<>();

for (PartitionInfo partitionInfo : partitionInfos) {
    topicPartitions.add(new TopicPartition(partitionInfo.topic(), partitionInfo.partition()));
}
consumer.assign(topicPartitions);

while (true) {
    ConsumerRecords<String, String> records = consumer.poll(10000); // timeout 10 sec
    for (ConsumerRecord<String, String> record : records) {
        System.out.println(record.key() + ":" + record.value());
    }
}
```

## Использование Kafka Connect

Инструмент [Kafka Connect](https://kafka.apache.org/documentation/#connect) предназначен для перемещения данных между Apache Kafka® и другими хранилищами данных.

Данные в Kafka Connect обрабатываются процессами-исполнителями.

{% note warning %}

Экземпляры Kafka Connect для работы с {{ ydb-short-name }} следует развертывать только в автономном режиме. {{ ydb-short-name }} не поддерживает Kafka Connect в распределенном режиме.

{% endnote %}

Фактическое перемещение данных выполняется с помощью коннекторов, которые работают в отдельных потоках исполняющего процесса.

Подробнее о Kafka Connect и его настройке читайте в документации [Apache Kafka®](https://kafka.apache.org/documentation/#connect).

### Настройка Kafka Connect

1. [Создайте читателя](../ydb-cli/topic-consumer-add.md) с именем `connect-<connector-name>`. Имя коннектора указывается в конфигурационном файле при его настройке в поле `name`.

1. [Скачайте](https://downloads.apache.org/kafka/) и распакуйте архив Apache Kafka®:

    ```bash
    wget https://downloads.apache.org/kafka/3.6.1/kafka_2.13-3.6.1.tgz && tar -xvf kafka_2.13-3.6.1.tgz --strip 1 --directory /opt/kafka/
    ```

    В этом примере используется версия Apache Kafka® 3.6.1.


1. Создайте директорию с настройками процесса-исполнителя:

    ```bash
    sudo mkdir --parents /etc/kafka-connect-worker
    ```

1. Создайте файл настроек процесса-исполнителя `/etc/kafka-connect-worker/worker.properties`:

    ```ini
    # Основные свойства
    bootstrap.servers=ydb:9093

    # Свойства AdminAPI
    sasl.mechanism=PLAIN
    security.protocol=SASL_SSL
    sasl.jaas.config=org.apache.kafka.common.security.plain.PlainLoginModule required username="<user>@<db>" password="<user-pass>";

    # Свойства Producer
    producer.sasl.mechanism=PLAIN
    producer.security.protocol=SASL_SSL
    producer.sasl.jaas.config=org.apache.kafka.common.security.plain.PlainLoginModule required username="<user>@<db>" password="<user-pass>";

    # Свойства Consumer
    consumer.sasl.mechanism=PLAIN
    consumer.security.protocol=SASL_SSL
    consumer.sasl.jaas.config=org.apache.kafka.common.security.plain.PlainLoginModule required username="<user>@<db>" password="<user-pass>";

    consumer.partition.assignment.strategy=org.apache.kafka.kafka.clients.consumer.RoundRobinAssignor
    consumer.check.crcs=false

    # Свойства конвертера
    key.converter=org.apache.kafka.connect.storage.StringConverter
    value.converter=org.apache.kafka.connect.storage.StringConverter
    key.converter.schemas.enable=false
    value.converter.schemas.enable=false

    # Свойства Worker
    plugin.path=/etc/kafka-connect-worker/plugins
    offset.storage.file.filename=/etc/kafka-connect-worker/worker.offset
    ```

1. Создайте файл настроек коннектора FileSink `/etc/kafka-connect-worker/file-sink.properties` для перемещения данных из топиков {{ ydb-short-name }} в файл:

    ```ini
    name=local-file-sink
    connector.class=FileStreamSink
    tasks.max=1
    file=/etc/kafka-connect-worker/file_to_write.json
    topics=Topic-1
    ```
```