#!/bin/bash

set -eu
SCRIPT_DIR="$(realpath "$(dirname "$0")")"
source ${SCRIPT_DIR}/common.sh

configure
build
