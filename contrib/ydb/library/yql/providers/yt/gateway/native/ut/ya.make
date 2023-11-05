UNITTEST()

SRCS(
    yql_yt_native_folders_ut.cpp
)

PEERDIR(
    contrib/ydb/library/yql/providers/yt/gateway/native
    contrib/ydb/library/yql/providers/yt/gateway/file
    contrib/ydb/library/yql/core/ut_common
    library/cpp/testing/mock_server
    library/cpp/testing/common
    contrib/ydb/library/yql/public/udf/service/terminate_policy
    contrib/ydb/library/yql/sql/pg
)

YQL_LAST_ABI_VERSION()

END()
