#ifndef CHIRP_GRPC_CLIENT_H_
#define CHIRP_GRPC_CLIENT_H_

#include <memory>
#include <string>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

template <typename GrpcStub>
// A client that is used to communicate with servers using grpc
class GrpcClient {
 public:
  // Constructor that takes two arguments which are hostname and port number
  // hostname and port number will be specified in the arguments
  GrpcClient(const std::string &host, std::string &port)
      : host_(host), port_(port) {
    channel_ = grpc::CreateChannel(host_ + ":" + port_,
                                   grpc::InsecureChannelCredentials());
    stub_.reset(new GrpcStub(channel_));
  }

  GrpcClient(const char *host, const char *port) : host_(host), port_(port) {
    channel_ = grpc::CreateChannel(host_ + ":" + port_,
                                   grpc::InsecureChannelCredentials());
    stub_.reset(new GrpcStub(channel_));
  }

 protected:
  // Two variables from grpc library to set up the connection.
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<GrpcStub> stub_;

 private:
  // server hostname
  std::string host_;
  // server port number
  std::string port_;
};

#endif /* CHIRP_GRPC_CLIENT_H_ */
