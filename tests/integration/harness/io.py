################################################################################
#  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain           #
#  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany         #
#                                                                              #
#  This software was partially supported by the                                #
#  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).   #
#                                                                              #
#  This software was partially supported by the                                #
#  ADA-FS project under the SPPEXA project funded by the DFG.                  #
#                                                                              #
#  SPDX-License-Identifier: MIT                                                #
################################################################################

import ctypes
from marshmallow import Schema, fields, pre_load, post_load
from collections import namedtuple

class DIR_p(fields.Field):
    """Field that deserializes a ::DIR* return value"""

    def _deserialize(self, value, attr, data, **kwargs):
        return ctypes.c_void_p(value)

class Errno(fields.Field):
    """Field that deserialies an errno return value"""

    def _deserialize(self, value, attr, data, **kwargs):
        return int(value)

class ByteList(fields.Field):
    """Field that deserializes a list of bytes"""
    def _deserialize(self, value, attr, data, **kwargs):
        return bytes(value)

class StructTimespecSchema(Schema):
    """Schema that deserializes a struct timespec"""
    tv_sec = fields.Integer(required=True)
    tv_nsec = fields.Integer(required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('StructTimespec',
                ['tv_sec', 'tv_nsec'])(**data)

class StructStatSchema(Schema):
    """Schema that deserializes a struct stat"""

    st_dev = fields.Integer(required=True)
    st_ino = fields.Integer(required=True)
    st_mode = fields.Integer(required=True)
    st_nlink = fields.Integer(required=True)
    st_uid = fields.Integer(required=True)
    st_gid = fields.Integer(required=True)
    st_rdev = fields.Integer(required=True)
    st_size = fields.Integer(required=True)
    st_blksize = fields.Integer(required=True)
    st_blocks = fields.Integer(required=True)

    st_atim = fields.Nested(StructTimespecSchema)
    st_mtim = fields.Nested(StructTimespecSchema)
    st_ctim = fields.Nested(StructTimespecSchema)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('StructStat',
                ['st_dev', 'st_ino', 'st_mode', 'st_nlink', 'st_uid',
                 'st_gid', 'st_rdev', 'st_size', 'st_blksize', 'st_blocks',
                 'st_atim', 'st_mtim', 'st_ctim'])(**data)


class StructStatxTimestampSchema(Schema):
    """Schema that deserializes a struct timespec"""
    tv_sec = fields.Integer(required=True)
    tv_nsec = fields.Integer(required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('StructStatxTimestampSchema',
                ['tv_sec', 'tv_nsec'])(**data)

class StructStatxSchema(Schema):
    """Schema that deserializes a struct stat"""

    stx_mask = fields.Integer(required=True)
    stx_blksize = fields.Integer(required=True)
    stx_attributes = fields.Integer(required=True)
    stx_nlink = fields.Integer(required=True)
    stx_uid = fields.Integer(required=True)
    stx_gid = fields.Integer(required=True)
    stx_mode = fields.Integer(required=True)
    stx_ino = fields.Integer(required=True)
    stx_size = fields.Integer(required=True)
    stx_blocks = fields.Integer(required=True)
    stx_attributes_mask = fields.Integer(required=True)

    stx_atime = fields.Nested(StructStatxTimestampSchema)
    stx_btime = fields.Nested(StructStatxTimestampSchema)
    stx_ctime = fields.Nested(StructStatxTimestampSchema)
    stx_mtime = fields.Nested(StructStatxTimestampSchema)

    stx_rdev_major = fields.Integer(required=True)
    stx_rdev_minor = fields.Integer(required=True)
    stx_dev_major = fields.Integer(required=True)
    stx_dev_minor = fields.Integer(required=True)


    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('StructStatx',
                ['stx_mask', 'stx_blksize', 'stx_attributes', 'stx_nlink', 'stx_uid',
                 'stx_gid', 'stx_mode', 'stx_ino', 'stx_size', 'stx_blocks', 'stx_attributes_mask',
                 'stx_atime', 'stx_btime', 'stx_ctime', 'stx_mtime', 'stx_rdev_major', 
                 'stx_rdev_minor', 'stx_dev_major', 'stx_dev_minor'])(**data)

class DirentStruct(Schema):
    """Schema that deserializes a struct dirent"""

    d_ino = fields.Integer(required=True)
    d_off = fields.Integer(required=True)
    d_reclen = fields.Integer(required=True)
    d_type = fields.Integer(required=True)
    d_name = fields.Str(required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('DirentStruct',
                ['d_ino', 'd_off', 'd_reclen', 'd_type', 'd_name'])(**data)

class MkdirOutputSchema(Schema):
    """Schema to deserialize the results of a mkdir() execution"""

    retval = fields.Integer(required=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('MkdirReturn', ['retval', 'errno'])(**data)

class OpenOutputSchema(Schema):
    """Schema to deserialize the results of an open() execution"""
    retval = fields.Integer(required=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('OpenReturn', ['retval', 'errno'])(**data)

class OpendirOutputSchema(Schema):
    """Schema to deserialize the results of an opendir() execution"""
    dirp = DIR_p(required=True, allow_none=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('OpendirReturn', ['dirp', 'errno'])(**data)

class ReadOutputSchema(Schema):
    """Schema to deserialize the results of a read() execution"""

    buf = ByteList(allow_none=True)
    retval = fields.Integer(required=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('ReadReturn', ['buf', 'retval', 'errno'])(**data)

class PreadOutputSchema(Schema):
    """Schema to deserialize the results of a pread() execution"""

    buf = ByteList(allow_none=True)
    retval = fields.Integer(required=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('PReadReturn', ['buf', 'retval', 'errno'])(**data)

class ReadvOutputSchema(Schema):
    """Schema to deserialize the results of a read() execution"""

    buf_0 = ByteList(allow_none=True)
    buf_1 = ByteList(allow_none=True)
    retval = fields.Integer(required=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('ReadvReturn', ['buf_0', 'buf_1', 'retval', 'errno'])(**data)

class PreadvOutputSchema(Schema):
    """Schema to deserialize the results of a read() execution"""

    buf_0 = ByteList(allow_none=True)
    buf_1 = ByteList(allow_none=True)
    retval = fields.Integer(required=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('PReadvReturn', ['buf_0', 'buf_1', 'retval', 'errno'])(**data)

class ReaddirOutputSchema(Schema):
    """Schema to deserialize the results of a readdir() execution"""

    dirents = fields.List(fields.Nested(DirentStruct), allow_none=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('ReaddirReturn', ['dirents', 'errno'])(**data)

class RmdirOutputSchema(Schema):
    """Schema to deserialize the results of an opendir() execution"""

    retval = fields.Integer(required=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('RmdirReturn', ['retval', 'errno'])(**data)

class WriteOutputSchema(Schema):
    """Schema to deserialize the results of a write() execution"""

    retval = fields.Integer(required=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('WriteReturn', ['retval', 'errno'])(**data)

class PwriteOutputSchema(Schema):
    """Schema to deserialize the results of a pwrite() execution"""

    retval = fields.Integer(required=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('PWriteReturn', ['retval', 'errno'])(**data)

class WritevOutputSchema(Schema):
    """Schema to deserialize the results of a writev() execution"""

    retval = fields.Integer(required=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('WritevReturn', ['retval', 'errno'])(**data)

class PwritevOutputSchema(Schema):
    """Schema to deserialize the results of a writev() execution"""

    retval = fields.Integer(required=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('PWritevReturn', ['retval', 'errno'])(**data)

class StatOutputSchema(Schema):
    """Schema to deserialize the results of a stat() execution"""

    retval = fields.Integer(required=True)
    statbuf = fields.Nested(StructStatSchema, required=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('StatReturn', ['retval', 'statbuf', 'errno'])(**data)


class StatxOutputSchema(Schema):
    """Schema to deserialize the results of a statx() execution"""

    retval = fields.Integer(required=True)
    statbuf = fields.Nested(StructStatxSchema, required=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('StatxReturn', ['retval', 'statbuf', 'errno'])(**data)


class LseekOutputSchema(Schema):
    """Schema to deserialize the results of an lseek() execution"""
    retval = fields.Integer(required=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('LseekReturn', ['retval', 'errno'])(**data)


class WriteValidateOutputSchema(Schema):
    """Schema to deserialize the results of a write() execution"""

    retval = fields.Integer(required=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('WriteValidateReturn', ['retval', 'errno'])(**data)


class WriteRandomOutputSchema(Schema):
    """Schema to deserialize the results of a write() execution"""

    retval = fields.Integer(required=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('WriteRandomReturn', ['retval', 'errno'])(**data)


class TruncateOutputSchema(Schema):
    """Schema to deserialize the results of an truncate() execution"""
    retval = fields.Integer(required=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('TruncateReturn', ['retval', 'errno'])(**data)


# UTIL
class FileCompareOutputSchema(Schema):
    """Schema to deserialize the results of comparing two files execution"""
    retval = fields.Integer(required=True)
    errno = Errno(data_key='errnum', required=True)

    @post_load
    def make_object(self, data, **kwargs):
        return namedtuple('FileCompareReturn', ['retval', 'errno'])(**data)

class IOParser:

    OutputSchemas = {
        'mkdir'   : MkdirOutputSchema(),
        'open'    : OpenOutputSchema(),
        'opendir' : OpendirOutputSchema(),
        'read'    : ReadOutputSchema(),
        'pread'   : PreadOutputSchema(),
        'readv'   : ReadvOutputSchema(),
        'preadv'  : PreadvOutputSchema(),
        'readdir' : ReaddirOutputSchema(),
        'rmdir'   : RmdirOutputSchema(),
        'write'   : WriteOutputSchema(),
        'pwrite'  : PwriteOutputSchema(),
        'writev'  : WritevOutputSchema(),
        'pwritev' : PwritevOutputSchema(),
        'stat'    : StatOutputSchema(),
        'statx'   : StatxOutputSchema(),
        'lseek'   : LseekOutputSchema(),
        'write_random': WriteRandomOutputSchema(),
        'write_validate' : WriteValidateOutputSchema(),
        'truncate': TruncateOutputSchema(),
        # UTIL
        'file_compare': FileCompareOutputSchema(),
    }

    def parse(self, command, output):
        if command in self.OutputSchemas:
            return self.OutputSchemas[command].loads(output)
        else:
            raise ValueError(f"Unknown I/O command {command}")
