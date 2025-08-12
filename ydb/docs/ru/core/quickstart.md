# Быстрый старт c {{ydb-short-name}}

В этом гайде вы установите одно-узловой локальный [кластер {{ ydb-short-name }}](concepts/glossary.md#cluster) и сделаете простые запросы в свою [бд](concepts/glossary.md#database).

Обычно {{ ydb-short-name }} хранит данные на нескольких SSD/NVMe дисках или HDD без ФС, но для удобства тут мы эмулируем диски в RAM или в файле обычной ФС. Поэтому, эта конфигурация **НЕ** годится для продакшн и даже для теста производительности - см. [дока для DevOps](devops/index.md) для запуска в продакшене.

## Установка и запуск {#Install}

{% list tabs %}

- Linux x86_64

   {% note info %}

   Рекомендуемая ОС — Linux x86-64. Если нет такой, переключитесь на "Docker" инструкцию

   {% endnote %}

   1. Создайте папку для теста {{ ydb-short-name }} и сделайте её рабочей:
      ```
      mkdir ~/ydbd && cd ~/ydbd
      ```
   2. Скачайте и запустите установочный скрипт:
      ```bash
      curl {{ ydbd-install-url }}|bash
      ```
      Скрипт скачает и распакует `ydbd`, либы, конфиги и скрипты для старта/стопа.  
      Он работает без sudo поэтому не изменит ничего критического. Чтобы увидеть команды — откройте этот url в браузере.
   3. Запустите кластер в одном из режимов:
      * RAM:
        ```
        ./start.sh  ram
        ```
      * Файл:
        ```
        ./start.sh disk
        ```
        Создаст `ydb.data` 80Gb (убедитесь, что есть место)
      * Диск:
        ```
        ./start.sh drive /dev/$DRIVE_NAME
        ```
        **Важно:** накопитель будет очищен. Рекомендуем NVME/SSD ≥ 800 GB.

- Docker x86_64

   1. Создайте папку и подпапки:
      ```bash
      mkdir ~/ydbd && cd ~/ydbd
      mkdir ydb_data ydb_certs
      ```
   2. Запустите контейнер:
      ```bash
      docker run -d --rm --name ydb-local -h localhost         --platform linux/amd64         -p 2135:2135 -p 2136:2136 -p 8765:8765 -p 9092:9092         -v $(pwd)/ydb_certs:/ydb_certs -v $(pwd)/ydb_data:/ydb_data         -e GRPC_TLS_PORT=2135 -e GRPC_PORT=2136 -e MON_PORT=8765         -e YDB_KAFKA_PROXY_PORT=9092         {{ydb_local_docker_image}}:{{ ydb_local_docker_image_tag}}
      ```

{% endlist %}

## Первый запрос Hello world!

Откройте [localhost:8765](http://localhost:8765) (или замените `localhost` на адрес сервера) — это веб интерфейс {{ ydb-short-name }}.  
Найдите `/Root/test` (или `/local`) и введите:
```yql
SELECT "Hello, world!"u
```
Результат: приветствие.  

{% note info %}
Суффикс `u` значит тип `Utf8`. [Подробнее](yql/reference/types/index.md)
{% endnote %}

## Создание таблицы

```yql
CREATE TABLE example(
  key UInt64,
  value String,
  PRIMARY KEY(key)
)
```

## Добавление данных

```yql
INSERT INTO example (key, value)
VALUES (123, "hello"), (321, "world")
```
Чтобы проверить:
```yql
SELECT COUNT(*) FROM example;
```

## Остановка {#stop}

{% list tabs %}
- Linux:
  ```bash
  ~/ydbd/stop.sh
  ```
- Docker:
  ```bash
  docker kill ydb-local
  ```
{% endlist %}
