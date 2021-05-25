/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GEKKOFS_DB_EXCEPTIONS_HPP
#define GEKKOFS_DB_EXCEPTIONS_HPP

#include <string>
#include <stdexcept>

namespace gkfs {
namespace metadata {

class DBException : public std::runtime_error {
public:
    explicit DBException(const std::string& s) : std::runtime_error(s) {};
};

class NotFoundException : public DBException {
public:
    explicit NotFoundException(const std::string& s) : DBException(s) {};
};

} // namespace metadata
} // namespace gkfs

#endif //GEKKOFS_DB_EXCEPTIONS_HPP
