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


def test_write(gkfs_daemon, gkfs_client):

    file = gkfs_daemon.mountdir / "file"

    ret = gkfs_client.open(file,
                           os.O_CREAT | os.O_WRONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    buf = b'42'
    ret = gkfs_client.write(file, buf, len(buf))

    assert ret.retval == len(buf) # Return the number of written bytes

def test_pwrite(gkfs_daemon, gkfs_client):

    file = gkfs_daemon.mountdir / "file"

    ret = gkfs_client.open(file,
                           os.O_CREAT | os.O_WRONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    buf = b'42'
    # write at the offset 1024
    ret = gkfs_client.pwrite(file, buf, len(buf), 1024)

    assert ret.retval == len(buf) # Return the number of written bytes

def test_writev(gkfs_daemon, gkfs_client):

    file = gkfs_daemon.mountdir / "file"

    ret = gkfs_client.open(file,
                           os.O_CREAT | os.O_WRONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    buf_0 = b'42'
    buf_1 = b'24'
    ret = gkfs_client.writev(file, buf_0, buf_1, 2)

    assert ret.retval == len(buf_0) + len(buf_1) # Return the number of written bytes

def test_pwritev(gkfs_daemon, gkfs_client):

    file = gkfs_daemon.mountdir / "file"

    ret = gkfs_client.open(file,
                           os.O_CREAT | os.O_WRONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    buf_0 = b'42'
    buf_1 = b'24'
    ret = gkfs_client.pwritev(file, buf_0, buf_1, 2, 1024)

    assert ret.retval == len(buf_0) + len(buf_1) # Return the number of written bytes
