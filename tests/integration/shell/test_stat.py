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

import pytest
from harness.logger import logger

file01 = 'file01'

@pytest.mark.skip(reason="shell tests seem to hang clients at times")
def test_shell_if_e(gkfs_daemon, gkfs_shell, file_factory):
    """
    Copy a file into gkfs using the shell and check that it
    exists using `if [[ -e <file> ]]`.
    """

    logger.info("creating input file")
    lf01 = file_factory.create(file01, size=4.0, unit='MB')

    logger.info("copying into gkfs")
    cmd = gkfs_shell.cp(lf01.pathname, gkfs_daemon.mountdir)
    assert cmd.exit_code == 0

    logger.info("checking if file exists")
    cmd = gkfs_shell.script(
        f"""
            expected_pathname={gkfs_daemon.mountdir / file01}
            if [[ -e ${{expected_pathname}} ]];
            then
                exit 0
            fi
            exit 1
        """)

    assert cmd.exit_code == 0

@pytest.mark.skip(reason="shell tests seem to hang clients at times")
def test_stat_script(gkfs_daemon, gkfs_shell, file_factory):
    """
    Copy a file into gkfs using the shell and check that it
    exists using stat <file> in a script.
    """

    logger.info("creating input file")
    lf01 = file_factory.create(file01, size=4.0, unit='MB')

    logger.info("copying into gkfs")
    cmd = gkfs_shell.cp(lf01.pathname, gkfs_daemon.mountdir)
    assert cmd.exit_code == 0

    logger.info("checking metadata")
    cmd = gkfs_shell.script(
        f"""
            expected_pathname={gkfs_daemon.mountdir / file01}
            {gkfs_shell.patched_environ} stat ${{expected_pathname}}
            exit $?
        """,
        intercept_shell=False)

    assert cmd.exit_code == 0

@pytest.mark.skip(reason="shell tests seem to hang clients at times")
def test_stat_command(gkfs_daemon, gkfs_shell, file_factory):
    """
    Copy a file into gkfs using the shell and check that it
    exists using stat <file> as a command.
    """

    logger.info("creating input file")
    lf01 = file_factory.create(file01, size=4.0, unit='MB')

    logger.info("copying into gkfs")
    cmd = gkfs_shell.cp(lf01.pathname, gkfs_daemon.mountdir)
    assert cmd.exit_code == 0

    logger.info("checking metadata")
    cmd = gkfs_shell.stat('--terse', gkfs_daemon.mountdir / file01)
    assert cmd.exit_code == 0
    assert cmd.parsed_stdout.size == lf01.size
