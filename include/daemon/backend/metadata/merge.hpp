/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef DB_MERGE_HPP
#define DB_MERGE_HPP


#include <rocksdb/merge_operator.h>
#include <global/metadata.hpp>

namespace rdb = rocksdb;

namespace gkfs {
namespace metadata {

enum class OperandID : char {
    increase_size = 'i',
    decrease_size = 'd',
    create = 'c'
};

class MergeOperand {
public:
    constexpr static char operand_id_suffix = ':';

    std::string serialize() const;

    static OperandID get_id(const rdb::Slice& serialized_op);

    static rdb::Slice get_params(const rdb::Slice& serialized_op);

protected:
    std::string serialize_id() const;

    virtual std::string serialize_params() const = 0;

    virtual OperandID id() const = 0;
};

class IncreaseSizeOperand : public MergeOperand {
public:
    constexpr const static char separator = ',';
    constexpr const static char true_char = 't';
    constexpr const static char false_char = 'f';

    size_t size;
    bool append;

    IncreaseSizeOperand(size_t size, bool append);

    explicit IncreaseSizeOperand(const rdb::Slice& serialized_op);

    OperandID id() const override;

    std::string serialize_params() const override;
};

class DecreaseSizeOperand : public MergeOperand {
public:
    size_t size;

    explicit DecreaseSizeOperand(size_t size);

    explicit DecreaseSizeOperand(const rdb::Slice& serialized_op);

    OperandID id() const override;

    std::string serialize_params() const override;
};

class CreateOperand : public MergeOperand {
public:
    std::string metadata;

    explicit CreateOperand(const std::string& metadata);

    OperandID id() const override;

    std::string serialize_params() const override;
};

class MetadataMergeOperator : public rocksdb::MergeOperator {
public:
    ~MetadataMergeOperator() override = default;

    bool FullMergeV2(const MergeOperationInput& merge_in,
                     MergeOperationOutput* merge_out) const override;

    bool PartialMergeMulti(const rdb::Slice& key,
                           const std::deque<rdb::Slice>& operand_list,
                           std::string* new_value, rdb::Logger* logger) const override;

    const char* Name() const override;

    bool AllowSingleOperand() const override;
};

} // namespace metadata
} // namespace gkfs

#endif // DB_MERGE_HPP
