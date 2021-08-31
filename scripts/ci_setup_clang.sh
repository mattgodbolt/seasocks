#!/bin/bash

VERSION=$1

wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
add-apt-repository "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-${VERSION} main"
apt update
apt-get install -y clang-${VERSION} libc++-${VERSION}-dev libc++abi-${VERSION}-dev

if [[ ${VERSION} -ge 12 ]]; then
    apt-get install -y --no-install-recommends libunwind-${VERSION}-dev
fi
