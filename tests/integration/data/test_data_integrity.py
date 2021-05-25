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
import string
import random
from harness.logger import logger

nonexisting = "nonexisting"
chunksize_start = 128192
chunksize_end = 2097153
step = 4096*9

def generate_random_data(size):
    return ''.join([random.choice(string.ascii_letters + string.digits) for _ in range(size)])




#@pytest.mark.xfail(reason="invalid errno returned on success")
def test_data_integrity(gkfs_daemon, gkfs_client):
    """Test several data write-read commands and check that the data is correct"""
    topdir = gkfs_daemon.mountdir / "top"
    file_a = topdir / "file_a"

    # create topdir
    ret = gkfs_client.mkdir(
            topdir,
            stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 0

    # test stat on existing dir
    ret = gkfs_client.stat(topdir)

    assert ret.retval == 0
    assert (stat.S_ISDIR(ret.statbuf.st_mode))

    ret = gkfs_client.open(file_a,
                   os.O_CREAT,
                   stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval != -1


    # test stat on existing file
    ret = gkfs_client.stat(file_a)

    assert ret.retval == 0
    assert (stat.S_ISDIR(ret.statbuf.st_mode)==0)
    assert (ret.statbuf.st_size == 0)

    # Step 1 - small sizes
    
    # Generate writes
    # Read data
    # Compare buffer

    
    for i in range (1, 512, 64):
        buf = bytes(generate_random_data(i), sys.stdout.encoding)
        
        ret = gkfs_client.write(file_a, buf, i)

        assert ret.retval == i
        ret = gkfs_client.stat(file_a)

        assert ret.retval == 0
        assert (ret.statbuf.st_size == i)

        ret = gkfs_client.read(file_a, i)
        assert ret.retval== i
        assert ret.buf == buf


    # Step 2 - Compare bigger sizes exceeding typical chunksize
    for i in range (chunksize_start, chunksize_end, step):
        ret = gkfs_client.write_validate(file_a, i)
        assert ret.retval == 1


    return


