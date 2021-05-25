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

import os, re, hashlib
from harness.logger import logger

class Workspace:
    """
    Test workspace, implements a self-contained subdirectory where a test
    can run and generate artifacts in a self-contained manner.
    """

    def __init__(self, twd, bindirs, libdirs):
        """
        Initializes the test workspace by creating the following directories
        under `twd`:

            ./twd/
            ├──logs/
            ├──mnt/
            ├──root/
            └──tmp/

        Parameters
        ----------
        twd: `pathlib.Path`
            Path to the test working directory. Must exist.
        bindirs: `list(pathlib.Path)`
            List of paths where programs required for the test should
            be searched for.
        libdirs: `list(pathlib.Path)`
            List of paths where libraries required for the test should be
            searched for.
        """

        self._twd = twd
        self._bindirs = bindirs
        self._libdirs = libdirs
        self._logdir = self._twd / 'logs'
        self._rootdir = self._twd / 'root'
        self._metadir = self._twd / 'meta'
        self._mountdir = self._twd / 'mnt'
        self._tmpdir = self._twd / 'tmp'

        self._logdir.mkdir()
        self._rootdir.mkdir()
        self._mountdir.mkdir()
        self._tmpdir.mkdir()

    @property
    def twd(self):
        return self._twd

    @property
    def bindirs(self):
        return self._bindirs

    @property
    def libdirs(self):
        return self._libdirs

    @property
    def logdir(self):
        return self._logdir

    @property
    def rootdir(self):
        return self._rootdir

    @property
    def metadir(self):
        return self._metadir

    @property
    def mountdir(self):
        return self._mountdir

    @property
    def tmpdir(self):
        return self._tmpdir

class File:

    def __init__(self, pathname, size):
        self._pathname = pathname
        self._size = size

    @property
    def pathname(self):
        return self._pathname

    @property
    def size(self):
        return self._size


    def md5sum(self, blocksize=65536):
        hash = hashlib.md5()
        with open(self.pathname, "rb") as f:
            for block in iter(lambda: f.read(blocksize), b""):
                hash.update(block)
        return hash.hexdigest()

class FileCreator:
    """
    Factory that allows tests to create files in a workspace.
    """

    def __init__(self, workspace):
        self._workspace = workspace

    def create(self, pathname, size, unit='c'):
        """
        Create a random binary file in the tests workspace's temporary
        directory.


        Parameters
        ----------
        pathname: `str`
            The file pathname under the workspace's tmpdir.
        size: `int` or `float`
            The desired size for the created file, expressed as a number of
            size units.
        unit: `str`
            The prefix of size unit to be used to compute the resulting
            file size. May be one of the following multiplicative units:
            c=1, w=2, b=512, kB=1000, K=1024, MB=1000*1000, M=1024*1024,
            GB=1000*1000*1000, G=1024*1024*1024.

            If left unspecified, it defaults to 'c'=1.

        Returns
        -------
        The full `pathlib.Path` pathname of the created file.
        """

        # allow some aliases for convenience
        suffixes = {
            'c'   : 1.0,
            'w'   : 2.0,
            'b'   : 512.0,
            'kB'  : 1000.0,
            'K'   : 1024.0,
            'MB'  : 1000*1000.0,
            'M'   : 1024*1024.0,
            'GB'  : 1000*1000*1000.0,
            'G'   : 1024*1024*1024.0
        }

        full_pathname = self._workspace.tmpdir / pathname
        total_size = int(float(size)*suffixes.get(unit, 1.0))

        with open(full_pathname, "wb") as out:
            out.write(os.urandom(total_size))

        return File(full_pathname, total_size)
