#ifndef CHIRP_TEST_BACKEND_CLIENT_LIB_H_
#define CHIRP_TEST_BACKEND_CLIENT_LIB_H_

#include <memory>
#include <string>
#include <vector>

#include <grpcpp/channel.h>
#include "key_value.grpc.pb.h"

class backend_client {
    public:
        backend_client(void);
        backend_client(const std::string&);
        backend_client(const std::string&, const std::string&);
        ~backend_client(void);

        bool send_put_request(const std::string&, const std::string&);
        bool send_get_request(const std::vector<std::string>&, std::vector<std::string>&);
        bool send_get_request(const std::vector<std::string>::iterator, const std::vector<std::string>::iterator, std::vector<std::string>&);
        bool send_deletekey_request(const std::string&);

    private:
        std::string host;
        std::string port;

        std::shared_ptr<grpc::Channel> channel;
        std::unique_ptr<chirp::KeyValueStore::Stub> stub;
};

#endif // CHIRP_TEST_BACKEND_CLIENT_LIB_H_
