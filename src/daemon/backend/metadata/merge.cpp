/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <daemon/backend/metadata/merge.hpp>

using namespace std;

namespace gkfs {
namespace metadata {

string MergeOperand::serialize_id() const {
    string s;
    s.reserve(2);
    s += (char) id(); // TODO check if static_cast can be used
    s += operand_id_suffix;
    return s;
}

string MergeOperand::serialize() const {
    string s = serialize_id();
    s += serialize_params();
    return s;
}

OperandID MergeOperand::get_id(const rdb::Slice& serialized_op) {
    return static_cast<OperandID>(serialized_op[0]);
}

rdb::Slice MergeOperand::get_params(const rdb::Slice& serialized_op) {
    assert(serialized_op[1] == operand_id_suffix);
    return {serialized_op.data() + 2, serialized_op.size() - 2};
}

IncreaseSizeOperand::IncreaseSizeOperand(const size_t size, const bool append) :
        size(size), append(append) {}

IncreaseSizeOperand::IncreaseSizeOperand(const rdb::Slice& serialized_op) {
    size_t chrs_parsed = 0;
    size_t read = 0;

    //Parse size
    size = ::stoul(serialized_op.data() + chrs_parsed, &read);
    chrs_parsed += read + 1;
    assert(serialized_op[chrs_parsed - 1] == separator);

    //Parse append flag
    assert(serialized_op[chrs_parsed] == false_char ||
           serialized_op[chrs_parsed] == true_char);
    append = serialized_op[chrs_parsed] != false_char;
    //check that we consumed all the input string
    assert(chrs_parsed + 1 == serialized_op.size());
}

OperandID IncreaseSizeOperand::id() const {
    return OperandID::increase_size;
}

string IncreaseSizeOperand::serialize_params() const {
    string s;
    s.reserve(3);
    s += ::to_string(size);
    s += this->separator;
    s += !append ? false_char : true_char;
    return s;
}


DecreaseSizeOperand::DecreaseSizeOperand(const size_t size) :
        size(size) {}

DecreaseSizeOperand::DecreaseSizeOperand(const rdb::Slice& serialized_op) {
    //Parse size
    size_t read = 0;
    //we need to convert serialized_op to a string because it doesn't contain the
    //leading slash needed by stoul
    size = ::stoul(serialized_op.ToString(), &read);
    //check that we consumed all the input string
    assert(read == serialized_op.size());
}

OperandID DecreaseSizeOperand::id() const {
    return OperandID::decrease_size;
}

string DecreaseSizeOperand::serialize_params() const {
    return ::to_string(size);
}


CreateOperand::CreateOperand(const string& metadata) : metadata(metadata) {}

OperandID CreateOperand::id() const {
    return OperandID::create;
}

string CreateOperand::serialize_params() const {
    return metadata;
}


bool MetadataMergeOperator::FullMergeV2(
        const MergeOperationInput& merge_in,
        MergeOperationOutput* merge_out) const {

    string prev_md_value;
    auto ops_it = merge_in.operand_list.cbegin();

    if (merge_in.existing_value == nullptr) {
        //The key to operate on doesn't exists in DB
        if (MergeOperand::get_id(ops_it[0]) != OperandID::create) {
            throw ::runtime_error("Merge operation failed: key do not exists and first operand is not a creation");
            // TODO use logger to print err info;
            //Log(logger, "Key %s do not exists", existing_value->ToString().c_str());
            //return false;
        }
        prev_md_value = MergeOperand::get_params(ops_it[0]).ToString();
        ops_it++;
    } else {
        prev_md_value = merge_in.existing_value->ToString();
    }

    Metadata md{prev_md_value};

    size_t fsize = md.size();

    for (; ops_it != merge_in.operand_list.cend(); ++ops_it) {
        const rdb::Slice& serialized_op = *ops_it;
        assert(serialized_op.size() >= 2);
        auto operand_id = MergeOperand::get_id(serialized_op);
        auto parameters = MergeOperand::get_params(serialized_op);

        if (operand_id == OperandID::increase_size) {
            auto op = IncreaseSizeOperand(parameters);
            if (op.append) {
                //append mode, just increment file
                fsize += op.size;
            } else {
                fsize = ::max(op.size, fsize);
            }
        } else if (operand_id == OperandID::decrease_size) {
            auto op = DecreaseSizeOperand(parameters);
            assert(op.size < fsize); // we assume no concurrency here
            fsize = op.size;
        } else if (operand_id == OperandID::create) {
            continue;
        } else {
            throw ::runtime_error("Unrecognized merge operand ID: " + (char) operand_id);
        }
    }

    md.size(fsize);
    merge_out->new_value = md.serialize();
    return true;
}

bool MetadataMergeOperator::PartialMergeMulti(const rdb::Slice& key,
                                              const ::deque<rdb::Slice>& operand_list,
                                              string* new_value, rdb::Logger* logger) const {
    return false;
}

const char* MetadataMergeOperator::Name() const {
    return "MetadataMergeOperator";
}

bool MetadataMergeOperator::AllowSingleOperand() const {
    return true;
}

} // namespace metadata
} // namespace gkfs