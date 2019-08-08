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


def shutdown_system(daemon_pid_path, nodelist, sigkill):
    """Shuts down GekkoFS on specified nodes.

    Args:
        daemon_pid_path (str): Path to daemon pid file
        nodelist (str): Comma-separated list of nodes where daemons need to be launched
        sigkill (bool): If true force kills daemons
    """
    global PSSH_PATH
    global PRETEND
    global WAITTIME
    global PSSH_HOSTFILE_PATH
    # get absolute paths
    daemon_pid_path = os.path.realpath(os.path.expanduser(daemon_pid_path))
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
    if sigkill:
        cmd_str = '%s "pkill -SIGKILL --pidfile \"%s\""' % (pssh, daemon_pid_path)
    else:
        cmd_str = '%s "pkill -SIGTERM --pidfile \"%s\""' % (pssh, daemon_pid_path)
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
            if sigkill:
                print('pssh daemon launch successfully executed. FS daemons have been force killed ...')
                exit(1)
            else:
                print('pssh daemon launch successfully executed. Checking for FS shutdown errors ...\n')
        else:
            print('[ERR] with pssh. Aborting...')
            exit(1)

    if not PRETEND:
        print('Give it some time ({} second) to finish up ...'.format(WAITTIME))
        for i in range(WAITTIME):
            print('{}\r'.format(WAITTIME - i))
            time.sleep(1)
    print('Checking logs ...\n')

    cmd_chk_str = '%s "tail -4 /tmp/gkfs_daemon.log"' % pssh
    if PRETEND:
        print('Pretending: {}'.format(cmd_chk_str))
    else:
        print('Running: {}'.format(cmd_chk_str))
        pssh_ret = util.exec_shell(cmd_chk_str, True)
        err = False
        fs_err = False
        for line in pssh_ret:
            if line == '':
                continue
            if 'Failure' in line.strip()[:30]:
                err = True
                print('------------------------- ERROR pssh -- Host "{}" -------------------------'.format(\
                      line[line.find('FAILURE'):].strip().split(' ')[1]))
                print(line)
            else:
                # check for errors in log
                if not 'All services shut down.' in line[line.strip().find('\n') + 1:]:
                    fs_err = True
                    print('------------------------- WARN pssh -- Host "{}" -------------------------'.format(\
                          line.strip().split(' ')[3].split('\n')[0]))
                    print('{}'.format(line[line.find('\n') + 1:]))

        if not err and not fs_err:
            print('pssh logging check successfully executed. Looks prime.')
        else:
            print('[WARN] while checking fs logs. Something might went wrong when shutting down')
            exit(1)


if __name__ == "__main__":
    # Init parser
    parser = argparse.ArgumentParser(description='This script stops GekkoFS on multiple nodes',
                                     formatter_class=argparse.RawTextHelpFormatter)
    # positional arguments
    parser.add_argument('daemonpidpath', type=str,
                        help='path to the daemon pid file')
    parser.add_argument('nodelist', type=str,
                        help='''list of nodes where the file system is launched. This can be a comma-separated list
                             or a path to a nodefile (one node per line)''')

    # optional arguments
    parser.add_argument('-p', '--pretend', action='store_true',
                        help='Output launch command and do not actually execute it')
    parser.add_argument('-9', '--sigkill', action='store_true',
                        help='Force kill daemons')
    parser.add_argument('-P', '--pssh', metavar='<PSSH_PATH>', type=str, default='',
                        help='Path to parallel-ssh/pssh. Defaults to /usr/bin/{parallel-ssh,pssh}')
    parser.add_argument('-J', '--jobid', metavar='<JOBID>', type=str, default='',
                        help='Jobid for cluster batch system. Used for a unique hostfile used for pssh.')
    parser.add_argument('-H', '--pssh_hostfile', metavar='<pssh_hostfile>', type=str, default='/tmp/hostfile_pssh',
                        help='''This script creates a hostfile to pass to MPI. This variable defines the path. 
Defaults to /tmp/hostfile_pssh''')
    args = parser.parse_args()

    if args.pretend is True:
        PRETEND = True
    else:
        PRETEND = False
    if args.jobid == '':
        PSSH_HOSTFILE_PATH = args.pssh_hostfile
    else:
        PSSH_HOSTFILE_PATH = '%s_%s' % (args.pssh_hostfile, args.jobid)
    PSSH_PATH = args.pssh
    WAITTIME = 5
    shutdown_system(args.daemonpidpath, args.nodelist, args.sigkill)

    print('\nNothing left to do; exiting. :)')
