#! /bin/bash

cd ~

# update and install make
apt update
apt install --assume-yes gcc g++ make

# grpc
apt install --assume-yes build-essential autoconf libtool pkg-config
git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc ~/grpc
cd ~/grpc
git submodule update --init
make
make install
# protoc
cd ~/grpc/third_party/protobuf
make install

# gflags
apt install --assume-yes libgflags-dev

# gtest
apt install --assume-yes libgtest-dev cmake
cd /usr/src/gtest
cmake CMakeLists.txt
make
cp *.a /usr/lib

# glog
apt install --assume-yes libgoogle-glog-dev
