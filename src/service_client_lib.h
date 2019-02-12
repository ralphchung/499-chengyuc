#ifndef CHIRP_SRC_SERVICE_CLIENT_LIB_H_
#define CHIRP_SRC_SERVICE_CLIENT_LIB_H_

#include <memory>
#include <string>
#include <vector>

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "grpc_client_lib.h"
#include "service.grpc.pb.h"

#include <iostream>

// A client that is used to communicate with the service server
class ServiceClient : public GrpcClient<chirp::ServiceLayer::Stub> {
 public:
  enum ReturnCodes : int {
    OK = 0,
    INVALID_ARGUMENT,
    USER_EXISTS,
    USER_NOT_FOUND,
    FOLLOWEE_NOT_FOUND,
    CHIRP_ID_NOT_FOUND,
    REPLY_ID_NOT_FOUND,
    PERMISSION_DENIED,

    INTENRAL_BACKEND_ERROR,
    SERVICE_LAYER_UNAVAILABLE,

    UNKOWN_ERROR // should always be the last
  };

  const char *ErrorMsgs[UNKOWN_ERROR + 1] = {
    "Success", // OK
    "Invalid arguments have been passed.", // INVALID_ARGUMENT
    "Username specified already exists.", // USER_EXISTS
    "Username specified have not been registered.", // USER_NOT_FOUND
    "Followee username specified does not exist.", // FOLLOWEE_NOT_FOUND
    "Cannot find the specified chirp id.", // CHIRP_ID_NOT_FOUND
    "Cannot find the specified reply id.", // REPLY_ID_NOT_FOUND
    "User does not have the permission to do so.", // PERMISSION_DENIED
    "Something goes wrong in the backend server.", // INTENRAL_BACKEND_ERROR
    "Unable to communicate with service layer.", // SERVICE_LAYER_UNAVAILABLE
    "Unknown error." // UNKOWN_ERROR
  };

  // forward declaration
  struct Chirp;

  // Constructor that doesn't take any argument
  // hostname will be "locolhost" and port number number will be "50002"
  ServiceClient();

  // Constructor that takes one argument to be the hostname
  // hostname is specified in the argument and port number will be "50002"
  ServiceClient(const std::string &host);

  // Send a user register request to the server
  // returns OK if this operation succeeds
  // returns other error codes if this operation fails
  ReturnCodes SendRegisterUserRequest(const std::string &username);

  // Send a post chirp request to the server
  // returns OK if this operation succeeds
  // returns other error codes if this operation fails
  ReturnCodes SendChirpRequest(const std::string &username,
                        const std::string &text,
                        const uint64_t &parent_id,
                        struct Chirp * const chirp);

  // Send a follow request to the server
  // returns OK if this operation succeeds
  // returns other error codes if this operation fails
  ReturnCodes SendFollowRequest(const std::string &username, const std::string &to_follow);

  // Send a read request to the server
  // returns OK if this operation succeeds
  // returns other error codes if this operation fails
  ReturnCodes SendReadRequest(const uint64_t &chirp_id, std::vector<struct Chirp> * const chirp);

  // Send a monitor request to the server
  // returns OK if this operation succeeds
  // returns other error codes if this operation fails
  ReturnCodes SendMonitorRequest(const std::string &username, std::vector<Chirp> * const chirps);

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

  // helper function to translate grpc status to `ReturnCodes`
  ReturnCodes GrpcStatusToReturnCodes(const grpc::Status &status);
};

#endif /* CHIRP_SRC_SERVICE_CLIENT_LIB_H_ */
