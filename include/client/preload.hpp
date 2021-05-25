/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef IOINTERCEPT_PRELOAD_HPP
#define IOINTERCEPT_PRELOAD_HPP

#include <client/preload_context.hpp>

#define EUNKNOWN (-1)

#define CTX gkfs::preload::PreloadContext::getInstance()
namespace gkfs {
namespace preload {
void init_ld_env_if_needed();
} // namespace preload
} // namespace gkfs

void init_preload() __attribute__((constructor));

void destroy_preload() __attribute__((destructor));


#endif //IOINTERCEPT_PRELOAD_HPP
