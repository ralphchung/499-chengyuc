# Project Chirp
* Cheng-Yu Chung
* chengyuc@usc.edu

---

# Vagrantfile
You can check out the [file](Vagrantfile) in the root directory.

## Environemnt
* Box: ```"ubuntu/bionic64"```
* OS: Ubuntu 18.04 LTS
* Provision: N/A

# Setup steps
**Note**: You can either follow the steps below or run the shell script [```setup.sh```](setup.sh) in the root directory (This script requires root permission).

## Basic tools
```shell
$ [sudo] apt update
$ [sudo] apt install gcc g++ make
```

## gRPC

**Dependency**
```shell
$ [sudo] apt install build-essential autoconf libtool pkg-config
```
**Clone the gRPC repository (including submodules)**
```shell
$ git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc ~/grpc
$ cd ~/grpc
$ git submodule update --init
```
**Make and install gRPC**
```shell
$ make
$ [sudo] make install
```
**Protoc**
```shell
$ cd ~/grpc/third_party/protobuf
$ [sudo] make install
```

## gflags
**Installation**
```shell
$ [sudo] apt install libgflags-dev
```

## gtest
**Installation**
```shell
$ [sudo] apt install libgtest-dev cmake
$ cd /usr/src/gtest
$ [sudo] cmake CMakeLists.txt
$ [sudo] make
$ [sudo] cp *.a /usr/lib
```

## glog
**Installation**
```shell
$ [sudo] apt install libgoogle-glog-dev
```

# Compilation instructions
## Backend Key-Value Store
**Server**
```shell
$ make backend_server
$ ./backend_server
```

**Unit test**
```shell
$ make backend_test
$ ./backend_test
```

## Service layer
**Server**
```shell
$ make service_server
$ ./service_server
```
**Unit Test**
```shell
$ make service_test
$ ./service_test
```

## Command-line tool
**Tool**
```shell
$ make command_line_tool
$ ./chirp
```

# Logging
This program uses glog to log. All the logging will be stored in the folder ```./log```. If this folder does not exist, give the following command
```
$ make check_log_folder
```

# Basic example usage
## Usage
```
./chirp operations <arguments> [options arguments]

Operations:
  --register <username>
  --chirp <chirp text>
  --follow <username>
  --read <chirp id>
  --monitor

Options:
  --user <username>
  --reply <reply chirp id>
```

## Examples
**Register a user**
```shell
$ ./chirp --register user
```

**Post a chirp as a user**
```shell
$ ./chirp --chirp text --user user
```

**Read a chirp with id 1**
```shell
$ ./chirp --read 1
```

**Reply a chirp with id 1**
```shell
$ ./chirp --chirp text --user user --reply 1
```

**Follow a user**
```shell
$ ./chirp --follow another_user --user user
```

**Monitor as user**
```shell
$ ./chirp --monitor --user user
```
