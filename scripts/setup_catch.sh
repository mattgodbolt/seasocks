#!/bin/bash

set -e

git clone --branch=v3.11.0 https://github.com/catchorg/Catch2.git
cd Catch2
cmake . -Bbuild
cmake --build build -- -j4
cmake --install build
