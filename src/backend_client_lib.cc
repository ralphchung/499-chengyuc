#include "backend_client_lib.h"

#include <thread>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "key_value.grpc.pb.h"

backend_client::backend_client(void) :
    host("localhost"), port("50000") {
        channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
        stub = chirp::KeyValueStore::NewStub(channel);
}

backend_client::backend_client(const std::string& port) :
    host("localhost"), port(port) {
        channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
        stub = chirp::KeyValueStore::NewStub(channel);
}

backend_client::backend_client(const std::string& host, const std::string& port) :
    host(host), port(port) {
        channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
        stub = chirp::KeyValueStore::NewStub(channel);
}

backend_client::~backend_client() {}

bool backend_client::send_put_request(const std::string& key, const std::string& value)
{
    grpc::ClientContext context;

    chirp::PutRequest request;
    request.set_key(key);
    request.set_value(value);
    chirp::PutReply reply;

    grpc::Status status = stub->put(&context, request, &reply);

    return status.ok();
}

bool backend_client::send_get_request(const std::vector<std::string>& keys, std::vector<std::string>& reply_values)
{
    grpc::ClientContext context;
    std::shared_ptr<grpc::ClientReaderWriter<chirp::GetRequest, chirp::GetReply> > stream(stub->get(&context));

    std::thread writer([&stream, &keys]() {
        for(const std::string& key : keys) {
            chirp::GetRequest request;
            request.set_key(key);
            stream->Write(request);
        }

        stream->WritesDone();
    });

    chirp::GetReply reply;
    while(stream->Read(&reply)) {
        reply_values.push_back(reply.value());
    }

    writer.join();
    grpc::Status status = stream->Finish();

    return status.ok();
}

bool backend_client::send_get_request(const std::vector<std::string>::iterator keys_begin, const std::vector<std::string>::iterator keys_end, std::vector<std::string>& reply_values)
{
    grpc::ClientContext context;
    std::shared_ptr<grpc::ClientReaderWriter<chirp::GetRequest, chirp::GetReply> > stream(stub->get(&context));

    std::thread writer([&stream, keys_begin, keys_end]() {
        for(auto it = keys_begin; it != keys_end; ++it) {
            chirp::GetRequest request;
            request.set_key(*it);
            stream->Write(request);
        }

        stream->WritesDone();
    });

    chirp::GetReply reply;
    while(stream->Read(&reply)) {
        reply_values.push_back(reply.value());
    }

    writer.join();
    grpc::Status status = stream->Finish();

    return status.ok();
}

bool backend_client::send_deletekey_request(const std::string& key)
{
    grpc::ClientContext context;

    chirp::DeleteRequest request;
    request.set_key(key);
    chirp::DeleteReply reply;

    grpc::Status status = stub->deletekey(&context, request, &reply);

    return status.ok();
}
