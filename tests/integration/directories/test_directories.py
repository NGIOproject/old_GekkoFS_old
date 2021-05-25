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
def test_mkdir(gkfs_daemon, gkfs_client):
    """Create a new directory in the FS's root"""

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

    # test stat on existing dir
    ret = gkfs_client.stat(topdir)

    assert ret.retval == 0
    assert stat.S_ISDIR(ret.statbuf.st_mode)

    # open topdir
    ret = gkfs_client.open(topdir, os.O_DIRECTORY)
    assert ret.retval != -1


    # read and write should be impossible on directories
    ret = gkfs_client.read(topdir, 1)

    assert ret.buf is None
    assert ret.retval == -1
    assert ret.errno == errno.EISDIR

    # buf = bytes('42', sys.stdout.encoding)
    # print(buf.hex())
    buf = b'42'
    ret = gkfs_client.write(topdir, buf, 1)

    assert ret.retval == -1
    assert ret.errno == errno.EISDIR


    # read top directory that is empty
    ret = gkfs_client.opendir(topdir)

    assert ret.dirp is not None

    ret = gkfs_client.readdir(topdir)

    # XXX: This might change in the future if we add '.' and '..'
    assert len(ret.dirents) == 0

    # close directory
    # TODO: disabled for now because we have no way to keep DIR* alive
    # between gkfs.io executions
    # ret = gkfs_client.opendir(XXX)


    # populate top directory
    for d in [dir_a, dir_b]:
        ret = gkfs_client.mkdir(
                d,
                stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

        assert ret.retval == 0

    ret = gkfs_client.open(file_a,
                           os.O_CREAT,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval != -1

    ret = gkfs_client.readdir(gkfs_daemon.mountdir)

    # XXX: This might change in the future if we add '.' and '..'
    assert len(ret.dirents) == 1
    assert ret.dirents[0].d_name == 'top'
    assert ret.dirents[0].d_type == 4 # DT_DIR

    expected = [
        ( dir_a.name,  4 ), # DT_DIR
        ( dir_b.name,  4 ),
        ( file_a.name, 8 ) # DT_REG
    ]

    ret = gkfs_client.readdir(topdir)
    assert len(ret.dirents) == len(expected)

    for d,e in zip(ret.dirents, expected):
        assert d.d_name == e[0]
        assert d.d_type == e[1]

    # remove file using rmdir should produce an error
    ret = gkfs_client.rmdir(file_a)
    assert ret.retval == -1
    assert ret.errno == errno.ENOTDIR

    # create a directory with the same prefix as topdir but longer name
    ret = gkfs_client.mkdir(
            longer,
            stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 0

    expected = [
        ( topdir.name,  4 ), # DT_DIR
        ( longer.name,  4 ), # DT_DIR
    ]

    ret = gkfs_client.readdir(gkfs_daemon.mountdir)
    assert len(ret.dirents) == len(expected)

    for d,e in zip(ret.dirents, expected):
        assert d.d_name == e[0]
        assert d.d_type == e[1]

    # create 2nd level subdir and check it's not included in readdir()
    ret = gkfs_client.mkdir(
            subdir_a,
            stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 0

    expected = [
        ( topdir.name,  4 ), # DT_DIR
        ( longer.name,  4 ), # DT_DIR
    ]

    ret = gkfs_client.readdir(gkfs_daemon.mountdir)
    assert len(ret.dirents) == len(expected)

    for d,e in zip(ret.dirents, expected):
        assert d.d_name == e[0]
        assert d.d_type == e[1]

    expected = [
        ( subdir_a.name,  4 ), # DT_DIR
    ]

    ret = gkfs_client.readdir(dir_a)

    assert len(ret.dirents) == len(expected)

    for d,e in zip(ret.dirents, expected):
        assert d.d_name == e[0]
        assert d.d_type == e[1]


    return

@pytest.mark.skip(reason="invalid errno returned on success")
@pytest.mark.parametrize("directory_path",
    [ nonexisting ])
def test_opendir(gkfs_daemon, gkfs_client, directory_path):

    ret = gkfs_client.opendir(gkfs_daemon.mountdir / directory_path)

    assert ret.dirp is None
    assert ret.errno == errno.ENOENT

# def test_stat(gkfs_daemon):
#     pass
#
# def test_rmdir(gkfs_daemon):
#     pass
#
# def test_closedir(gkfs_daemon):
#     pass
