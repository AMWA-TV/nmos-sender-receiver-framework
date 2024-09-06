#!/bin/bash

set -eu
SCRIPT_DIR="$(realpath "$(dirname "$0")")"
source ${SCRIPT_DIR}/common.sh

clean_build
clean_dist
clean_install
