/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GEKKOFS_CHNK_CALC_UTIL_HPP
#define GEKKOFS_CHNK_CALC_UTIL_HPP

#include <cassert>

namespace gkfs {
namespace util {

/**
 * Compute the base2 logarithm for 64 bit integers
 */
inline int log2(uint64_t n) {

    /* see http://stackoverflow.com/questions/11376288/fast-computing-of-log2-for-64-bit-integers */
    static const int table[64] = {
            0, 58, 1, 59, 47, 53, 2, 60, 39, 48, 27, 54, 33, 42, 3, 61,
            51, 37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4, 62,
            57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21, 56,
            45, 25, 31, 35, 16, 9, 12, 44, 24, 15, 8, 23, 7, 6, 5, 63};

    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;

    return table[(n * 0x03f6eaf2cd271461) >> 58];
}


/**
 * Align an @offset to the closest left side chunk boundary
 */
inline off64_t chnk_lalign(const off64_t offset, const size_t chnk_size) {
    return offset & ~(chnk_size - 1);
}


/**
 * Align an @offset to the closest right side chunk boundary
 */
inline off64_t chnk_ralign(const off64_t offset, const size_t chnk_size) {
    return chnk_lalign(offset + chnk_size, chnk_size);
}


/**
 * Return the padding (bytes) that separates the @offset from the closest
 * left side chunk boundary
 *
 * If @offset is a boundary the resulting padding will be 0
 */
inline size_t chnk_lpad(const off64_t offset, const size_t chnk_size) {
    return offset % chnk_size;
}


/**
 * Return the padding (bytes) that separates the @offset from the closest
 * right side chunk boundary
 *
 * If @offset is a boundary the resulting padding will be 0
 */
inline size_t chnk_rpad(const off64_t offset, const size_t chnk_size) {
    return (-offset) % chnk_size;
}


/**
 * Given an @offset calculates the chunk number to which the @offset belongs
 *
 * chunk_id(8,4) = 2;
 * chunk_id(7,4) = 1;
 * chunk_id(2,4) = 0;
 * chunk_id(0,4) = 0;
 */
inline uint64_t chnk_id_for_offset(const off64_t offset, const size_t chnk_size) {
    /*
     * This does not work for offsets that use the 64th bit, i.e., 9223372036854775808.
     * 9223372036854775808 - 1 uses 63 bits and still works. `offset / chnk_size` works with the 64th bit.
     * With this number we can address more than 19,300,000 exabytes of data though. Hi future me?
     */
    return static_cast<uint64_t>(chnk_lalign(offset, chnk_size) >> log2(chnk_size));
}


/**
 * Return the number of chunks involved in an operation that operates
 * from @offset for a certain amount of bytes (@count).
 */
inline uint64_t chnk_count_for_offset(const off64_t offset, const size_t count, const size_t chnk_size) {

    off64_t chnk_start = chnk_lalign(offset, chnk_size);
    off64_t chnk_end = chnk_lalign(offset + count - 1, chnk_size);

    return static_cast<uint64_t>((chnk_end >> log2(chnk_size)) -
                                 (chnk_start >> log2(chnk_size)) + 1);
}

} // namespace util
} // namespace gkfs

#endif
