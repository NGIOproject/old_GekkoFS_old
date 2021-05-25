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




#@pytest.mark.xfail(reason="invalid errno returned on success")
def test_statx(gkfs_daemon, gkfs_client):
    """Test several statx commands"""
    topdir = gkfs_daemon.mountdir / "top"
    longer = Path(topdir.parent, topdir.name + "_plus")
    dir_a  = topdir / "dir_a"
    dir_b  = topdir / "dir_b"
    file_a = topdir / "file_a"
    subdir_a  = dir_a / "subdir_a"

    # create topdir
    ret = gkfs_client.mkdir(
            topdir,
            stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 0

    # test statx on existing dir
    ret = gkfs_client.statx(0, topdir, 0, 0)

    assert ret.retval == 0
    assert stat.S_ISDIR(ret.statbuf.stx_mode)

    ret = gkfs_client.open(file_a,
                           os.O_CREAT,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval != -1


    # test statx on existing file
    ret = gkfs_client.statx(0, file_a, 0, 0)

    assert ret.retval == 0
    assert (stat.S_ISDIR(ret.statbuf.stx_mode)==0)
    assert (ret.statbuf.stx_size == 0)


    buf = b'42'
    ret = gkfs_client.write(file_a, buf, 2)

    assert ret.retval == 2

    # test statx on existing file
    ret = gkfs_client.statx(0, file_a, 0, 0)

    assert ret.retval == 0
    assert (ret.statbuf.stx_size == 2)


    return


