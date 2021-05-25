/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <daemon/backend/data/data_module.hpp>

namespace gkfs {
namespace data {

const std::shared_ptr<spdlog::logger>& DataModule::log() const {
    return log_;
}

void DataModule::log(const std::shared_ptr<spdlog::logger>& log) {
    DataModule::log_ = log;
}

} // namespace data
} // namespace gkfs