#include "command_line_tool_lib.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <stack>
#include <string>
#include <thread>
#include <vector>

#include "service_client_lib.h"

// The definition of the service client
ServiceClient command_tool::service_client;

std::string command_tool::usage;

ServiceClient::ReturnCodes command_tool::Register(const std::string &username) {
  std::cout << "Registered username: " << username << ": ";

  if (username.empty()) {
    std::cout << "Empty username.\n";
    std::cout << command_tool::usage;
    return ServiceClient::INVALID_ARGUMENT;
  }

  // ServiceClient::ReturnCodes
  auto ret = service_client.SendRegisterUserRequest(username);
  std::cout << service_client.ErrorMsgs[ret] << "\n";

  return ret;
}

ServiceClient::ReturnCodes command_tool::Chirp(const std::string &username,
                                               const std::string &text,
                                               const uint64_t &parent_id) {

  std::cout << "Posted a chirp as " << username << ": ";

  if (username.empty()) {
    std::cout << "Empty username.\n";
    std::cout << command_tool::usage;
    return ServiceClient::INVALID_ARGUMENT;
  }

  if (text.empty()) {
    std::cout << "Empty text.\n";
    std::cout << command_tool::usage;
    return ServiceClient::INVALID_ARGUMENT;
  }

  struct ServiceClient::Chirp chirp;
  // ServiceClient::ReturnCodes
  auto ret = service_client.SendChirpRequest(username, text, parent_id, &chirp);
  std::cout << service_client.ErrorMsgs[ret] << "\n";
  if (ret == ServiceClient::OK) {
    std::cout << '\n';
    command_tool::PrintSingleChirp(chirp, 0);
  }

  return ret;
}

ServiceClient::ReturnCodes command_tool::Follow(const std::string &username,
                                                const std::string &to_follow) {

  std::cout << "Followed " << to_follow << " as " << username << ": ";
  if (username.empty()) {
    std::cout << "Empty username.\n";
    std::cout << command_tool::usage;
    return ServiceClient::INVALID_ARGUMENT;
  }

  if (to_follow.empty()) {
    std::cout << "Empty followee username.\n";
    std::cout << command_tool::usage;
    return ServiceClient::INVALID_ARGUMENT;
  }

  // ServiceClient::ReturnCodes
  auto ret = service_client.SendFollowRequest(username, to_follow);
  std::cout << service_client.ErrorMsgs[ret] << "\n";

  return ret;
}

ServiceClient::ReturnCodes command_tool::Read(const uint64_t &chirp_id) {
  std::cout << "Read a chirp with id " << chirp_id << ": ";

  std::vector<struct ServiceClient::Chirp> chirps;
  // ServiceClient::ReturnCodes
  auto ret = service_client.SendReadRequest(chirp_id, &chirps);
  std::cout << service_client.ErrorMsgs[ret] << "\n";

  if (ret == ServiceClient::OK) {
    std::cout << "\n";
    PrintChirps(chirps);
  }

  return ret;
}

ServiceClient::ReturnCodes command_tool::Monitor(const std::string &username) {
  std::cout << "Monitored as " << username << ": ";
  if (username.empty()) {
    std::cout << "Empty username.\n";
    std::cout << command_tool::usage;
    return ServiceClient::INVALID_ARGUMENT;
  }

  std::cout << "\n";

  std::vector<struct ServiceClient::Chirp> chirps;
  bool flag = true;

  std::thread print_chirps([&]() {
    size_t last_size = 0;
    size_t current_size;

    while(flag) {
      current_size = chirps.size();

      for(size_t i = last_size; i < current_size; ++i) {
        PrintSingleChirp(chirps[i], 0);
      }

      last_size = current_size;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });

  // ServiceClient::ReturnCodes
  auto ret = service_client.SendMonitorRequest(username, &chirps);
  std::cout << service_client.ErrorMsgs[ret] << "\n";

  flag = false;
  print_chirps.join();

  return ret;
}

const char padding_char = '|';
void command_tool::PrintSingleChirp(const struct ServiceClient::Chirp &chirp, unsigned padding) {
  std::string prefix(padding, padding_char);

  // Display ID
  std::cout << prefix << "ID: " << chirp.id << '\n';

  // Display username
  std::cout << prefix << '@' << chirp.username << " \u00B7 ";

  // Display time diff
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  std::chrono::system_clock::time_point post_time_tp(std::chrono::seconds(chirp.timestamp.seconds));
  auto diff = now - post_time_tp;
  if (std::chrono::duration_cast<std::chrono::hours>(diff) > std::chrono::seconds(86400)) {
    std::cout << std::chrono::duration_cast<std::chrono::hours>(diff).count() / 24 << " day(s) ago";
  } else if (std::chrono::duration_cast<std::chrono::hours>(diff).count() > 0) {
    std::cout << std::chrono::duration_cast<std::chrono::hours>(diff).count() << " hour(s) ago";
  } else if (std::chrono::duration_cast<std::chrono::minutes>(diff).count() > 0) {
    std::cout << std::chrono::duration_cast<std::chrono::minutes>(diff).count() << " min(s) ago";
  } else {
    std::cout << std::chrono::duration_cast<std::chrono::seconds>(diff).count() << " sec(s) ago";
  }
  std::cout << ' ';

  // Display formated time
  auto post_time_time_t = std::chrono::system_clock::to_time_t(post_time_tp);
  std::cout << '(' << std::put_time(std::localtime(&post_time_time_t), "%F %T") << ")\n";

  // Display parent id if available
  if (chirp.parent_id > 0) {
    std::cout << prefix << "Reply: " << chirp.parent_id << '\n';
  }

  // Display text
  std::cout << prefix << chirp.text << '\n';
}

void command_tool::PrintChirps(const std::vector<struct ServiceClient::Chirp> &chirps) {
  std::stack<uint64_t> dfs;

  std::cout << "--------------------------\n";
  for(const auto &chirp : chirps) {
    while(!dfs.empty() && dfs.top() != chirp.parent_id) {
      dfs.pop();
    }

    PrintSingleChirp(chirp, dfs.size());
    std::cout << "--------------------------\n";

    if (dfs.empty() || dfs.top() == chirp.parent_id) {
      dfs.push(chirp.id);
    }
  }
}
