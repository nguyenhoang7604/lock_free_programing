#!/bin/bash

set -e

LOCAL_SOURCE_PATH=$(pwd)
nCPUs=$(getconf _NPROCESSORS_ONLN)
nBUILD_JOBs=$(( nCPUs + 1 ))
START_TIME=$(date -u "+%s")
ENABLE_TSAN=ON

PREFIX_PATH=${1:-/build/build}
BUILD_OUTPUT="${PREFIX_PATH}/build.log"
PREFIX_SOURCE_PATH="${PREFIX_PATH}/sources"

mkdir -p "$PREFIX_PATH" "$PREFIX_SOURCE_PATH"

CMAKE_OPTIONS+=" -DCMAKE_C_COMPILER=clang"
CMAKE_OPTIONS+=" -DCMAKE_CXX_COMPILER=clang++"
CMAKE_OPTIONS+=" -DCMAKE_INSTALL_PREFIX=/usr"
CMAKE_OPTIONS+=" -DCMAKE_INSTALL_LIBDIR=lib"

function print_status() {
	local status=$1
	local msg=$2
	if [[ $status -eq 0 ]]; then
		echo -e "\e[1;32m[ OK ]\e[0m $msg"
	else
		echo -e "\e[1;31m[ FAIL ]\e[0m $msg"
		exit $status
	fi
}

function do_init() {
	echo "Cleaning up build directory..."
	rm -rf "$PREFIX_PATH"
	mkdir -p "$PREFIX_SOURCE_PATH"
	print_status $? "Initialization complete."
}

function configure_local() {
	echo "Configuring with CMake..."
	set +e
	cmake -S "$LOCAL_SOURCE_PATH" \
		-B "$PREFIX_SOURCE_PATH" \
		-DCMAKE_BUILD_TYPE=Debug \
		-DENABLE_TSAN=${ENABLE_TSAN} \
		$CMAKE_OPTIONS >> "$BUILD_OUTPUT" 2>&1
	status=$?
	set -e
	print_status $status "CMake configuration complete."
}


function build_local() {
	echo "Building project..."
	set +e  # Disable exit-on-error temporarily
	cmake --build "$PREFIX_SOURCE_PATH" -j "$nBUILD_JOBs" >> "$BUILD_OUTPUT" 2>&1
	status=$?
	set -e  # Re-enable after capturing status
	print_status $status "Build complete."
}

function install_local() {
	echo "Installing project..."
	set +e
	cmake --install "$PREFIX_SOURCE_PATH" --prefix "$PREFIX_PATH" >> "$BUILD_OUTPUT" 2>&1
	status=$?
	set -e
	print_status $status "Installation complete."
}

function do_done() {
	EndTime=$(date -u "+%s")
	BuildTime=$(( EndTime - START_TIME ))
	echo -e "Build took: $(date -u -d @${BuildTime} +%H:%M:%S)"
}

do_init
configure_local
build_local
install_local
do_done
