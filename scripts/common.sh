#!/bin/bash

set -eu

SCRIPT_DIR="$(realpath "$(dirname "$0")")"
PROJECT_DIR=${SCRIPT_DIR}/..

############################################
PROJECT_NAME="ossrf"
CONFIGURATIONS="Release Debug"
############################################
BUILD_DIR="${PROJECT_DIR}/build"
INSTALL_DIR=${PROJECT_DIR}/install
DIST_DIR=${PROJECT_DIR}/dist
############################################
CONAN_MIN_VERSION="2.0.0"
############################################

[ -d ${DIST_DIR} ] || mkdir -p ${DIST_DIR}
[ -d ${INSTALL_DIR} ] || mkdir -p ${INSTALL_DIR}

function setup() {
    pushd ${PROJECT_DIR}
    for configuration in $CONFIGURATIONS; do
        conan install . --output-folder ${PROJECT_DIR} --build=missing --settings=build_type="$configuration" --settings=compiler.cppstd=17 --lockfile=conan.lock
    done
    popd
}

function configure() {
    for configuration in $CONFIGURATIONS; do
        pushd ${BUILD_DIR}/$configuration
        source ./generators/conanbuild.sh
        local preset="conan-${configuration,,}"
        cmake ${PROJECT_DIR} --preset ${preset}
        popd
    done
}

function build_configuration() {
    local configuration=$1
    pushd ${BUILD_DIR}/$configuration
    source ./generators/conanbuild.sh
    cmake --build .
    ctest .
    popd
}

function build() {
    for configuration in $CONFIGURATIONS; do
        build_configuration "$configuration"
    done
}

function pack_configuration() {
    local configuration=$1
    pushd ${BUILD_DIR}/$configuration
    cpack
    popd
}

function pack() {
    build_configuration Release
    pack_configuration Release

    local version=$(cat "${BUILD_DIR}/Release/cpp/version.txt")

    echo "Packing ${PROJECT_NAME} v${version}..."
    local installer_base_name="${BUILD_DIR}/Release/${PROJECT_NAME}-${version}-Linux"
    mv "${installer_base_name}.sh" ${DIST_DIR}
    rm "${installer_base_name}.tar.Z"
    rm "${installer_base_name}.tar.gz"
}

function clean_build() {
    rm -rf ${BUILD_DIR}
}

function clean_dist() {
    if [ -d ${DIST_DIR} ]; then
        rm -rf ${DIST_DIR}
    fi
}

function clean_install() {
    if [ -d ${INSTALL_DIR} ]; then
        rm -rf ${INSTALL_DIR}
    fi
}

function install_installer() {
    clean_install
    mkdir -p ${INSTALL_DIR}

    pushd ${INSTALL_DIR}
    local version=$(cat "${BUILD_DIR}/Release/cpp/version.txt")
    INSTALLER="${DIST_DIR}/${PROJECT_NAME}-${version}-Linux.sh"
    bash ${INSTALLER} --skip-license
    popd
}

function test_installer() {
    pushd ${SCRIPT_DIR}/test/dist/bare_demo
    INSTALL_ROOT=${INSTALL_DIR} make clean
    INSTALL_ROOT=${INSTALL_DIR} make
    echo "starting test demo..."
    LD_LIBRARY_PATH=${INSTALL_DIR}/lib ./main
    echo "finished with code " $?
    popd
}

function check_conan_version() {
    conan_version_full="$(conan --version)"
    conan_version="${conan_version_full/Conan version /}"
    conan_required_version=${CONAN_MIN_VERSION}

    if [ "$(printf '%s\n' "$conan_required_version" "$conan_version" | sort -V | head -n1)" = "$conan_required_version" ]; then
        echo "Conan version is ${conan_version}"
    else
        echo "Conan version is less than ${conan_required_version}"
        exit 1
    fi
}
