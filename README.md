# 499-chengyu
* Cheng-Yu Chung
* chengyuc@usc.edu

# Pre-requisites

## gRPC

### Dependency
```
$ [sudo] apt-get install build-essential autoconf libtool pkg-config
```

### Clone the gRPC repository (including submodules)
```
$ git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc
$ cd grpc
$ git submodule update --init
```

### Make and install gRPC
```
$ make
$ [sudo] make install
```

# Build

## Build *.proto files
```
$ make proto
```
