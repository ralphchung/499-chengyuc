#ifndef CHIRP_SRC_SERVICE_SERVER_H_
#define CHIRP_SRC_SERVICE_SERVER_H_

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "service.grpc.pb.h"
#include "service_data_structure.h"

// Service implementation inherits from `chirp::ServiceLayer::Service`
// It implements `registeruser`, `chirp`, `follow`, `read`, and `monitor`
// operations
class ServiceImpl final : public chirp::ServiceLayer::Service {
 public:
  explicit ServiceImpl();

  // This accepts registeruser request
  // returns grpc::Status::Ok if this operation succeeds
  grpc::Status registeruser(grpc::ServerContext *context,
                            const chirp::RegisterRequest *request,
                            chirp::RegisterReply *reply) override;

  // This accepts chirp request
  // returns grpc::Status::Ok if this operation succeeds
  grpc::Status chirp(grpc::ServerContext *context,
                     const chirp::ChirpRequest *request,
                     chirp::ChirpReply *reply) override;

  // This accepts follow request
  // returns grpc::Status::Ok if this operation succeeds
  grpc::Status follow(grpc::ServerContext *context,
                      const chirp::FollowRequest *request,
                      chirp::FollowReply *reply) override;

  // This accepts read request
  // returns grpc::Status::Ok if this operation succeeds
  grpc::Status read(grpc::ServerContext *context,
                    const chirp::ReadRequest *request,
                    chirp::ReadReply *reply) override;

  // This accepts monitor request
  // returns grpc::Status::Ok if this operation succeeds
  grpc::Status monitor(
      grpc::ServerContext *context, const chirp::MonitorRequest *request,
      grpc::ServerWriter<chirp::MonitorReply> *writer) override;

  // This accepts stream request
  // returns grpc::Status::Ok if this operation succeeds
  grpc::Status stream(
      grpc::ServerContext *context, const chirp::StreamRequest *request,
      grpc::ServerWriter<chirp::StreamReply> *writer) override;

 private:
  // This instantiates a ServiceDataStructure so that those operations above
  // can leverage this.
  ServiceDataStructure service_data_structure_;

  // This is a helper function that helps translate a
  // `ServiceDataStructure::Chirp` object to a grpc version of `chirp::Chirp`
  // object.
  void InternalChirpToGrpcChirp(
      const ServiceDataStructure::Chirp &internal_chirp,
      chirp::Chirp *const grpc_chirp);

  // This is a helper function that helps traverse all children chirps within a
  // chirp in a DFS manner.
  ServiceDataStructure::ReturnCodes DfsScanChirps(chirp::ReadReply *const reply,
                                                  const uint64_t &chirp_id);

  // This is a helper function that helps translate `ReturnCodes` to
  // `grpc::Status`
  grpc::Status ReturnCodesToGrpcStatus(
      const ServiceDataStructure::ReturnCodes &ret);
};

#endif /* CHIRP_SRC_SERVICE_SERVER_H_ */
