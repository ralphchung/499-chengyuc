#ifndef CHIRP_SRC_BACKEND_SERVER_H_
#define CHIRP_SRC_BACKEND_SERVER_H_

#include <atomic>
#include <map>
#include <string>

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "backend_data_structure.h"
#include "key_value.grpc.pb.h"

// Key-value store implementation inherits from the
// `chirp::KeyValueStore::Service` which implements the `put`, `get`, and
// `deletekey` operations
class KeyValueStoreImpl final : public chirp::KeyValueStore::Service {
 public:
  explicit KeyValueStoreImpl();

  // Accepts put requests
  grpc::Status put(grpc::ServerContext *context,
                   const chirp::PutRequest *request,
                   chirp::PutReply *reply) override;

  // Accepts get requests
  grpc::Status get(grpc::ServerContext *context,
                   grpc::ServerReaderWriter<chirp::GetReply, chirp::GetRequest>
                       *stream) override;

  // Accepts deletekey requests
  grpc::Status deletekey(grpc::ServerContext *context,
                         const chirp::DeleteRequest *request,
                         chirp::DeleteReply *reply) override;

 private:
  BackendDataStructure backend_data_;

  // spinlock
  std::atomic_flag lock_;
};

#endif /* CHIRP_SRC_BACKEND_SERVER_H_ */
