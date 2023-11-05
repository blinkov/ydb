#include "mark.h"
#include <contrib/ydb/library/yverify_stream/yverify_stream.h>
#include <contrib/ydb/core/tx/columnshard/common/scalars.h>

namespace NKikimr::NOlap {

TString TMark::SerializeScalar(const NArrow::TReplaceKey& key, const std::shared_ptr<arrow::Schema>& schema) {
    Y_ABORT_UNLESS(key.Size() == 1);
    Y_VERIFY_S(key.Column(0).type()->Equals(schema->field(0)->type()),
        key.Column(0).type()->ToString() << ", expected " << schema->ToString());
    return SerializeKeyScalar(NArrow::TReplaceKey::ToScalar(key));
}

NArrow::TReplaceKey TMark::DeserializeScalar(const TString& key, const std::shared_ptr<arrow::Schema>& schema) {
    Y_ABORT_UNLESS(schema->num_fields() > 0);
    return NArrow::TReplaceKey::FromScalar(DeserializeKeyScalar(key, schema->field(0)->type()));
}

TString TMark::SerializeComposite(const NArrow::TReplaceKey& key, const std::shared_ptr<arrow::Schema>& schema) {
    auto batch = key.ToBatch(schema);
    Y_ABORT_UNLESS(batch && batch->num_rows() == 1);
    return NArrow::SerializeBatchNoCompression(batch);
}

NArrow::TReplaceKey TMark::DeserializeComposite(const TString& key, const std::shared_ptr<arrow::Schema>& schema) {
    auto batch = NArrow::DeserializeBatch(key, schema);
    Y_ABORT_UNLESS(batch && batch->num_rows() == 1);
    return NArrow::TReplaceKey::FromBatch(batch, 0);
}

std::string TMark::ToString() const {
    if (Border.Size() == 1) {
        return NArrow::TReplaceKey::ToScalar(Border)->ToString();
    } else {
        TStringBuilder out;
        out << "(";
        for (int i = 0; i < Border.Size(); ++i) {
            if (i) {
                out << ", ";
            }
            out << NArrow::TReplaceKey::ToScalar(Border, i)->ToString();
        }
        out << ")";
        return out;
    }
}

std::shared_ptr<arrow::Scalar> TMark::MinScalar(const std::shared_ptr<arrow::DataType>& type) {
    if (type->id() == arrow::Type::TIMESTAMP) {
        // TODO: support negative timestamps in index
        return std::make_shared<arrow::TimestampScalar>(0, type);
    }
    return NArrow::MinScalar(type);
}

NArrow::TReplaceKey TMark::MinBorder(const std::shared_ptr<arrow::Schema>& schema) {
    if (schema->num_fields() == 1) {
        return NArrow::TReplaceKey::FromScalar(MinScalar(schema->field(0)->type()));
    } else {
        std::vector<std::shared_ptr<arrow::Array>> columns;
        columns.reserve(schema->num_fields());
        for (const auto& field : schema->fields()) {
            auto scalar = MinScalar(field->type());
            Y_DEBUG_ABORT_UNLESS(scalar);
            auto res = arrow::MakeArrayFromScalar(*scalar, 1);
            Y_DEBUG_ABORT_UNLESS(res.ok());
            columns.emplace_back(*res);
        }
        return NArrow::TReplaceKey::FromBatch(arrow::RecordBatch::Make(schema, 1, columns), 0);
    }
}

NArrow::TReplaceKey TMark::ExtendBorder(const NArrow::TReplaceKey& key,
    const std::shared_ptr<arrow::Schema>& schema) {
    std::vector<std::shared_ptr<arrow::Array>> columns;
    i32 numFields = schema->num_fields();
    columns.reserve(numFields);
    for (i32 i = 0; i < numFields; ++i) {
        const auto& field = schema->field(i);
        if (i < key.Size()) {
            columns.emplace_back(key.ColumnPtr(i));
            Y_ABORT_UNLESS(columns.back()->type()->Equals(field->type()));
        } else {
            auto scalar = MinScalar(field->type());
            Y_ABORT_UNLESS(scalar);
            auto res = arrow::MakeArrayFromScalar(*scalar, 1);
            Y_ABORT_UNLESS(res.ok());
            columns.emplace_back(*res);
        }
    }
    return NArrow::TReplaceKey::FromBatch(arrow::RecordBatch::Make(schema, 1, columns), 0);
}

}
