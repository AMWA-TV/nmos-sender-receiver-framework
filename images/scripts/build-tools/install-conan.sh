#!/bin/bash

set -eu

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

source ${SCRIPT_DIR}/../common/env.sh

check_env_var CONAN_VERSION

apt-get update -y && apt-get install -y --no-install-recommends \
    software-properties-common \
    python3 \
    python3-pip \
    python3-setuptools \
    python3-wheel

# CONAN
pip3 install --upgrade pip
pip3 install setuptools
pip3 install conan==${CONAN_VERSION} --upgrade
