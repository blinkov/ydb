#pragma once

#include <contrib/ydb/core/protos/kqp.pb.h>
#include <contrib/ydb/library/yql/minikql/mkql_node.h>
#include <contrib/ydb/library/yql/minikql/computation/mkql_computation_node_holders.h>
#include <contrib/ydb/core/scheme/scheme_tabledefs.h>
#include <contrib/ydb/core/tx/datashard/sys_tables.h>
#include <contrib/ydb/core/tx/datashard/datashard.h>

namespace NKikimr {
namespace NKqp {

class TKqpStreamLookupWorker {
public:
    using TReadList = std::vector<std::pair<ui64, THolder<TEvDataShard::TEvRead>>>;
    using TPartitionInfo = std::shared_ptr<const TVector<TKeyDesc::TPartitionInfo>>;

    struct TShardReadResult {
        const ui64 ShardId;
        THolder<TEventHandle<TEvDataShard::TEvReadResult>> ReadResult;
        size_t UnprocessedResultRow = 0;
    };

    struct TReadResultStats {
        ui64 RowsCount = 0;
        ui64 BytesCount = 0;
    };

public:
    TKqpStreamLookupWorker(NKikimrKqp::TKqpStreamLookupSettings&& settings,
        const NMiniKQL::TTypeEnvironment& typeEnv, const NMiniKQL::THolderFactory& holderFactory);

    virtual ~TKqpStreamLookupWorker();

    virtual std::string GetTablePath() const;
    virtual TTableId GetTableId() const;
    virtual std::vector<NScheme::TTypeInfo> GetKeyColumnTypes() const;

    virtual void AddInputRow(NUdf::TUnboxedValue inputRow) = 0;
    virtual TReadList BuildRequests(const TPartitionInfo& partitioning, ui64& readId) = 0;
    virtual void AddResult(TShardReadResult result) = 0;
    virtual TReadResultStats ReplyResult(NKikimr::NMiniKQL::TUnboxedValueBatch& batch, i64 freeSpace) = 0;
    virtual bool AllRowsProcessed() = 0;
    virtual void ResetRowsProcessing(ui64 readId, ui32 firstUnprocessedQuery, TMaybe<TOwnedCellVec> lastProcessedKey) = 0;

protected:
    const NMiniKQL::TTypeEnvironment& TypeEnv;
    const NMiniKQL::THolderFactory& HolderFactory;
    const TString TablePath;
    const TTableId TableId;
    std::unordered_map<TString, TSysTables::TTableColumnInfo> KeyColumns;
    std::vector<TSysTables::TTableColumnInfo*> LookupKeyColumns;
    std::vector<TSysTables::TTableColumnInfo> Columns;
};

std::unique_ptr<TKqpStreamLookupWorker> CreateStreamLookupWorker(NKikimrKqp::TKqpStreamLookupSettings&& settings,
    const NMiniKQL::TTypeEnvironment& typeEnv, const NMiniKQL::THolderFactory& holderFactory);

} // namespace NKqp
} // namespace NKikimr
