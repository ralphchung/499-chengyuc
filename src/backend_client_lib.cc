#include "backend_client_lib.h"

#include <thread>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "grpc_client_lib.h"
#include "key_value.grpc.pb.h"

namespace {
const char* kDefaultHostname = "localhost";
const char* kDefaultPort = "50000";
} // Anonymous namespace

// Start of `BackendClient` definitions
BackendClient::BackendClient()
    : GrpcClient<chirp::KeyValueStore::Stub>(kDefaultHostname, kDefaultPort) {}

BackendClient::BackendClient(const std::string &host)
    : GrpcClient<chirp::KeyValueStore::Stub>(host.c_str(), kDefaultPort) {}
// End of `BackendClient` definitions

// Start of `BackendClientStandard` definitions
bool BackendClientStandard::SendPutRequest(const std::string &key,
                                           const std::string &value) {
  grpc::ClientContext context;

  chirp::PutRequest request;
  request.set_key(key);
  request.set_value(value);
  chirp::PutReply reply;

  grpc::Status status = stub_->put(&context, request, &reply);

  return status.ok();
}

bool BackendClientStandard::SendGetRequest(
    const std::vector<std::string> &keys,
    std::vector<std::string> *reply_values) {
  grpc::ClientContext context;
  std::shared_ptr<grpc::ClientReaderWriter<chirp::GetRequest, chirp::GetReply>> stream(stub_->get(&context));

  // this lambda function takes `stream` and `keys` from this `BackendClient::SendGetRequest` scope
  // and takes them by reference
  // this thread runner fills in the get requests and writes them to the `stream`
  std::thread writer([&stream, &keys]() {
    for (const std::string &key : keys) {
      chirp::GetRequest request;
      request.set_key(key);
      stream->Write(request);
    }

    stream->WritesDone();
  });

  chirp::GetReply reply;
  while (stream->Read(&reply)) {
    reply_values->push_back(reply.value());
  }

  writer.join();
  grpc::Status status = stream->Finish();

  return status.ok();
}

bool BackendClientStandard::SendDeleteKeyRequest(const std::string &key) {
  grpc::ClientContext context;

  chirp::DeleteRequest request;
  request.set_key(key);
  chirp::DeleteReply reply;

  grpc::Status status = stub_->deletekey(&context, request, &reply);

  return status.ok();
}
// End of `BackendClientStandard` definitions

// Start of `BackendClientDebug` definitions
bool BackendClientDebug::SendPutRequest(const std::string &key,
                                        const std::string &value) {
  key_value_[key] = value;
  return true;
}

bool BackendClientDebug::SendGetRequest(
    const std::vector<std::string> &keys,
    std::vector<std::string> *reply_values) {
  for (const auto &key : keys) {
    reply_values->push_back(key_value_[key]);
  }
  return true;
}

bool BackendClientDebug::SendDeleteKeyRequest(const std::string &key) {
  return key_value_.erase(key);
}
// End of `BackendClientDebug` definitions
