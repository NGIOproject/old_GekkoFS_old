/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef IFS_DB_EXCEPTIONS_HPP
#define IFS_DB_EXCEPTIONS_HPP

#include <string>
#include <stdexcept>

class DBException: public std::runtime_error {
    public:
        DBException(const std::string & s) : std::runtime_error(s) {};
};

class NotFoundException: public DBException {
    public:
        NotFoundException(const std::string & s) : DBException(s) {};
};

#endif //IFS_DB_EXCEPTIONS_HPP
