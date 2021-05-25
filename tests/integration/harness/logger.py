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

import re
import logging
from loguru import logger

# remove loguru's default logger
logger.remove()

class LogFormatter:
    """
    Formatter class for our log messages
    """

    def __init__(self, colorize=True):
        self._colorize = colorize

        # color
        self._prefix_fmt = "<green>{time:YYYY-MM-DD HH:mm:ss.SSS}</green> | <level>{level: <8}</level> | <cyan>{name}</cyan>:<cyan>{line}</cyan> | "
        self._suffix_fmt = "<level>{message}</level>\n"

        # raw
        self._raw_prefix_fmt = "{time:YYYY-MM-DD HH:mm:ss.SSS} | {level: <8} | {name}:{line} | "
        self._raw_suffix_fmt = "{extra[raw_message]}\n"

        if colorize:
            self._short_fmt = self._suffix_fmt
            self._fmt = self._prefix_fmt + self._suffix_fmt
        else:
            self._short_fmt = self._raw_suffix_fmt
            self._fmt = self._raw_prefix_fmt + self._raw_suffix_fmt


    def format(self, record):

        def _ansi_strip(msg):
            # 7-bit C1 ANSI sequences
            ansi_escape = re.compile(r'''
                \x1B  # ESC
                (?:   # 7-bit C1 Fe (except CSI)
                    [@-Z\\-_]
                |     # or [ for CSI, followed by a control sequence
                    \[
                    [0-?]*  # Parameter bytes
                    [ -/]*  # Intermediate bytes
                    [@-~]   # Final byte
                )
            ''', re.VERBOSE)
            return ansi_escape.sub('', msg)

        if not self._colorize:
            record["extra"]["raw_message"] = _ansi_strip(record["message"])

        patch_location = record["extra"].get("patch_location", None)
        if patch_location:
            record.update(file="foobar")

        if record["extra"].get("skip_prefix", False):
            return self._short_fmt

        return self._fmt

class PropagateHandler(logging.Handler):
    """
    This class ensures that loguru messages are propagated to caplog
    """
    def emit(self, record):
        logging.getLogger(record.name).handle(record)


def initialize_logging(logger, test_log_path, propagate=False):

    handles = []

    # remove loguru's default logger
    logger.remove()

    # create loggers:
    #   1. log to file with ansi color codes
    h0 = logger.add(
            test_log_path.with_suffix(".color.log"),
            colorize=True,
            format=LogFormatter().format)
    handles.append(h0)

    #   2. log to file with plain text
    h1 = logger.add(test_log_path, 
            colorize=False,
            format=LogFormatter(False).format)
    handles.append(h1)

    #   3. log propagator to pytest
    if propagate:
        h2 = logger.add(
                PropagateHandler(),
                colorize=True,
                format=LogFormatter().format)
        handles.append(h2)

    return handles

def finalize_logging(logger, handles):

    for h in handles:
        logger.remove(h)
