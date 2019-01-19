
PROTO_PATH := ./proto
SRC_PATH := ./src

check_src_folder:
	mkdir -p "$(SRC_PATH)"

proto: check_src_folder
	for proto in `ls $(PROTO_PATH)/*.proto`; do \
		protoc --proto_path=$(PROTO_PATH) --cpp_out=$(SRC_PATH) $$proto ; \
		protoc --proto_path=$(PROTO_PATH) --grpc_out=$(SRC_PATH) --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` $$proto ; \
	done

# remove the compiled output from "protoc"
remove_compiled_proto:
	rm -f $(SRC_PATH)/*.pb.*

clean: remove_compiled_proto
