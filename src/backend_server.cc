#include "key_value.grpc.pb.h"
#include <map>
#include <string>

#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

namespace {
    std::map<std::string, std::string > internal_data;
}

class KeyValueStoreImpl final : public chirp::KeyValueStore::Service {
    public:
        explicit KeyValueStoreImpl(void) {}

        grpc::Status put(grpc::ServerContext* context,
                const chirp::PutRequest* request,
                chirp::PutReply* reply) override {

            // may need locks to ensure thread safety in the future
            internal_data[request->key()] = request->value();

            return grpc::Status::OK;
        }

        grpc::Status get(grpc::ServerContext* context,
                grpc::ServerReaderWriter<chirp::GetReply, chirp::GetRequest>* stream) override {

            chirp::GetRequest request;

            while(stream->Read(&request)) {
                chirp::GetReply reply;
                if (internal_data.count(request.key()) > 0) {
                    reply.set_value(internal_data[request.key()]);
                }
                stream->Write(reply);
            }

            return grpc::Status::OK;
        }

        grpc::Status deletekey(grpc::ServerContext* context,
                const chirp::DeleteRequest* request,
                chirp::DeleteReply* reply) override {
            internal_data.erase(request->key());

            return grpc::Status::OK;
        }
};

void run_server(void)
{
    std::string server_address("0.0.0.0:50000");
    KeyValueStoreImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server is listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char** argv)
{
    run_server();

    return 0;
}
