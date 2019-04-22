#include "service_client_lib.h"

#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "grpc_client_lib.h"
#include "service.grpc.pb.h"
#include "utility.h"

namespace {
const char *kDefaultHostname = "localhost";
const char *kDefaultPort = "50002";
}  // Anonymous namespace

ServiceClient::ServiceClient()
    : GrpcClient<chirp::ServiceLayer::Stub>(kDefaultHostname, kDefaultPort) {}

ServiceClient::ServiceClient(const std::string &host)
    : GrpcClient<chirp::ServiceLayer::Stub>(host.c_str(), kDefaultPort) {}

ServiceClient::ReturnCodes ServiceClient::SendRegisterUserRequest(
    const std::string &username) {
  grpc::ClientContext context;

  chirp::RegisterRequest request;
  request.set_username(username);

  chirp::RegisterReply reply;

  grpc::Status status = stub_->registeruser(&context, request, &reply);

  return GrpcStatusToReturnCodes(status);
}

ServiceClient::ReturnCodes ServiceClient::SendChirpRequest(
    const std::string &username, const std::string &text,
    const uint64_t &parent_id, struct ServiceClient::Chirp *const chirp) {
  grpc::ClientContext context;

  chirp::ChirpRequest request;
  request.set_username(username);
  request.set_text(text);
  request.set_parent_id(Uint64ToBinary(parent_id));

  chirp::ChirpReply reply;

  grpc::Status status = stub_->chirp(&context, request, &reply);

  if (chirp != nullptr && status.ok()) {
    GrpcChirpToClientChirp(reply.chirp(), chirp);
  }

  return GrpcStatusToReturnCodes(status);
}

ServiceClient::ReturnCodes ServiceClient::SendFollowRequest(
    const std::string &username, const std::string &to_follow) {
  grpc::ClientContext context;

  chirp::FollowRequest request;
  request.set_username(username);
  request.set_to_follow(to_follow);

  chirp::FollowReply reply;

  grpc::Status status = stub_->follow(&context, request, &reply);

  return GrpcStatusToReturnCodes(status);
}

ServiceClient::ReturnCodes ServiceClient::SendReadRequest(
    const uint64_t &chirp_id,
    std::vector<struct ServiceClient::Chirp> *const chirps) {
  grpc::ClientContext context;

  chirp::ReadRequest request;
  request.set_chirp_id(Uint64ToBinary(chirp_id));

  chirp::ReadReply reply;

  grpc::Status status = stub_->read(&context, request, &reply);

  if (chirps != nullptr) {
    for (size_t i = 0; i < reply.chirps_size(); ++i) {
      struct ServiceClient::Chirp chirp;
      GrpcChirpToClientChirp(reply.chirps(i), &chirp);
      chirps->push_back(std::move(chirp));
    }
  }

  return GrpcStatusToReturnCodes(status);
}

ServiceClient::ReturnCodes ServiceClient::SendMonitorRequest(
    const std::string &username,
    std::vector<ServiceClient::Chirp> *const chirps) {
  grpc::ClientContext context;

  chirp::MonitorRequest request;
  request.set_username(username);

  std::unique_ptr<grpc::ClientReader<chirp::MonitorReply> > reader(
      stub_->monitor(&context, request));

  std::cout << "Ctrl + C to terminate\n";

  chirp::MonitorReply reply;
  while (reader->Read(&reply)) {
    struct ServiceClient::Chirp client_chirp;
    GrpcChirpToClientChirp(reply.chirp(), &client_chirp);
    if (chirps != nullptr) {
      chirps->push_back(client_chirp);
    }
  }

  grpc::Status status = reader->Finish();

  return GrpcStatusToReturnCodes(status);
}

ServiceClient::ReturnCodes ServiceClient::SendStreamRequest(const std::string &tag,
    std::vector<ServiceClient::Chirp> *const chirps) {
  grpc::ClientContext context;

  chirp::StreamRequest request;
  request.set_tag(tag);

  std::unique_ptr<grpc::ClientReader<chirp::StreamReply> > reader(
      stub_->stream(&context, request));

  std::cout << "Ctrl + C to terminate\n";

  chirp::StreamReply reply;
  while (reader->Read(&reply)) {
    struct ServiceClient::Chirp client_chirp;
    GrpcChirpToClientChirp(reply.chirp(), &client_chirp);
    if (chirps != nullptr) {
      chirps->push_back(client_chirp);
    }
  }

  grpc::Status status = reader->Finish();

  return GrpcStatusToReturnCodes(status);
}

void ServiceClient::GrpcChirpToClientChirp(const chirp::Chirp &grpc_chirp,
                                           struct Chirp *const client_chirp) {
  if (client_chirp == nullptr) {
    return;
  }

  client_chirp->username = grpc_chirp.username();
  client_chirp->text = grpc_chirp.text();
  client_chirp->id = BinaryToUint64(grpc_chirp.id());
  client_chirp->parent_id = BinaryToUint64(grpc_chirp.parent_id());
  client_chirp->timestamp.seconds = grpc_chirp.timestamp().seconds();
  client_chirp->timestamp.useconds = grpc_chirp.timestamp().useconds();
}

ServiceClient::ReturnCodes ServiceClient::GrpcStatusToReturnCodes(
    const grpc::Status &status) {
  if (status.ok()) {
    return OK;
  } else if (status.error_code() == grpc::INVALID_ARGUMENT) {
    return INVALID_ARGUMENT;
  } else if (status.error_code() == grpc::NOT_FOUND) {
    if (status.error_message() == "user") {
      return USER_NOT_FOUND;
    } else if (status.error_message() == "followee") {
      return FOLLOWEE_NOT_FOUND;
    } else if (status.error_message() == "chirp id") {
      return CHIRP_ID_NOT_FOUND;
    } else if (status.error_message() == "reply id") {
      return REPLY_ID_NOT_FOUND;
    } else {
      return UNKOWN_ERROR;
    }
  } else if (status.error_code() == grpc::ALREADY_EXISTS) {
    if (status.error_message() == "user") {
      return USER_EXISTS;
    } else {
      return UNKOWN_ERROR;
    }
  } else if (status.error_code() == grpc::PERMISSION_DENIED) {
    return PERMISSION_DENIED;
  } else if (status.error_code() == grpc::INTERNAL) {
    return INTERNAL_BACKEND_ERROR;
  } else if (status.error_code() == grpc::UNAVAILABLE) {
    return SERVICE_LAYER_UNAVAILABLE;
  } else {
    return UNKOWN_ERROR;
  }
}
