
PROTO_PATH := ./proto
SRC_PATH := ./src

CXX := g++
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11

PROTOS_PATH = ./proto

vpath %.proto $(PROTOS_PATH)

check_src_folder:
	mkdir -p "$(SRC_PATH)"

protos: check_src_folder key_value.pb.cc key_value.grpc.pb.cc service.pb.cc service.grpc.pb.cc

%.grpc.pb.cc: %.proto
	protoc --proto_path=$(PROTO_PATH) --grpc_out=$(SRC_PATH) --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` $<

%.pb.cc: %.proto
	protoc --proto_path=$(PROTO_PATH) --cpp_out=$(SRC_PATH) $<

%.pb.o: %.pb.cc
	g++ -std=c++11 `pkg-config --cflags protobuf grpc` -c -o $(SRC_PATH)/$@ $(SRC_PATH)/$<


# remove the compiled output from "protoc"
remove_compiled_proto:
	rm -f $(SRC_PATH)/*.pb.*
remove_object_files:
	rm -f $(SRC_PATH)/*.o
	rm -f $(TEST_PATH)/*.o

backend_server: $(SRC_PATH)/backend_server.cc key_value.pb.o key_value.grpc.pb.o
	g++ -std=c++11 `pkg-config --cflags protobuf grpc` -c -o $(SRC_PATH)/backend_server.o $(SRC_PATH)/backend_server.cc
	g++ $(SRC_PATH)/backend_server.o $(SRC_PATH)/key_value.pb.o $(SRC_PATH)/key_value.grpc.pb.o -L/usr/local/lib `pkg-config --libs protobuf grpc++` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl -o backend_server

clean: remove_compiled_proto remove_object_files
	rm -f backend_server
