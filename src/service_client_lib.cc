#include "service_client_lib.h"

#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "service.grpc.pb.h"

namespace {
const char* kDefaultHostname = "localhost";
const char* kDefaultPort = "50002";
} // Anonymous namespace

ServiceClient::ServiceClient()
    : host_(kDefaultHostname), port_(kDefaultPort) {
  InitChannelAndStub();
}

ServiceClient::ServiceClient(const std::string &host)
    : host_(host), port_(kDefaultPort) {
  InitChannelAndStub();
}

ServiceClient::ServiceClient(const std::string &host, const std::string &port)
    : host_(host), port_(port) {
  InitChannelAndStub();
}

bool ServiceClient::SendRegisterUserRequest(const std::string &username) {
  grpc::ClientContext context;

  chirp::RegisterRequest request;
  request.set_username(username);

  chirp::RegisterReply reply;

  grpc::Status status = stub_->registeruser(&context, request, &reply);

  return status.ok();
}

bool ServiceClient::SendChirpRequest(const std::string &username,
                                     const std::string &text,
                                     const uint64_t &parent_id,
                                     struct ServiceClient::Chirp * const chirp) {
  grpc::ClientContext context;

  chirp::ChirpRequest request;
  request.set_username(username);
  request.set_text(text);
  std::string parent_id_in_bytes(reinterpret_cast<const char*>(&parent_id), sizeof(uint64_t));
  request.set_parent_id(parent_id_in_bytes);

  chirp::ChirpReply reply;

  grpc::Status status = stub_->chirp(&context, request, &reply);

  if (chirp != nullptr && status.ok()) {
    GrpcChirpToClientChirp(reply.chirp(), chirp);
  }

  return status.ok();
}

bool ServiceClient::SendFollowRequest(const std::string &username, const std::string &to_follow) {
  grpc::ClientContext context;

  chirp::FollowRequest request;
  request.set_username(username);
  request.set_to_follow(to_follow);

  chirp::FollowReply reply;

  grpc::Status status = stub_->follow(&context, request, &reply);

  return status.ok();
}

bool ServiceClient::SendReadRequest(const uint64_t &chirp_id,
                                    std::vector<struct ServiceClient::Chirp> * const chirps) {
  grpc::ClientContext context;

  chirp::ReadRequest request;
  std::string chirp_id_in_bytes(reinterpret_cast<const char*>(&chirp_id), sizeof(uint64_t));
  request.set_chirp_id(chirp_id_in_bytes);

  chirp::ReadReply reply;

  grpc::Status status = stub_->read(&context, request, &reply);

  if (chirps != nullptr) {
    for(size_t i = 0; i < reply.chirps_size(); ++i) {
      struct ServiceClient::Chirp chirp;
      GrpcChirpToClientChirp(reply.chirps(i), &chirp);
      chirps->push_back(std::move(chirp));
    }
  }

  return status.ok();
}

bool ServiceClient::SendMonitorRequest(const std::string &username,
                                       std::vector<ServiceClient::Chirp> * const chirps) {
  grpc::ClientContext context;

  chirp::MonitorRequest request;
  request.set_username(username);

  std::unique_ptr<grpc::ClientReader<chirp::MonitorReply> > reader(stub_->monitor(&context, request));

  chirp::MonitorReply reply;
  while (reader->Read(&reply)) {
    struct ServiceClient::Chirp client_chirp;
    GrpcChirpToClientChirp(reply.chirp(), &client_chirp);
    if (chirps != nullptr) {
      chirps->push_back(client_chirp);
    }
  }

  grpc::Status status = reader->Finish();

  return status.ok();
}
