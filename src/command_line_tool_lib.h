#ifndef CHIRP_COMMAND_LINE_TOOL_H_
#define CHIRP_COMMAND_LINE_TOOL_H_

#include <string>
#include <vector>

#include "service_client_lib.h"

namespace command_tool {
int Register(const std::string &username);

int Chirp(const std::string &username, const std::string &text, const uint64_t &parent_id = 0);

int Follow(const std::string &username, const std::string &to_follow);

int Read(const uint64_t &chirp_id);

int Monitor(const std::string &username);

void PrintSingleChirp(const struct ServiceClient::Chirp &chirp, unsigned padding);

void PrintChirps(const std::vector<struct ServiceClient::Chirp> &chirps);

extern ServiceClient service_client;

extern std::string usage;

} /* command_tool */

#endif /* CHIRP_COMMAND_LINE_TOOL_H_ */
