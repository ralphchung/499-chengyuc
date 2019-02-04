#include "backend_client_lib.h"

#include <thread>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "key_value.grpc.pb.h"

#ifdef DEBUG
#include <map>
#include <string>

std::map<std::string, std::string> key_value;
#endif /* DEBUG */

const char* kDefaultHostname = "localhost";
const char* kDefaultPort = "50000";

BackendClient::BackendClient()
    : host_(kDefaultHostname), port_(kDefaultPort) {
  channel_ = grpc::CreateChannel(host_ + ":" + port_,
                                 grpc::InsecureChannelCredentials());
  stub_ = chirp::KeyValueStore::NewStub(channel_);
}

BackendClient::BackendClient(const std::string &host)
    : host_(host), port_(kDefaultPort) {
  channel_ = grpc::CreateChannel(host_ + ":" + port_,
                                 grpc::InsecureChannelCredentials());
  stub_ = chirp::KeyValueStore::NewStub(channel_);
}

BackendClient::BackendClient(const std::string &host, const std::string &port)
    : host_(host), port_(port) {
  channel_ = grpc::CreateChannel(host_ + ":" + port_,
                                 grpc::InsecureChannelCredentials());
  stub_ = chirp::KeyValueStore::NewStub(channel_);
}

BackendClient::~BackendClient() {}

bool BackendClient::SendPutRequest(const std::string &key,
                                   const std::string &value) {
  #ifdef DEBUG
  key_value[key] = value;
  return true;

  #else

  grpc::ClientContext context;

  chirp::PutRequest request;
  request.set_key(key);
  request.set_value(value);
  chirp::PutReply reply;

  grpc::Status status = stub_->put(&context, request, &reply);

  return status.ok();
  #endif /* DEBUG */
}

bool BackendClient::SendGetRequest(const std::vector<std::string> &keys,
                                   std::vector<std::string> *reply_values) {
  #ifdef DEBUG
  for(const auto &key : keys) {
    reply_values->push_back(key_value[key]);
  }
  return true;

  #else

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

  #endif /* DEBUG */
}

bool BackendClient::SendDeleteKeyRequest(const std::string &key) {
  #ifdef DEBUG
  return key_value.erase(key);

  #else

  grpc::ClientContext context;

  chirp::DeleteRequest request;
  request.set_key(key);
  chirp::DeleteReply reply;

  grpc::Status status = stub_->deletekey(&context, request, &reply);

  return status.ok();

  #endif /* DEBUG */
}
