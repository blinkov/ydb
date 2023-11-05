PY3TEST()

TEST_SRCS(
    test_doc.py
)

SIZE(MEDIUM)
TIMEOUT(600)

REQUIREMENTS(
    cpu:4
    ram:32
)

DATA(
    arcadia/contrib/ydb/docs/ru/core/postgresql/_includes/functions.md
    arcadia/contrib/ydb/library/yql/cfg/udf_test
    arcadia/contrib/ydb/library/yql/mount
)

PEERDIR(
    contrib/ydb/library/yql/tests/common/test_framework
)

DEPENDS(
    contrib/ydb/library/yql/tools/yqlrun
    contrib/ydb/library/yql/udfs/common/re2
)

IF (SANITIZER_TYPE == "memory")
    TAG(ya:not_autocheck) # YQL-15385
ENDIF()

END()
