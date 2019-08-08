/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <global/path_util.hpp>
#include <unistd.h>
#include <system_error>
#include <cstring>
#include <cassert>
#include <sys/stat.h>


bool is_relative_path(const std::string& path) {
    return (!path.empty()) &&
           (path.front() != PSP);
}

bool is_absolute_path(const std::string& path) {
    return (!path.empty()) &&
           (path.front() == PSP);
}

bool has_trailing_slash(const std::string& path) {
    return (!path.empty()) && (path.back() == PSP);
}

/* Add path prefix to a given C string.
 *
 * Returns a string composed by the `prefix_path`
 * followed by `raw_path`.
 *
 * This would return the same of:
 * ```
 * std::string(raw_path).append(prefix_path);
 * ```
 * But it is faster because it avoids to copy the `raw_path` twice.
 *
 *
 * Time cost approx: O(len(prefix_path)) + 2 O(len(raw_path))
 *
 * Example:
 * ```
 * prepend_path("/tmp/prefix", "./my/path") == "/tmp/prefix/./my/path"
 * ```
 */
std::string prepend_path(const std::string& prefix_path, const char * raw_path) {
    assert(!has_trailing_slash(prefix_path));
    std::size_t raw_len = std::strlen(raw_path);
    std::string res;
    res.reserve(prefix_path.size() + 1 + raw_len);
    res.append(prefix_path);
    res.push_back(PSP);
    res.append(raw_path, raw_len);
    return res;
}

/* Split a path into its components
 *
 * Returns a vector of the components of the given path.
 *
 * Example:
 *  split_path("/first/second/third") == ["first", "second", "third"]
 */
std::vector<std::string> split_path(const std::string& path) {
    std::vector<std::string> tokens;
    size_t start = std::string::npos;
    size_t end = (path.front() != PSP)? 0 : 1;
    while(end != std::string::npos && end < path.size()) {
        start = end;
        end = path.find(PSP, start);
        tokens.push_back(path.substr(start, end - start));
        if(end != std::string::npos) {
            ++end;
        }
    }
    return tokens;
}





/* Make an absolute path relative to a root path
 *
 * Convert @absolute_path into a relative one with respect to the given @root_path.
 * If @absolute_path do not start at the given @root_path an empty string will be returned.
 * NOTE: Trailing slash will be stripped from the new constructed relative path.
 */
std::string path_to_relative(const std::string& root_path, const std::string& absolute_path) {
    assert(is_absolute_path(root_path));
    assert(is_absolute_path(absolute_path));
    assert(!has_trailing_slash(root_path));

    auto diff_its = std::mismatch(absolute_path.cbegin(), absolute_path.cend(), root_path.cbegin());
    if(diff_its.second != root_path.cend()){
        // complete path doesn't start with root_path
        return {};
    }

    // iterator to the starting char of the relative portion of the @absolute_path
    auto rel_it_begin = diff_its.first;
    // iterator to the end of the relative portion of the @absolute_path
    auto rel_it_end = absolute_path.cend();

    // relative path start exactly after the root_path prefix
    assert((size_t)(rel_it_begin - absolute_path.cbegin()) == root_path.size());

    if(rel_it_begin == rel_it_end) {
        //relative path is empty, @absolute_path was equal to @root_path
        return {'/'};
    }

    // remove the trailing slash from relative path
    if(has_trailing_slash(absolute_path) &&
      rel_it_begin != rel_it_end - 1) { // the relative path is longer then 1 char ('/')
        --rel_it_end;
    }

    return {rel_it_begin, rel_it_end};
}

std::string dirname(const std::string& path) {
    assert(path.size() > 1 || path.front() == PSP);
    assert(path.size() == 1 || !has_trailing_slash(path));

    auto parent_path_size = path.find_last_of(PSP);
    assert(parent_path_size != std::string::npos);
    if(parent_path_size == 0) {
        // parent is '/'
        parent_path_size = 1;
    }
    return path.substr(0, parent_path_size);
}
