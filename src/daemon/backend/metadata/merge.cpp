/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <daemon/backend/metadata/merge.hpp>


std::string MergeOperand::serialize_id() const {
    std::string s;
    s.reserve(2);
    s += (char)id();
    s += operand_id_suffix;
    return s;
}

std::string MergeOperand::serialize() const {
    std::string s = serialize_id();
    s += serialize_params();
    return s;
}

OperandID MergeOperand::get_id(const rdb::Slice& serialized_op){
        return static_cast<OperandID>(serialized_op[0]);
}

rdb::Slice MergeOperand::get_params(const rdb::Slice& serialized_op){
    assert(serialized_op[1] == operand_id_suffix);
    return {serialized_op.data() + 2, serialized_op.size() - 2};
}

IncreaseSizeOperand::IncreaseSizeOperand(const size_t size, const bool append):
    size(size), append(append) {}

IncreaseSizeOperand::IncreaseSizeOperand(const rdb::Slice& serialized_op){
    size_t chrs_parsed = 0;
    size_t read = 0;

    //Parse size
    size = std::stoul(serialized_op.data() + chrs_parsed, &read);
    chrs_parsed += read + 1;
    assert(serialized_op[chrs_parsed - 1] == separator);

    //Parse append flag
    assert(serialized_op[chrs_parsed] == false_char ||
           serialized_op[chrs_parsed] == true_char);
    append = (serialized_op[chrs_parsed] == false_char) ? false : true;
    //check that we consumed all the input string
    assert(chrs_parsed + 1 == serialized_op.size());
}

OperandID IncreaseSizeOperand::id() const {
    return OperandID::increase_size;
}

std::string IncreaseSizeOperand::serialize_params() const {
    std::string s;
    s.reserve(3);
    s += std::to_string(size);
    s += this->separator;
    s += (append == false)? false_char : true_char;
    return s;
}


DecreaseSizeOperand::DecreaseSizeOperand(const size_t size) :
    size(size) {}

DecreaseSizeOperand::DecreaseSizeOperand(const rdb::Slice& serialized_op){
    //Parse size
    size_t read = 0;
    //we need to convert serialized_op to a string because it doesn't contain the
    //leading slash needed by stoul
    size = std::stoul(serialized_op.ToString(), &read);
    //check that we consumed all the input string
    assert(read == serialized_op.size());
}

OperandID DecreaseSizeOperand::id() const {
    return OperandID::decrease_size;
}

std::string DecreaseSizeOperand::serialize_params() const {
    return std::to_string(size);
}


CreateOperand::CreateOperand(const std::string& metadata): metadata(metadata) {}

OperandID CreateOperand::id() const{
    return OperandID::create;
}

std::string CreateOperand::serialize_params() const {
    return metadata;
}


bool MetadataMergeOperator::FullMergeV2(
        const MergeOperationInput& merge_in,
        MergeOperationOutput* merge_out) const {

    std::string prev_md_value;
    auto ops_it = merge_in.operand_list.cbegin();

    if(merge_in.existing_value == nullptr){
        //The key to operate on doesn't exists in DB
        if(MergeOperand::get_id(ops_it[0]) != OperandID::create){
            throw std::runtime_error("Merge operation failed: key do not exists and first operand is not a creation");
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

    for (; ops_it != merge_in.operand_list.cend(); ++ops_it){
        const rdb::Slice& serialized_op = *ops_it;
        assert(serialized_op.size() >= 2);
        auto operand_id = MergeOperand::get_id(serialized_op);
        auto parameters = MergeOperand::get_params(serialized_op);

        if(operand_id == OperandID::increase_size){
            auto op = IncreaseSizeOperand(parameters);
            if(op.append){
                //append mode, just increment file
                fsize += op.size;
            } else {
                fsize = std::max(op.size, fsize);
            }
        } else if(operand_id == OperandID::decrease_size) {
            auto op = DecreaseSizeOperand(parameters);
            assert(op.size < fsize); // we assume no concurrency here
            fsize = op.size;
        } else if(operand_id == OperandID::create){
            continue;
        } else {
            throw std::runtime_error("Unrecognized merge operand ID: " + (char)operand_id);
        }
    }

    md.size(fsize);
    merge_out->new_value = md.serialize();
    return true;
}

bool MetadataMergeOperator::PartialMergeMulti(const rdb::Slice& key,
                const std::deque<rdb::Slice>& operand_list,
                std::string* new_value, rdb::Logger* logger) const {
    return false;
}

const char* MetadataMergeOperator::Name() const {
    return "MetadataMergeOperator";
}

bool MetadataMergeOperator::AllowSingleOperand() const {
   return true;
}
