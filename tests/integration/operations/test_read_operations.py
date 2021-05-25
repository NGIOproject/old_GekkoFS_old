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

import harness
from pathlib import Path
import errno
import stat
import os
import ctypes
import sh
import sys
import pytest
from harness.logger import logger

nonexisting = "nonexisting"


def test_read(gkfs_daemon, gkfs_client):

    file = gkfs_daemon.mountdir / "file"

    # create a file in gekkofs
    ret = gkfs_client.open(file,
                           os.O_CREAT | os.O_WRONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    # write a buffer we know
    buf = b'42'
    ret = gkfs_client.write(file, buf, len(buf))

    assert ret.retval == len(buf) # Return the number of written bytes

    # open the file to read
    ret = gkfs_client.open(file,
                           os.O_RDONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    # read the file
    ret = gkfs_client.read(file, len(buf))

    assert ret.buf == buf
    assert ret.retval == len(buf) # Return the number of read bytes

def test_pread(gkfs_daemon, gkfs_client):

    file = gkfs_daemon.mountdir / "file"

    # create a file in gekkofs
    ret = gkfs_client.open(file,
                           os.O_CREAT | os.O_WRONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    # write a buffer we know
    buf = b'42'
    ret = gkfs_client.pwrite(file, buf, len(buf), 1024)

    assert ret.retval == len(buf) # Return the number of written bytes

    # open the file to read
    ret = gkfs_client.open(file,
                           os.O_RDONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    # read the file at offset 1024
    ret = gkfs_client.pread(file, len(buf), 1024)

    assert ret.buf == buf
    assert ret.retval == len(buf) # Return the number of read bytes

def test_readv(gkfs_daemon, gkfs_client):

    file = gkfs_daemon.mountdir / "file"

    # create a file in gekkofs
    ret = gkfs_client.open(file,
                           os.O_CREAT | os.O_WRONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    # write a buffer we know
    buf_0 = b'42'
    buf_1 = b'24'
    ret = gkfs_client.writev(file, buf_0, buf_1, 2)

    assert ret.retval == len(buf_0) + len(buf_1) # Return the number of written bytes

    # open the file to read
    ret = gkfs_client.open(file,
                           os.O_RDONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    # read the file
    ret = gkfs_client.readv(file, len(buf_0), len(buf_1))

    assert ret.buf_0 == buf_0
    assert ret.buf_1 == buf_1
    assert ret.retval == len(buf_0) + len(buf_1) # Return the number of read bytes

def test_preadv(gkfs_daemon, gkfs_client):

    file = gkfs_daemon.mountdir / "file"

    # create a file in gekkofs
    ret = gkfs_client.open(file,
                           os.O_CREAT | os.O_WRONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    # write a buffer we know
    buf_0 = b'42'
    buf_1 = b'24'
    ret = gkfs_client.pwritev(file, buf_0, buf_1, 2, 1024)

    assert ret.retval == len(buf_0) + len(buf_1) # Return the number of written bytes

    # open the file to read
    ret = gkfs_client.open(file,
                           os.O_RDONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    # read the file
    ret = gkfs_client.preadv(file, len(buf_0), len(buf_1), 1024)

    assert ret.buf_0 == buf_0
    assert ret.buf_1 == buf_1
    assert ret.retval == len(buf_0) + len(buf_1) # Return the number of read bytes