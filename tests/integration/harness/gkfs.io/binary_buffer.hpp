/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef IO_BINARY_BUFFER_HPP
#define IO_BINARY_BUFFER_HPP

#include <nlohmann/json.hpp>
#include <utility>
#include <fmt/format.h>


namespace io {

struct buffer {
    buffer(::size_t size = 0) :
        m_data(size) { }

    buffer(nullptr_t p) { }

    buffer(const std::string& s) {
        m_data.clear();
        std::copy(s.begin(), s.end(), std::back_inserter(m_data));
    }

    buffer(std::vector<uint8_t> data) : m_data(std::move(data)) {}

    bool
    operator==(nullptr_t) const {
        return m_data.empty();
    }

    bool
    operator!=(nullptr_t) const {
        return !m_data.empty();
    }

    auto
    data() {
        return m_data.data();
    }

    auto
    storage() const {
        return m_data;
    }

    std::size_t
    size() const {
        return m_data.size();
    }

    std::vector<uint8_t> m_data;
};

inline void
to_json(nlohmann::json& record, 
        const buffer& out) {

    if(out == nullptr) {
        record = nullptr;
    }
    else {

//    record = fmt::format("x{:2x}", fmt::join(out, "x"));
        record = out.storage();
    }
}

} // namespace io

#endif // IO_BINARY_BUFFER_HPP
