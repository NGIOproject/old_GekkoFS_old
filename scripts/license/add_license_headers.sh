#!/bin/bash
set -e
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJ_DIR=`readlink -f ${DIR}/../../`

docker run --rm --volume ${PROJ_DIR}:/usr/src/ osterman/copyright-header:latest \
    --syntax scripts/license/copyright_header_syntax.yml \
    --license-file scripts/license/header \
    --guess-extension \
    --word-wrap 80 \
    --output-dir /usr/src/ \
    --add-path ./src:./include
