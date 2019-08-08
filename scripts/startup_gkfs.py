#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import argparse
import time

import os

from util import util

__author__ = "Marc-Andre Vef"
__email__ = "vef@uni-mainz.de"

global PRETEND
global PSSH_PATH
global WAITTIME
global PSSH_HOSTFILE_PATH


def check_dependencies():
    global PSSH_PATH
    """Check if pssh is installed"""
    pssh_path = os.popen('which pssh').read().strip()
    if pssh_path != '':
        PSSH_PATH = pssh_path
        return
    pssh_path = os.popen('which parallel-ssh').read().strip()
    if pssh_path != '':
        PSSH_PATH = pssh_path
        return
    print('[ERR] parallel-ssh/pssh executable cannot be found. Please add it to the parameter list')
    exit(1)


def init_system(daemon_path, rootdir, metadir, mountdir, nodelist, cleanroot, numactl):
    """Initializes GekkoFS on specified nodes.

    Args:
        daemon_path (str): Path to daemon executable
        rootdir (str): Path to root directory for fs data
        metadir (str): Path to metadata directory where metadata is stored
        mountdir (str): Path to mount directory where is used in
        nodelist (str): Comma-separated list of nodes where daemons need to be launched
        cleanroot (bool): if True, root and metadir is cleaned before daemon init
        numactl (str): numactl arguments for daemon init
    """
    global PSSH_PATH
    global PRETEND
    global PSSH_HOSTFILE_PATH
    # get absolute paths
    daemon_path = os.path.realpath(os.path.expanduser(daemon_path))
    mountdir = os.path.realpath(os.path.expanduser(mountdir))
    rootdir = os.path.realpath(os.path.expanduser(rootdir))
    # Replace metadir with rootdir if only rootdir is given
    if len(metadir) == 0:
        metadir = rootdir
    else:
        metadir = os.path.realpath(os.path.expanduser(metadir))
    pssh_nodelist = ''
    nodefile = False
    if os.path.exists(nodelist):
        nodefile = True
        if not util.create_pssh_hostfile(nodelist, PSSH_HOSTFILE_PATH):
            exit(1)
    if PSSH_PATH is '':
        check_dependencies()
    # set pssh arguments
    if nodefile:
        pssh = '%s -O StrictHostKeyChecking=no -i -h "%s"' % (PSSH_PATH, PSSH_HOSTFILE_PATH)
    else:
        pssh = '%s -O StrictHostKeyChecking=no -i -H "%s"' % (PSSH_PATH, nodelist.replace(',', ' '))

    # clean root and metadata dir if needed
    if cleanroot:
        cmd_rm_str = '%s "rm -rf %s/* %s/* && truncate -s 0 /tmp/gkfs_daemon.log /tmp/gkfs_preload.log"' % (pssh, rootdir, metadir)
        if PRETEND:
            print('Pretending: {}'.format(cmd_rm_str))
        else:
            print('Running: {}'.format(cmd_rm_str))
            pssh_ret = util.exec_shell(cmd_rm_str, True)
            err = False
            for line in pssh_ret:
                if 'FAILURE' in line.strip()[:30]:
                    err = True
                    print('------------------------- ERROR pssh -- Host "{}" -------------------------'.format(\
                          line[line.find('FAILURE'):].strip().split(' ')[1]))
                    print(line)
            if not err:
                print('pssh daemon launch successfully executed. Root and Metadata dir are cleaned.\n')
            else:
                print('[ERR] with pssh. Aborting!')
                exit(1)

    # Start deamons
    if nodefile:
        if len(numactl) == 0:
            cmd_str = '%s "nohup %s -r %s -i %s -m %s --hostfile %s > /tmp/gkfs_daemon.log 2>&1 &"' \
                      % (pssh, daemon_path, rootdir, metadir, mountdir, nodelist)
        else:
            cmd_str = '%s "nohup numactl %s %s -r %s -i %s -m %s --hostfile %s > /tmp/gkfs_daemon.log 2>&1 &"' \
                      % (pssh, numactl, daemon_path, rootdir, metadir, mountdir, nodelist)

    else:
        if len(numactl) == 0:
            cmd_str = '%s "nohup %s -r %s -i %s -m %s --hosts %s > /tmp/gkfs_daemon.log 2>&1 &"' \
                      % (pssh, daemon_path, rootdir, metadir, mountdir, nodelist)
        else:
            cmd_str = '%s "nohup numactl %s %s -r %s -i %s -m %s --hosts %s > /tmp/gkfs_daemon.log 2>&1 &"' \
                      % (pssh, numactl, daemon_path, rootdir, metadir, mountdir, nodelist)

    if PRETEND:
        print('Pretending: {}'.format(cmd_str))
    else:
        print('Running: {}'.format(cmd_str))
        pssh_ret = util.exec_shell(cmd_str, True)
        err = False
        for line in pssh_ret:
            if 'FAILURE' in line.strip()[:30]:
                err = True
                print('------------------------- ERROR pssh -- Host "{}" -------------------------'.format(\
                      line[line.find('FAILURE'):].strip().split(' ')[1]))
                print(line)
        if not err:
            print('pssh daemon launch successfully executed. Checking for FS startup errors ...\n')
        else:
            print('[ERR] with pssh. Aborting. Please run shutdown_gkfs.py to shut down orphan daemons!')
            exit(1)

    if not PRETEND:
        print('Give it some time ({} second) to startup ...'.format(WAITTIME))
        for i in range(WAITTIME):
            print('{}\r'.format(WAITTIME - i)),
            time.sleep(1)

    # Check logs for errors
    cmd_chk_str = '%s "head -5 /tmp/gkfs_daemon.log"' % pssh
    if PRETEND:
        print('Pretending: {}'.format(cmd_chk_str))
    else:
        print('Running: {}'.format(cmd_chk_str))
        pssh_ret = util.exec_shell(cmd_chk_str, True)
        err = False
        fs_err = False
        for line in pssh_ret:
            if 'Failure' in line.strip()[:30]:
                err = True
                print('------------------------- ERROR pssh -- Host "{}" -------------------------'.format(\
                      line[line.find('FAILURE'):].strip().split(' ')[1]))
                print(line)
            else:
                # check for errors in log
                if '[E]' in line[line.strip().find('\n') + 1:] or 'Assertion `err\'' in line[
                                                                                          line.strip().find('\n') + 1:]:
                    fs_err = True
                    print('------------------------- ERROR pssh -- Host "{}" -------------------------'.format(\
                          line.strip().split(' ')[3].split('\n')[0]))
                    print('{}'.format(line[line.find('\n') + 1:]))

        if not err and not fs_err:
            print('pssh logging check successfully executed. Looks prime.')
        else:
            print('[ERR] while checking fs logs. Aborting. Please run shutdown_gkfs.py to shut down orphan daemons!')
            exit(1)


if __name__ == "__main__":
    # Init parser
    parser = argparse.ArgumentParser(description='This script launches GekkoFS on multiple nodes',
                                     formatter_class=argparse.RawTextHelpFormatter)
    # positional arguments
    parser.add_argument('daemonpath', type=str,
                        help='path to the daemon executable')
    parser.add_argument('rootdir', type=str,
                        help='path to the root directory where all data will be stored')
    parser.add_argument('mountdir', type=str,
                        help='path to the mount directory of the file system')
    parser.add_argument('nodelist', type=str,
                        help='''list of nodes where the file system is launched. This can be a comma-separated list
or a path to a nodefile (one node per line)''')

    # optional arguments
    parser.add_argument('-i', '--metadir', metavar='<METADIR_PATH>', type=str, default='',
                        help='''Path to separate metadir directory where metadata is stored. 
If not set, rootdir will be used instead.''')
    parser.add_argument('-p', '--pretend', action='store_true',
                        help='Output launch command and do not actually execute it')
    parser.add_argument('-P', '--pssh', metavar='<PSSH_PATH>', type=str, default='',
                        help='Path to parallel-ssh/pssh. Defaults to /usr/bin/{parallel-ssh,pssh}')
    parser.add_argument('-J', '--jobid', metavar='<JOBID>', type=str, default='',
                        help='Jobid for cluster batch system. Used for a unique hostfile used for pssh.')
    parser.add_argument('-c', '--cleanroot', action='store_true',
                        help='Removes contents of root and metadata directory before starting daemon. Be careful!')
    parser.add_argument('-n', '--numactl', metavar='<numactl_args>', type=str, default='',
                        help='If daemon should be pinned to certain cores, set numactl arguments here.')
    parser.add_argument('-H', '--pssh_hostfile', metavar='<pssh_hostfile>', type=str, default='/tmp/hostfile_pssh',
                        help='''This script creates a hostfile to pass to MPI. This variable defines the path. 
Defaults to /tmp/hostfile_pssh''')
    args = parser.parse_args()
    if args.pretend:
        PRETEND = True
    else:
        PRETEND = False
    if args.jobid == '':
        PSSH_HOSTFILE_PATH = args.pssh_hostfile
    else:
        PSSH_HOSTFILE_PATH = '%s_%s' % (args.pssh_hostfile, args.jobid)
    PSSH_PATH = args.pssh
    WAITTIME = 5
    init_system(args.daemonpath, args.rootdir, args.metadir, args.mountdir, args.nodelist, args.cleanroot, args.numactl)

    print('\nNothing left to do; exiting. :)')
