#ifndef CHIRP_SRC_SERVICE_SERVER_H_
#define CHIRP_SRC_SERVICE_SERVER_H_

#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

#include "service_data_structure.h"
#include "service.grpc.pb.h"

class ServiceImpl final : public chirp::ServiceLayer::Service {
 public:
  explicit ServiceImpl();

  grpc::Status registeruser(grpc::ServerContext *context,
                            const chirp::RegisterRequest *request,
                            chirp::RegisterReply *reply) override;

  grpc::Status chirp(grpc::ServerContext *context,
                     const chirp::ChirpRequest *request,
                     chirp::ChirpReply *reply) override;

  grpc::Status follow(grpc::ServerContext *context,
                      const chirp::FollowRequest *request,
                      chirp::FollowReply *reply) override;

  grpc::Status read(grpc::ServerContext *context,
                    const chirp::ReadRequest *request,
                    chirp::ReadReply *reply) override;

  grpc::Status monitor(grpc::ServerContext *context,
                       const chirp::MonitorRequest *request,
                       grpc::ServerWriter<chirp::MonitorReply> *writer) override;

 private:
  class ServiceDataStructure service_data_structure_;

  void InternalChirpToGrpcChirp(const ServiceDataStructure::Chirp &internal_chirp,
                                chirp::Chirp * const grpc_chirp);
  void DFSScanChirps(chirp::ReadReply * const reply, const uint64_t &chirp_id);

  inline uint64_t ChirpIdBytesToUint(const std::string &chirp_id_in_bytes) {
    return *(reinterpret_cast<const uint64_t*>(chirp_id_in_bytes.c_str()));
  }

  inline std::string ChirpIdUintToBytes(const uint64_t &chirp_id_in_uint) {
    return std::string(reinterpret_cast<const char*>(&chirp_id_in_uint), sizeof(uint64_t));
  }
};

#endif /* CHIRP_SRC_SERVICE_SERVER_H_ */
