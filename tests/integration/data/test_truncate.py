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

# import harness
# from pathlib import Path
import os
import stat


# @pytest.mark.xfail(reason="invalid errno returned on success")
def test_truncate(gkfs_daemon, gkfs_client):
    """Testing truncate:
    1. create a large file over multiple chunks
    2. truncate it in the middle and compare it with a fresh file with equal contents (exactly at chunk border)
    3. truncate it again so that in truncates in the middle of the chunk and compare with fresh file
    TODO chunksize needs to be respected to make sure chunk border and in the middle of chunk truncates are honored
    """
    truncfile = gkfs_daemon.mountdir / "trunc_file"

    # open and create test file
    ret = gkfs_client.open(truncfile, os.O_CREAT | os.O_WRONLY, stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval != -1

    # write a multi MB file (16mb)
    buf_length = 16777216
    ret = gkfs_client.write_random(truncfile, buf_length)

    assert ret.retval == buf_length

    ret = gkfs_client.stat(truncfile)

    assert ret.statbuf.st_size == buf_length

    # truncate it
    # split exactly in the middle
    trunc_size = buf_length // 2
    ret = gkfs_client.truncate(truncfile, trunc_size)

    assert ret.retval == 0
    # check file length
    ret = gkfs_client.stat(truncfile)

    assert ret.statbuf.st_size == trunc_size

    # verify contents by writing a new file (random content is seeded) and checksum both
    truncfile_verify = gkfs_daemon.mountdir / "trunc_file_verify"

    # open and create test file
    ret = gkfs_client.open(truncfile_verify, os.O_CREAT | os.O_WRONLY, stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval != -1

    # write trunc_size of data to new file
    ret = gkfs_client.write_random(truncfile_verify, trunc_size)

    assert ret.retval == trunc_size

    ret = gkfs_client.stat(truncfile_verify)

    assert ret.statbuf.st_size == trunc_size

    ret = gkfs_client.file_compare(truncfile, truncfile_verify, trunc_size)

    assert ret.retval == 0

    # trunc at byte 712345 (middle of chunk)
    # TODO feed chunksize into test to make sure it is always in the middle of the chunk
    trunc_size = 712345
    ret = gkfs_client.truncate(truncfile, trunc_size)

    assert ret.retval == 0

    # check file length
    ret = gkfs_client.stat(truncfile)

    assert ret.statbuf.st_size == trunc_size

    # verify contents by writing a new file (random content is seeded) and checksum both
    truncfile_verify_2 = gkfs_daemon.mountdir / "trunc_file_verify_2"

    # open and create test file
    ret = gkfs_client.open(truncfile_verify_2, os.O_CREAT | os.O_WRONLY, stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval != -1

    # write trunc_size of data to new file
    ret = gkfs_client.write_random(truncfile_verify_2, trunc_size)

    assert ret.retval == trunc_size

    ret = gkfs_client.stat(truncfile_verify_2)

    assert ret.statbuf.st_size == trunc_size

    ret = gkfs_client.file_compare(truncfile, truncfile_verify_2, trunc_size)

    assert ret.retval == 0
