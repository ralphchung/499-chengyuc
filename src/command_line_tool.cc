#include "command_line_tool_lib.h"

#include <string>

#include <gflags/gflags.h>

DEFINE_string(register, "", "");
DEFINE_string(user, "", "");
DEFINE_string(chirp, "", "");
DEFINE_uint64(reply, 0, "");
DEFINE_string(follow, "", "");
DEFINE_uint64(read, 0, "");
DEFINE_bool(monitor, false, "");

int main(int argc, char **argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  command_tool::usage = std::string("Usage: ") + argv[0] + " --register <username> --user <username> --chirp <chirp text> --reply <reply chirp id> --follow <username> --read <chirp id> --monitor\n";

  if (!FLAGS_register.empty()) {
    return command_tool::Register(FLAGS_register);
  } else if (!FLAGS_chirp.empty()) {
    return command_tool::Chirp(FLAGS_user, FLAGS_chirp, FLAGS_reply);
  } else if (!FLAGS_follow.empty()) {
    return command_tool::Follow(FLAGS_user, FLAGS_follow);
  } else if (FLAGS_read > 0) {
    return command_tool::Read(FLAGS_read);
  } else if (FLAGS_monitor) {
    return command_tool::Monitor(FLAGS_user);
  } else {
    std::cout << command_tool::usage;
    return 1;
  }

  return 0;
}
