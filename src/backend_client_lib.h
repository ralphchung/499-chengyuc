#ifndef CHIRP_SRC_BACKEND_CLIENT_LIB_H_
#define CHIRP_SRC_BACKEND_CLIENT_LIB_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <grpcpp/channel.h>

#include "grpc_client_lib.h"
#include "key_value.grpc.pb.h"

// This is an abstract class for backend clients.
// Those who are going to inherit this should implement the three interfaces
// which are `SendPutRequest`, `SendGetRequest`, and `SendDeleteKeyRequest`
class BackendClient : public GrpcClient<chirp::KeyValueStore::Stub> {
 public:
  // Constructor that doesn't take any argument
  // hostname will be "localhost" and port number will be "50000"
  BackendClient();

  // Constructor that takes one argument to be the hostname
  // hostname is specified in the argument and port number will be "50000"
  BackendClient(const std::string &host);

  // Send a put request to the server
  // returns true if this operation succeeds
  // returns false otherwise
  virtual bool SendPutRequest(const std::string &key,
                              const std::string &value) = 0;

  // Send a get request to the server
  // This member function will change the vector that `reply_values` points to.
  // It will not change anything if `reply_values` is nullptr
  // returns true if this operation succeeds
  // returns false otherwise
  virtual bool SendGetRequest(const std::vector<std::string> &keys,
                              std::vector<std::string> *reply_values) = 0;

  // Send a delete key request to the server
  // returns true if this operation succeeds
  // returns false otherwise
  virtual bool SendDeleteKeyRequest(const std::string &key) = 0;
};

// This is the standard version of backend client
// which will complete the requests through grpc
class BackendClientStandard : public BackendClient {
 public:
  bool SendPutRequest(const std::string &key,
                      const std::string &value) override;
  bool SendGetRequest(const std::vector<std::string> &keys,
                      std::vector<std::string> *reply_values) override;
  bool SendDeleteKeyRequest(const std::string &key) override;
};

// This is the debug version of backend client
// which will complete the requests locally without going through grpc
class BackendClientDebug : public BackendClient {
 public:
  bool SendPutRequest(const std::string &key,
                      const std::string &value) override;
  bool SendGetRequest(const std::vector<std::string> &keys,
                      std::vector<std::string> *reply_values) override;
  bool SendDeleteKeyRequest(const std::string &key) override;

 private:
  std::map<std::string, std::string> key_value_;
};

#endif // CHIRP_TEST_BACKEND_CLIENT_LIB_H_
