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

import sys, pytest, pluggy, py, platform
from collections import namedtuple
from pathlib import Path
from pprint import pformat

report_width = 80

def _add_sep(sepchar, msg, sepcolor=None, msgcolor=None, fullwidth = report_width):
    # we want 2 + 2*len(fill) + len(msg) <= fullwidth
    # i.e.    2 + 2*len(sepchar)*N + len(msg) <= fullwidth
    #         2*len(sepchar)*N <= fullwidth - len(msg) - 2
    #         N <= (fullwidth - len(msg) - 2) // (2*len(sepchar))
    N = max((fullwidth - len(msg) - 2) // (2*len(sepchar)), 1)
    fill = sepchar * N

    if sepcolor is not None:
        fill = f"<{sepcolor}>{fill}</>"

    if msgcolor is not None:
        msg = f"<{msgcolor}>{msg}</>"
    line = "%s %s %s" % (fill, msg, fill)

    if len(line) + len(sepchar.rstrip()) <= fullwidth:
        line += sepchar.rstrip()

    return line

def _format_exception(report):

    if not report.failed:
        return ""

    tw = py.io.TerminalWriter(file=None, stringio=True)
    tw.hasmarkup = True
    tw.fullwidth = report_width
    report.toterminal(tw)

    return tw._file.getvalue()

def report_test_headline(logger, testid, config, workspace):
    """
    Emit a log message describing a test configuration
    """

    lg = logger.bind(skip_prefix=True).opt(depth=1, colors=True)

    lg.info(_add_sep("=", "Test session starts"))

    verinfo = platform.python_version()
    msg = "platform {} -- Python {}".format(sys.platform, verinfo)
    pypy_version_info = getattr(sys, "pypy_version_info", None)
    if pypy_version_info:
        verinfo = ".".join(map(str, pypy_version_info[:3]))
        msg += "[pypy-{}-{}]".format(verinfo, pypy_version_info[3])
    msg += ", pytest-{}, py-{}, pluggy-{}".format(
        pytest.__version__, py.__version__, pluggy.__version__
    )

    lg.info(f"<normal>{msg}</>")

    msg = "rootdir: %s" % config.rootdir

    if config.inifile:
        msg += ", inifile: " + config.rootdir.bestrelpath(config.inifile)

    testpaths = config.getini("testpaths")
    if testpaths and config.args == testpaths:
        rel_paths = [config.rootdir.bestrelpath(x) for x in testpaths]
        msg += ", testpaths: {}".format(", ".join(rel_paths))

    lg.info(f"<normal>{msg}</>")
    lg.info(f"<normal>workspace: {workspace.twd}</>")
    lg.info(f"\n<yellow>{_add_sep('=', testid)}</>\n")

def report_test_status(logger, report):
    """
    Emit a log message describing a test report
    """

    lg_opts = logger.opt(colors=True).bind(skip_prefix=True)

    def get_phase(report):
        if report.when == "setup":
            return "SETUP"
        elif report.when == "call":
            return "TEST"
        elif report.when == "teardown":
            return "TEARDOWN"
        else:
            raise ValueError("Test report has unknown phase")

    def get_status(report):
        TestReport = namedtuple(
                'TestReport', ['phase', 'color', 'status', 'logfun'])

        phase = get_phase(report)
        was_xfail = hasattr(report, "wasxfail")

        if report.passed and not was_xfail:
            return TestReport(phase, "green", "PASSED", lg_opts.info)
        elif report.passed and was_xfail:
            return TestReport(phase, "yellow", "PASSED", lg_opts.warning)
        elif report.failed:
            return TestReport(phase, "red", "FAILED", lg_opts.error)
        elif report.skipped:
            return TestReport(phase, "yellow", "SKIPPED", lg_opts.warning)
        else:
            raise ValueError("Test report has unknown state")


    phase, color, status, log_fun = get_status(report)
    msg = _add_sep("=", f"{phase} {status}")
    log_fun(f"\n<{color}>{msg}</>\n")

    if report.failed:
        location = f"{_add_sep('_', report.location[2])}"
        exception = _format_exception(report)
        lg_opts.info(f"<normal>{location}\n{{}}</>", exception)
        log_fun(f"{'=' * report_width}")

def report_assertion_pass(logger, location, orig, expl):

    def patch_record(r):
        copy = r.copy()
        copy["file"].name = Path(location.path).name
        copy["file"].path = location.path
        copy["function"] = location.function
        copy["line"] = location.lineno
        #copy["module"] = location.module
        copy["name"] = Path(location.path).stem
        r.update(copy)

    logger.patch(lambda r : patch_record(r)).info(
        f"assertion \"{orig}\" passed")
