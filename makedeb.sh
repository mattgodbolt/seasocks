#!/bin/bash -x

# make a working directory and put a copy of the source tarball in there
mkdir WORK && cd WORK

# need a copy of the tarball in the WORK directory
cp ../seasocks-1.4.4.tar.gz

# expand the upstream tarball source
tar xvf seasocks-1.4.4.tar.gz

# build the package
cd seasocks-1.4.4 
dh_make -f ../*seasocks-1.4.4.tar.gz
dpkg-buildpackage -uc -us
# check its contents
dpkg --contents ../*.deb

echo It should be able to install now with dpkg --install
