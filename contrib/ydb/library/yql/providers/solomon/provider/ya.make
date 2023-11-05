LIBRARY()

SRCS(
    yql_solomon_config.cpp
    yql_solomon_datasink_execution.cpp
    yql_solomon_datasink_type_ann.cpp
    yql_solomon_datasink.cpp
    yql_solomon_datasource_execution.cpp
    yql_solomon_datasource_type_ann.cpp
    yql_solomon_datasource.cpp
    yql_solomon_dq_integration.cpp
    yql_solomon_io_discovery.cpp
    yql_solomon_load_meta.cpp
    yql_solomon_physical_optimize.cpp
    yql_solomon_provider.cpp
)

PEERDIR(
    library/cpp/actors/protos
    contrib/ydb/library/yql/dq/expr_nodes
    contrib/ydb/library/yql/dq/integration
    contrib/ydb/library/yql/providers/common/config
    contrib/ydb/library/yql/providers/common/proto
    contrib/ydb/library/yql/providers/common/provider
    contrib/ydb/library/yql/providers/common/transform
    contrib/ydb/library/yql/providers/dq/expr_nodes
    contrib/ydb/library/yql/providers/result/expr_nodes
    contrib/ydb/library/yql/providers/solomon/expr_nodes
    contrib/ydb/library/yql/providers/solomon/proto
    contrib/ydb/library/yql/dq/opt
)

YQL_LAST_ABI_VERSION()

END()
