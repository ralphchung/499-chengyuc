#ifndef CHIRP_SRC_BACKEND_CLIENT_LIB_H_
#define CHIRP_SRC_BACKEND_CLIENT_LIB_H_

#include <memory>
#include <string>
#include <vector>

#include <grpcpp/channel.h>

#include "key_value.grpc.pb.h"

// A client that is used to communicate with the backend server
class BackendClient {
 public:
  // ctors to set the server host and port
  BackendClient();
  BackendClient(const std::string &port);
  BackendClient(const std::string &host, const std::string &port);
  ~BackendClient();

  // Send a put request to the server
  // returns true if this operation succeeds
  // returns false otherwise
  bool SendPutRequest(const std::string &key, const std::string &value);
  // Send a get request to the server
  // This member function will change the vector that `reply_values` points to.
  // It will not change anything if `reply_values` is nullptr
  // returns true if this operation succeeds
  // returns false otherwise
  bool SendGetRequest(const std::vector<std::string> &keys, std::vector<std::string> *reply_values);
  // Send a delete key request to the server
  // returns true if this operation succeeds
  // returns false otherwise
  bool SendDeleteKeyRequest(const std::string &key);

 private:
  // server hostname
  std::string host_;
  // server port number
  std::string port_;

  // Two variables from grpc library to set up the connection.
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<chirp::KeyValueStore::Stub> stub_;
};

#endif // CHIRP_TEST_BACKEND_CLIENT_LIB_H_
