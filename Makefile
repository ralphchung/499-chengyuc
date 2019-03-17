
PROTO_PATH := ./proto
SRC_PATH := ./src
TEST_PATH := ./test

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

backend_data_structure: $(SRC_PATH)/backend_data_structure.h $(SRC_PATH)/backend_data_structure.cc
	g++ -std=c++11 -c -o $(SRC_PATH)/backend_data_structure.o $(SRC_PATH)/backend_data_structure.cc

backend_server: $(SRC_PATH)/backend_server.h $(SRC_PATH)/backend_server.cc key_value.pb.o key_value.grpc.pb.o backend_data_structure
	g++ -std=c++11 -c -o $(SRC_PATH)/backend_server.o $(SRC_PATH)/backend_server.cc
	g++ $(SRC_PATH)/backend_data_structure.o $(SRC_PATH)/backend_server.o $(SRC_PATH)/key_value.pb.o $(SRC_PATH)/key_value.grpc.pb.o -L/usr/local/lib `pkg-config --libs protobuf grpc++` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl -o backend_server

backend_client_lib: $(SRC_PATH)/grpc_client_lib.h $(SRC_PATH)/backend_client_lib.h $(SRC_PATH)/backend_client_lib.cc key_value.pb.cc key_value.grpc.pb.cc
	g++ -std=c++11 -c -o $(SRC_PATH)/backend_client_lib.o $(SRC_PATH)/backend_client_lib.cc

#shell_backend: $(TEST_PATH)/shell_backend.cc key_value.pb.o key_value.grpc.pb.o backend_client_lib
#	g++ -std=c++11 `pkg-config --cflags protobuf grpc` -I $(SRC_PATH) -c -o $(TEST_PATH)/shell_backend.o $(TEST_PATH)/shell_backend.cc
#	g++ $(SRC_PATH)/key_value.pb.o $(SRC_PATH)/key_value.grpc.pb.o $(SRC_PATH)/backend_client_lib.o $(TEST_PATH)/shell_backend.o -L/usr/local/lib `pkg-config --libs protobuf grpc++` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl -lgflags -o shell_backend

backend_test: $(TEST_PATH)/backend_test.cc key_value.pb.o key_value.grpc.pb.o backend_client_lib backend_data_structure
	g++ -std=c++11 -I $(SRC_PATH) -Igtest/include  -c -o $(TEST_PATH)/backend_test.o $(TEST_PATH)/backend_test.cc
	g++ $(SRC_PATH)/key_value.pb.o $(SRC_PATH)/key_value.grpc.pb.o $(SRC_PATH)/backend_client_lib.o $(SRC_PATH)/backend_data_structure.o $(TEST_PATH)/backend_test.o -L/usr/local/lib -Lgtest/lib -lgtest -lpthread `pkg-config --libs protobuf grpc++` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl -o backend_test

service_data_structure: $(SRC_PATH)/service_data_structure.cc $(SRC_PATH)/service_data_structure.h backend_client_lib
	g++ -std=c++11 -c -o $(SRC_PATH)/service_data_structure.o $(SRC_PATH)/service_data_structure.cc

service_client_lib: $(SRC_PATH)/grpc_client_lib.h $(SRC_PATH)/service_client_lib.h $(SRC_PATH)/service_client_lib.cc service.pb.cc service.grpc.pb.cc
	g++ -std=c++11 -c -o $(SRC_PATH)/service_client_lib.o $(SRC_PATH)/service_client_lib.cc

service_server: $(SRC_PATH)/service_server.h $(SRC_PATH)/service_server.cc service.pb.o service.grpc.pb.o key_value.pb.o key_value.grpc.pb.o service_data_structure
	g++ -std=c++11 -c -o $(SRC_PATH)/service_server.o $(SRC_PATH)/service_server.cc
	g++ $(SRC_PATH)/service_data_structure.o $(SRC_PATH)/service_server.o $(SRC_PATH)/service.pb.o $(SRC_PATH)/service.grpc.pb.o $(SRC_PATH)/key_value.pb.o $(SRC_PATH)/key_value.grpc.pb.o $(SRC_PATH)/backend_client_lib.o -L/usr/local/lib -lglog `pkg-config --libs protobuf grpc++` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl -o service_server

service_test: service_data_structure service_client_lib $(TEST_PATH)/service_test.cc key_value.pb.o key_value.grpc.pb.o service.pb.o service.grpc.pb.o
	g++ -std=c++11 -I $(SRC_PATH) -Igtest/include  -c -o $(TEST_PATH)/service_test.o $(TEST_PATH)/service_test.cc
	g++ $(SRC_PATH)/key_value.pb.o $(SRC_PATH)/key_value.grpc.pb.o $(SRC_PATH)/service.pb.o $(SRC_PATH)/service.grpc.pb.o $(SRC_PATH)/backend_client_lib.o $(SRC_PATH)/service_data_structure.o $(SRC_PATH)/service_client_lib.o $(TEST_PATH)/service_test.o -L/usr/local/lib -Lgtest/lib -lgtest -lpthread -lglog `pkg-config --libs protobuf grpc++` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl -o service_test

command_line_tool_lib: $(SRC_PATH)/command_line_tool_lib.h $(SRC_PATH)/command_line_tool_lib.cc service.pb.cc service.grpc.pb.cc
	g++ -std=c++11 -c -o $(SRC_PATH)/command_line_tool_lib.o $(SRC_PATH)/command_line_tool_lib.cc

command_line_tool: command_line_tool_lib service_client_lib service.pb.o service.grpc.pb.o
	g++ -std=c++11 -c -o $(SRC_PATH)/command_line_tool.o $(SRC_PATH)/command_line_tool.cc
	g++ $(SRC_PATH)/service.pb.o $(SRC_PATH)/service.grpc.pb.o $(SRC_PATH)/command_line_tool_lib.o $(SRC_PATH)/command_line_tool.o $(SRC_PATH)/service_client_lib.o -lgflags `pkg-config --libs protobuf grpc++` -o chirp

command_line_tool_test: $(TEST_PATH)/command_line_tool_test.cc command_line_tool_lib service_client_lib service.pb.o service.grpc.pb.o
	g++ -std=c++11 -I $(SRC_PATH) -Igtest/include -c -o $(TEST_PATH)/command_line_tool_test.o $(TEST_PATH)/command_line_tool_test.cc
	g++ $(SRC_PATH)/service.pb.o $(SRC_PATH)/service.grpc.pb.o $(SRC_PATH)/service_client_lib.o $(SRC_PATH)/command_line_tool_lib.o $(TEST_PATH)/command_line_tool_test.o -L/usr/local/lib -Lgtest/lib -lgtest -lpthread `pkg-config --libs protobuf grpc++` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl -o command_line_tool_test

all: backend_server service_server command_line_tool

all_test: backend_test service_test command_line_tool_test

clean: remove_compiled_proto remove_object_files
	rm -f ./backend_*
	rm -f ./service_*
	rm -f ./command_line_tool_*
	rm -f ./chirp
