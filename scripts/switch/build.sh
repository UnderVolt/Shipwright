#!/bin/bash

cmake -H. -Bbuild-switch -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Switch.cmake
cmake --build build-switch --config Debug --target soh_nro -j $(nproc)
nxlink -s build-switch/soh/soh.nro