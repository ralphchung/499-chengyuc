#include "backend_server.h"

#include <map>
#include <string>
#include <utility>

#include <grpc/grpc.h>
#include <grpcpp/impl/codegen/status.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

#include "backend_data_structure.h"
#include "key_value.grpc.pb.h"

#define DEFAULT_HOST_AND_PORT "0.0.0.0:50000"

KeyValueStoreImpl::KeyValueStoreImpl() : backend_data_(), lock_(ATOMIC_FLAG_INIT) {}

grpc::Status KeyValueStoreImpl::put(
    grpc::ServerContext *context,
    const chirp::PutRequest *request,
    chirp::PutReply *reply) {
  
  if (request == nullptr) {
    return grpc::Status(grpc::INVALID_ARGUMENT,
                        "`PutRequest` is nullptr.",
                        "");
  }

  // acquire lock
  while (lock_.test_and_set(std::memory_order_acquire))
    ; // spin
  bool ok = backend_data_.Put(request->key(), request->value());
  // release lock
  lock_.clear(std::memory_order_release);

  if (ok) {
    return grpc::Status::OK;
  }
  else {
    return grpc::Status(grpc::UNKNOWN, "Unknown error happened.", "");
  }
}

grpc::Status KeyValueStoreImpl::get(
    grpc::ServerContext *context,
    grpc::ServerReaderWriter<chirp::GetReply, chirp::GetRequest> *stream) {

  chirp::GetRequest request;

  // acquire lock
  while (lock_.test_and_set(std::memory_order_acquire))
    ; // spin
  while(stream->Read(&request)) {
    chirp::GetReply reply;
    std::string value;

    bool ok = backend_data_.Get(request.key(), &value);
    if (ok) {
      reply.set_value(value);
    }
    else {
      reply.set_value(std::string());
    }

    stream->Write(reply);
  }
  // release lock
  lock_.clear(std::memory_order_release);

  return grpc::Status::OK;
}

grpc::Status KeyValueStoreImpl::deletekey(
    grpc::ServerContext *context,
    const chirp::DeleteRequest *request,
    chirp::DeleteReply *reply) {

  if (request == nullptr) {
    return grpc::Status(grpc::INVALID_ARGUMENT,
                        "`PutRequest` is nullptr.",
                        "");
  }

  // acquire lock
  while (lock_.test_and_set(std::memory_order_acquire))
    ; // spin
  bool ok = backend_data_.DeleteKey(request->key());
  // release lock
  lock_.clear(std::memory_order_release);

  if (ok) {
    return grpc::Status::OK;
  }
  else {
    return grpc::Status(grpc::NOT_FOUND, "Unknown error happened.", "");
  }
}

void run_server() {
  std::string server_address(DEFAULT_HOST_AND_PORT);
  KeyValueStoreImpl service;

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server is listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char **argv) {
  run_server();

  return 0;
}
