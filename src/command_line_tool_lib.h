#ifndef CHIRP_COMMAND_LINE_TOOL_H_
#define CHIRP_COMMAND_LINE_TOOL_H_

#include <string>
#include <vector>

#include "service_client_lib.h"

// This namespace contains functions for command-line tool
namespace command_tool {
// This executes register operation through grpc using the API in `service_client_lib`
// the `username` cannot be an empty string
// returns OK if succeed
// returns other error codes otherwise
ServiceClient::ReturnCodes Register(const std::string &username);

// This executes post a chirp operation through grpc using the API in `service_client_lib`
// the `username` cannot be an empty string
// if the `parent_id` is not 0, this chirp is replying to another chirp
// returns OK if succeed
// returns other error codes otherwise
ServiceClient::ReturnCodes Chirp(const std::string &username, const std::string &text, const uint64_t &parent_id = 0);

// This executes follow operation through grpc using the API in `service_client_lib`
// the `username` and `to_follow` cannot be empty strings
// returns OK if succeed
// returns other error codes otherwise
ServiceClient::ReturnCodes Follow(const std::string &username, const std::string &to_follow);

// This executes read operation through grpc using the API in `service_client_lib`
// returns OK if succeed
// returns other error codes otherwise
ServiceClient::ReturnCodes Read(const uint64_t &chirp_id);

// This executes monitor operation through grpc using the API in `service_client_lib`
// the `username` cannot be an empty string
// This function should never return except users press Ctrl+C in the console
// returns OK if succeed
// returns other error codes otherwise
ServiceClient::ReturnCodes Monitor(const std::string &username);

// This is a helper function that helps print a single chirp
// used in `Chirp()`, `Read()`, and `PrintChirps()`
// the `padding` controls the indention of this chirp
void PrintSingleChirp(const struct ServiceClient::Chirp &chirp, unsigned padding);

// This is a helper function that hekos print a series of chirps
// This assumes the order of the input is ordered so that it can be easily iterated in the DFS manner
void PrintChirps(const std::vector<struct ServiceClient::Chirp> &chirps);

// The `ServiceClient` object is declared here
// defined in `command_line_tool.cc`
extern ServiceClient service_client;

// The usage string shows the user needs some instructions to use this program
// defined in `command_line_tool.cc`
extern std::string usage;

} /* command_tool */

#endif /* CHIRP_COMMAND_LINE_TOOL_H_ */
