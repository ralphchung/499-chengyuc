#include <csignal>
#include <fstream>
#include <iostream>
#include <istream>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

#include "backend_client_lib.h"

namespace {
    std::string shell_prefix = "";
    std::unique_ptr<backend_client> client(nullptr);

    std::unique_ptr<chirp::KeyValueStore::Stub> stub(nullptr);

    void SIGNT_handler(int signum)
    {
        std::cout << "Ctrl-C caught" << std::endl;
    }

    void try_open(std::ifstream& ifs, const char* filename)
    {
        ifs.open(filename, std::ios::in);
        if (ifs.is_open() == false) {
            std::cerr << "Failed to open the file " << filename << std::endl;
            exit(1);
        }
    }

    void put_operation(const std::vector<std::string>& splited_command)
    {
        if (splited_command.size() != 3) {
            std::cerr << "Error: usage: put key value\n";
            return;
        }
        const std::string& key = splited_command[1];
        const std::string& value = splited_command[2];

        bool ret = client->send_put_request(key, value);

        if (ret != true) {
            std::cout << "error: put failed\n";
        }
    }

    void get_operation(std::vector<std::string> splited_command)
    {
        if (splited_command.size() <= 1) {
            std::cerr << "Error: usage: get ...\n";
            return;
        }

        std::vector<std::string> output;
        bool ret = client->send_get_request(splited_command.begin() + 1, splited_command.end(), output);

        if (ret != true) {
            std::cout << "error: get failed\n";
            return;
        }

        for(auto& s : output) {
            std::cout << s << std::endl;
        }
    }

    void delete_operation(const std::vector<std::string>& splited_command)
    {
        if (splited_command.size() != 2) {
            std::cerr << "Error: usage: get key\n";
            return;
        }
        const std::string& key = splited_command[1];

        bool ret = client->send_deletekey_request(key);

        if (ret != true) {
            std::cout << "error: delete failed\n";
        }
    }

    void dispatch_operation(const std::vector<std::string>& splited_command)
    {
        const std::string& opt = splited_command[0];
        if (opt == "put") {
            put_operation(splited_command);
        }
        else if (opt == "get") {
            get_operation(splited_command);
        }
        else if (opt == "delete") {
            delete_operation(splited_command);
        }
        else {
            std::cout << "unknown" << std::endl;
        }
    }

    std::streambuf* input_redirect(const int& argc, char**& argv)
    {
        std::streambuf* buf;

        // file mode
        if (argc > 1) {
            static std::ifstream ifs;
            try_open(ifs, argv[1]);
            buf = ifs.rdbuf();
        }
        // cin mode
        else {
            buf = std::cin.rdbuf();
            shell_prefix = "> ";
        }

        return buf;
    }
}

int main(int argc, char** argv)
{
    // client initialization
    client.reset(new backend_client());
    // end of client initialization

    // 'input_stream' will be the only stream to get all the input
    // either from a file or std::cin
    std::istream input_stream(input_redirect(argc, argv));
    // end of initialization for getting inputs

    // Initialization for signal handlers
    std::signal(SIGINT, SIGNT_handler);
    // end of initialization for signal handlers

    // parse the command
    std::string command;
    while(std::cout << shell_prefix, std::getline(input_stream, command)) {
        if (command.size() == 0) {
            continue;
        }

        std::stringstream ss(command);
        std::string tmp;
        std::vector<std::string> splited_command;

        while(ss >> tmp) {
            splited_command.push_back(tmp);
        }

        dispatch_operation(splited_command);
    }

    return 0;
}
