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
import time
import ctypes
import socket
import sh
import sys
import pytest
from harness.logger import logger

nonexisting = "nonexisting"


def test_two_io_nodes(gkfwd_daemon_factory, gkfwd_client_factory):
    """Write files from two clients using two daemons"""

    d00 = gkfwd_daemon_factory.create()
    d01 = gkfwd_daemon_factory.create()

    c00 = gkfwd_client_factory.create('c-0')
    c01 = gkfwd_client_factory.create('c-1')

    file = d00.mountdir / "file-c00"

    # create a file in gekkofs
    ret = c00.open(file,
                           os.O_CREAT | os.O_WRONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    # write a buffer we know
    buf = b'42'
    ret = c00.write(file, buf, len(buf))

    assert ret.retval == len(buf) # Return the number of written bytes

    # open the file to read
    ret = c00.open(file,
                           os.O_RDONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    # read the file
    ret = c00.read(file, len(buf))

    assert ret.buf == buf
    assert ret.retval == len(buf) # Return the number of read bytes


    file = d01.mountdir / "file-c01"

    # create a file in gekkofs
    ret = c01.open(file,
                           os.O_CREAT | os.O_WRONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    # write a buffer we know
    buf = b'42'
    ret = c01.write(file, buf, len(buf))

    assert ret.retval == len(buf) # Return the number of written bytes

    # open the file to read
    ret = c01.open(file,
                           os.O_RDONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    # read the file
    ret = c01.read(file, len(buf))

    assert ret.buf == buf
    assert ret.retval == len(buf) # Return the number of read bytes

    # both files should be there and accessible by the two clients
    ret = c00.readdir(d00.mountdir)

    assert len(ret.dirents) == 2

    assert ret.dirents[0].d_name == 'file-c00'
    assert ret.dirents[0].d_type == 8 # DT_REG

    assert ret.dirents[1].d_name == 'file-c01'
    assert ret.dirents[1].d_type == 8 # DT_REG

    with open(c00.log) as f:
        lines = f.readlines()

        for line in lines:
            if 'Forward to' in line:
                ion = line.split()[-1]

                assert ion == '0'

    with open(c01.log) as f:
        lines = f.readlines()

        for line in lines:
            if 'Forward to' in line:
                ion = line.split()[-1]

                assert ion == '1'

def test_two_io_nodes_remap(gkfwd_daemon_factory, gkfwd_client_factory):
    """Write files from two clients using two daemons"""

    d00 = gkfwd_daemon_factory.create()
    d01 = gkfwd_daemon_factory.create()

    c00 = gkfwd_client_factory.create('c-0')
    c01 = gkfwd_client_factory.create('c-1')

    file = d00.mountdir / "file-c00-1"

    # create a file in gekkofs
    ret = c00.open(file,
                           os.O_CREAT | os.O_WRONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    # write a buffer we know
    buf = b'42'
    ret = c00.write(file, buf, len(buf))

    assert ret.retval == len(buf) # Return the number of written bytes

    with open(c00.log) as f:
        lines = f.readlines()

        for line in lines:
            if 'Forward to' in line:
                ion = line.split()[-1]

                assert ion == '0'

    # recreate the mapping so that the server that wrote will now read
    c00.remap('c-1')

    # we need to wait for at least the number of seconds between remap calls
    time.sleep(10)

    file = d00.mountdir / "file-c00-2"

    # open the file to write
    ret = c00.open(file,
                           os.O_CREAT | os.O_WRONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    # read the file
    buf = b'24'
    ret = c00.write(file, buf, len(buf))

    assert ret.retval == len(buf) # Return the number of read bytes

    with open(c00.log) as f:
        lines = f.readlines()

        for line in lines:
            if 'Forward to' in line:
                ion = line.split()[-1]

                assert ion == '1'

def test_two_io_nodes_operations(gkfwd_daemon_factory, gkfwd_client_factory):
    """Write files from one client and read in the other using two daemons"""

    d00 = gkfwd_daemon_factory.create()
    d01 = gkfwd_daemon_factory.create()

    c00 = gkfwd_client_factory.create('c-0')
    c01 = gkfwd_client_factory.create('c-1')

    file = d00.mountdir / "file-c00"

    # create a file in gekkofs
    ret = c00.open(file,
                           os.O_CREAT | os.O_WRONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    # write a buffer we know
    buf = b'42'
    ret = c00.write(file, buf, len(buf))

    assert ret.retval == len(buf) # Return the number of written bytes

    # open the file to read
    ret = c00.open(file,
                           os.O_RDONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    # read the file
    ret = c00.read(file, len(buf))

    assert ret.buf == buf
    assert ret.retval == len(buf) # Return the number of read bytes

    # open the file to read
    ret = c01.open(file,
                           os.O_RDONLY,
                           stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

    assert ret.retval == 10000

    # read the file
    ret = c01.read(file, len(buf))

    assert ret.buf == buf
    assert ret.retval == len(buf) # Return the number of read bytes

    # the file should be there and accessible by the two clients
    ret = c00.readdir(d00.mountdir)

    assert len(ret.dirents) == 1

    assert ret.dirents[0].d_name == 'file-c00'
    assert ret.dirents[0].d_type == 8 # DT_REG

    # the file should be there and accessible by the two clients
    ret = c01.readdir(d01.mountdir)

    assert len(ret.dirents) == 1

    assert ret.dirents[0].d_name == 'file-c00'
    assert ret.dirents[0].d_type == 8 # DT_REG

    with open(c00.log) as f:
        lines = f.readlines()

        for line in lines:
            if 'Forward to' in line:
                ion = line.split()[-1]

                assert ion == '0'

    with open(c01.log) as f:
        lines = f.readlines()

        for line in lines:
            if 'Forward to' in line:
                ion = line.split()[-1]

                assert ion == '1'