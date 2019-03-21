#include "service_server.h"

#include <sys/time.h>
#include <chrono>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <iostream>

ServiceImpl::ServiceImpl() : service_data_structure_() {}

grpc::Status ServiceImpl::registeruser(grpc::ServerContext *context,
                                       const chirp::RegisterRequest *request,
                                       chirp::RegisterReply *reply) {
  if (context == nullptr || request == nullptr) {
    return grpc::Status(grpc::FAILED_PRECONDITION,
                        "`ServerContext` or `RegisterRequest` is nullptr.");
  }

  // ServiceDataStructure::ReturnCodes
  auto ret = service_data_structure_.UserRegister(request->username());
  return ReturnCodesToGrpcStatus(ret);
}

grpc::Status ServiceImpl::chirp(grpc::ServerContext *context,
                                const chirp::ChirpRequest *request,
                                chirp::ChirpReply *reply) {
  if (context == nullptr || request == nullptr || reply == nullptr) {
    return grpc::Status(
        grpc::FAILED_PRECONDITION,
        "`ServerContext`, `RegisterRequest`, or `reply` is nullptr.");
  }

  // std::unique_ptr<ServiceDataStructure::UserSession>
  auto user_session = service_data_structure_.UserLogin(request->username());
  if (user_session == nullptr) {
    return grpc::Status(grpc::NOT_FOUND, "user");
  }

  uint64_t chirp_id;
  // ServiceDataStructure::ReturnCodes
  auto ret = user_session->PostChirp(request->text(), &chirp_id,
                                     ChirpIdBytesToUint(request->parent_id()));
  if (ret != ServiceDataStructure::OK) {
    return ReturnCodesToGrpcStatus(ret);
  }

  struct ServiceDataStructure::Chirp internal_chirp;
  chirp::Chirp *grpc_chirp = new chirp::Chirp();
  // ServiceDataStructure::ReturnCodes
  ret = service_data_structure_.ReadChirp(chirp_id, &internal_chirp);
  CHECK(ret == ServiceDataStructure::OK)
      << "The newly created chirp should not fail to be read.";

  InternalChirpToGrpcChirp(internal_chirp, grpc_chirp);
  reply->set_allocated_chirp(grpc_chirp);

  return grpc::Status::OK;
}

grpc::Status ServiceImpl::follow(grpc::ServerContext *context,
                                 const chirp::FollowRequest *request,
                                 chirp::FollowReply *reply) {
  if (context == nullptr || request == nullptr) {
    return grpc::Status(grpc::FAILED_PRECONDITION,
                        "`ServerContext` or `RegisterRequest` is nullptr.");
  }

  auto user_session = service_data_structure_.UserLogin(request->username());
  if (user_session == nullptr) {
    return grpc::Status(grpc::NOT_FOUND, "user");
  }

  // ServiceDataStructure::ReturnCodes
  auto ret = user_session->Follow(request->to_follow());

  return ReturnCodesToGrpcStatus(ret);
}

grpc::Status ServiceImpl::read(grpc::ServerContext *context,
                               const chirp::ReadRequest *request,
                               chirp::ReadReply *reply) {
  if (context == nullptr || request == nullptr || reply == nullptr) {
    return grpc::Status(
        grpc::FAILED_PRECONDITION,
        "`ServerContext`, `RegisterRequest`, or `reply` is nullptr.");
  }

  // ServiceDataStructure::ReturnCodes
  auto ret = DfsScanChirps(reply, ChirpIdBytesToUint(request->chirp_id()));
  return ReturnCodesToGrpcStatus(ret);
}

grpc::Status ServiceImpl::monitor(
    grpc::ServerContext *context, const chirp::MonitorRequest *request,
    grpc::ServerWriter<chirp::MonitorReply> *writer) {
  if (context == nullptr || request == nullptr || writer == nullptr) {
    return grpc::Status(
        grpc::FAILED_PRECONDITION,
        "`ServerContext`, `RegisterRequest`, or `writer` is nullptr.");
  }

  const int mseconds_per_wait = 50;
  const int times_count = INT_MAX;

  struct timeval start_time;
  gettimeofday(&start_time, nullptr);

  auto user_session = service_data_structure_.UserLogin(request->username());
  if (user_session == nullptr) {
    return grpc::Status(grpc::NOT_FOUND, "Failed to login.");
  }

  // This indicates that the stream is still on
  int cnt = 0;
  bool flag = true;

  while (flag && cnt < times_count) {
    // sleep a while to avoid busy polling
    std::this_thread::sleep_for(std::chrono::milliseconds(mseconds_per_wait));

    // `start_time` will be modified to the time backend collects the chirps
    std::set<uint64_t> chirps_collector =
        user_session->MonitorFrom(&start_time);

    // May use a thread to do the following things
    if (chirps_collector.size() > 0) {
      for (const auto &chirp_id : chirps_collector) {
        struct ServiceDataStructure::Chirp internal_chirp;
        // ServiceDataStructure::ReturnCodes
        auto ret = service_data_structure_.ReadChirp(chirp_id, &internal_chirp);
        // ignore errors here
        if (ret != ServiceDataStructure::OK) {
          continue;
        }

        chirp::MonitorReply reply;
        chirp::Chirp *grpc_chirp = new chirp::Chirp();
        InternalChirpToGrpcChirp(internal_chirp, grpc_chirp);
        reply.set_allocated_chirp(grpc_chirp);

        if (!writer->Write(reply)) {
          flag = false;
          break;
        }
      }

      cnt = 0;
    } else {
      ++cnt;
    }
  }

  return grpc::Status::OK;
}

void ServiceImpl::InternalChirpToGrpcChirp(
    const ServiceDataStructure::Chirp &internal_chirp,
    chirp::Chirp *const grpc_chirp) {
  if (grpc_chirp == nullptr) {
    return;
  }

  chirp::Timestamp *timestamp = new chirp::Timestamp();
  timestamp->set_seconds(internal_chirp.time.tv_sec);
  timestamp->set_useconds(internal_chirp.time.tv_usec);

  grpc_chirp->set_username(internal_chirp.user);
  grpc_chirp->set_text(internal_chirp.text);
  grpc_chirp->set_id(ChirpIdUintToBytes(internal_chirp.id));
  grpc_chirp->set_parent_id(ChirpIdUintToBytes(internal_chirp.parent_id));
  grpc_chirp->set_allocated_timestamp(timestamp);
}

ServiceDataStructure::ReturnCodes ServiceImpl::DfsScanChirps(
    chirp::ReadReply *const reply, const uint64_t &chirp_id) {
  struct ServiceDataStructure::Chirp internal_chirp;
  // ServiceDataStructure::ReturnCodes
  auto ret = service_data_structure_.ReadChirp(chirp_id, &internal_chirp);
  if (ret != ServiceDataStructure::OK) {
    return ServiceDataStructure::CHIRP_ID_NOT_FOUND;
  }

  chirp::Chirp *grpc_chirp = reply->add_chirps();
  InternalChirpToGrpcChirp(internal_chirp, grpc_chirp);

  for (const auto &id : internal_chirp.children_ids) {
    // ServiceDataStructure::ReturnCodes
    auto ret = DfsScanChirps(reply, id);
    if (ret != ServiceDataStructure::OK) {
      return ret;
    }
  }

  return ServiceDataStructure::OK;
}

grpc::Status ServiceImpl::ReturnCodesToGrpcStatus(
    const ServiceDataStructure::ReturnCodes &ret) {
  switch (ret) {
    case ServiceDataStructure::OK:
      return grpc::Status::OK;
    case ServiceDataStructure::INVALID_ARGUMENT:
      return grpc::Status(grpc::INVALID_ARGUMENT, "");
    case ServiceDataStructure::USER_EXISTS:
      return grpc::Status(grpc::ALREADY_EXISTS, "user");
    case ServiceDataStructure::FOLLOWEE_NOT_FOUND:
      return grpc::Status(grpc::NOT_FOUND, "followee");
    case ServiceDataStructure::CHIRP_ID_NOT_FOUND:
      return grpc::Status(grpc::NOT_FOUND, "chirp id");
    case ServiceDataStructure::REPLY_ID_NOT_FOUND:
      return grpc::Status(grpc::NOT_FOUND, "reply id");
    case ServiceDataStructure::PERMISSION_DENIED:
      return grpc::Status(grpc::PERMISSION_DENIED, "");
    case ServiceDataStructure::INTERNAL_BACKEND_ERROR:
      return grpc::Status(grpc::INTERNAL, "backend");
    default:
      return grpc::Status(grpc::UNKNOWN, "");
  }
}

void run_server() {
  const char *server_address = "0.0.0.0:50002";
  ServiceImpl service;

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
