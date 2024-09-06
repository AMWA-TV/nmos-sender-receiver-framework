#!/bin/bash
set -eu

if [[ -z "${CMAKE_VERSION+x}" ]]; then
    echo ">>> ERROR: CMAKE_VERSION must be defined"
    exit 1
fi

apt-get update -y && apt-get install -y \
    wget \
    curl

apt-get remove cmake

cd /tmp
wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.sh
mkdir /opt/cmake
cp cmake-${CMAKE_VERSION}-linux-x86_64.sh /opt/cmake
cd /opt/cmake
chmod +x cmake-${CMAKE_VERSION}-linux-x86_64.sh
./cmake-${CMAKE_VERSION}-linux-x86_64.sh --skip-license
echo export PATH="/opt/cmake/bin/:${PATH}" >> /etc/profile.d/100-cmake.sh

cd /tmp
rm cmake-${CMAKE_VERSION}-linux-x86_64.sh
cd /
