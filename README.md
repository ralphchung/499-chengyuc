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

## gflags
### Installation
```
$ [sudo] apt install libgflags-dev
```

## gtest
### Installation
```
$ [sudo] apt install libgtest-dev cmake
$ cd /usr/src/gtest
$ [sudo] cmake CMakeLists.txt
$ [sudo] make
$ [sudo] cp *.a /usr/lib
```

## glog
### Installation
```
$ [sudo] apt install libgoogle-glog-dev
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

### Unit Test
#### backend_test
```
$ make backend_test
$ ./backend_test
```

## Service layer
### Server
```
$ make service_server
$ ./service_server
```

### Unit Test
#### service_test
```
$ make service_test
$ ./service_test
```
#### service_test: debug version
```
$ make service_test_debug
$ ./service_test_debug
```

## Command-line tool
### Tool
```
$ make command_line_tool
$ ./chirp
```

### Unit Test
```
$ make command_line_tool_test
$ ./command_line_tool_test
```
