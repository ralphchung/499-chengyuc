# 499-chengyu
* Cheng-Yu Chung
* chengyuc@usc.edu

# Pre-requisites

## gRPC

### Dependency
```
$ [sudo] apt install build-essential autoconf libtool pkg-config
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

# Components
## *.proto files
```
$ make protos
```

## Backend Key-Value Store
### Server
```
$ make backend_server
$ ./backend_server
```

### Shell
```
$ make shell_backend
```
**Usage**:
```
$ ./shell_backend [script file]
```
**Commands**:
```
put key value
get key...
delete key
```
