#ifndef CHIRP_SRC_SERVICE_CLIENT_LIB_H_
#define CHIRP_SRC_SERVICE_CLIENT_LIB_H_

#include <memory>
#include <string>
#include <vector>

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "service.grpc.pb.h"

#include <iostream>

// A client that is used to communicate with the service server
class ServiceClient {
 public:
  // forward declaration
  struct Chirp;

  // Constructor that doesn't take any argument
  // hostname will be "locolhost" and port number number will be "50002"
  ServiceClient();

  // Constructor that takes one argument to be the hostname
  // hostname is specified in the argument and port number will be "50002"
  ServiceClient(const std::string &host);

  // Constructor that takes two arguments which are hostname and port number
  // hostname and port number will be specified in the arguments
  ServiceClient(const std::string &host, const std::string &port);

  // Send a user register request to the server
  // returns true if this operation succeeds
  // returns false otherwise
  bool SendRegisterUserRequest(const std::string &username);

  // Send a post chirp request to the server
  // returns true if this operation succeeds
  // returns false otherwise
  bool SendChirpRequest(const std::string &username,
                        const std::string &text,
                        const uint64_t &parent_id,
                        struct Chirp * const chirp);

  // Send a follow request to the server
  // returns true if this opeation succeeds
  // returns false otherwise
  bool SendFollowRequest(const std::string &username, const std::string &to_follow);

  // Send a read request to the server
  // returns true if this operation succeeds
  // returns false otherwise
  bool SendReadRequest(const uint64_t &chirp_id, std::vector<struct Chirp> * const chirp);

  // Send a monitor request to the server
  // returns true if this operation succeeds
  // returns false otherwise
  bool SendMonitorRequest(const std::string &username, std::vector<Chirp> * const chirps);

  struct Chirp {
    struct Timestamp {
      uint64_t seconds;
      uint64_t useconds;
    };

    std::string username;
    std::string text;
    uint64_t id;
    uint64_t parent_id;
    struct Timestamp timestamp;
  };

 private:
  // server hostname
  std::string host_;
  // server port number
  std::string port_;

  // Two variables from grpc library to set up the connection.
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<chirp::ServiceLayer::Stub> stub_;

  // helper function to initialize the `channel_` and `stub_`
  inline void InitChannelAndStub() {
    channel_ = grpc::CreateChannel(host_ + ":" + port_,
                                   grpc::InsecureChannelCredentials());
    stub_ = chirp::ServiceLayer::NewStub(channel_);
  }

  // Translate grpc chirp to the chirp we define here
  inline void GrpcChirpToClientChirp(const chirp::Chirp &grpc_chirp, struct Chirp * const client_chirp) {
    if (client_chirp != nullptr) {
      client_chirp->username = grpc_chirp.username();
      client_chirp->text = grpc_chirp.text();
      client_chirp->id = *(reinterpret_cast<const uint64_t*>(grpc_chirp.id().c_str()));
      client_chirp->parent_id = *(reinterpret_cast<const uint64_t*>(grpc_chirp.parent_id().c_str()));
      client_chirp->timestamp.seconds = grpc_chirp.timestamp().seconds();
      client_chirp->timestamp.useconds = grpc_chirp.timestamp().useconds();
    }
  }
};

#endif /* CHIRP_SRC_SERVICE_CLIENT_LIB_H_ */
