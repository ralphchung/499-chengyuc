#include <cstdio>
#include <iostream>
#include <istream>
#include <regex>
#include <string>
#include <sstream>

#include "gtest/gtest.h"

#include "command_line_tool_lib.h"

namespace {

inline uint64_t GetChirpIdFromStr(const std::string &post) {
  std::smatch m;
  std::regex_search(post, m, std::regex(" [0-9]*\n"));
  return stoull(m[0]);
}

// This series of tests still requires `service_server` and `backend_server` to run simultaneously
class CommandLineTest : public ::testing::Test {
 protected:
  void SetUp() override {
    username = "user";
  }

  std::string username;
};

// This tests register operation
TEST_F(CommandLineTest, RegisterTest) {
  // Set up for redirecting standard output
  std::stringstream buffer;
  std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());
  std::string output;

  // Register one user
  // ServiceClient::ReturnCodes
  auto ret = command_tool::Register(username);
  // This should be successful
  EXPECT_EQ(ServiceClient::OK, ret);
  // check whether the output message is correct
  std::string expected_success_output = std::string("Registered username: ") + username + std::string(": ") + command_tool::service_client.ErrorMsgs[ServiceClient::OK] + std::string("\n");
  EXPECT_EQ(expected_success_output, buffer.str());
  // clean the `buffer
  buffer.str(std::string());

  // Register the same user again
  ret = command_tool::Register(username);
  EXPECT_EQ(ServiceClient::USER_EXISTS, ret);
  // check whether the output message is correct
  std::string expected_fail_output = std::string("Registered username: ") + username + std::string(": ") + command_tool::service_client.ErrorMsgs[ServiceClient::USER_EXISTS] + std::string("\n");
  EXPECT_EQ(expected_fail_output, buffer.str());
  // clean the `buffer
  buffer.str(std::string());

  // restore the standard output
  std::cout.rdbuf(old);
}

// This tests chirp operation
TEST_F(CommandLineTest, ChirpTest) {
  // Set up for redirecting standard output
  std::stringstream buffer;
  std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());
  std::string output;

  // ensure the username is registered
  command_tool::Register(username);
  // clean the `buffer
  buffer.str(std::string());

  // Post a chirp from a non-existed user
  // ServiceClient::ReturnCodes
  auto ret = command_tool::Chirp(std::string("non-existed"), std::string("text"));
  // This should fail
  EXPECT_EQ(ServiceClient::USER_NOT_FOUND, ret);
  // check whether the output message is correct
  std::string expected_fail_output = std::string("Posted a chirp as non-existed: ") + command_tool::service_client.ErrorMsgs[ServiceClient::USER_NOT_FOUND] + std::string("\n");
  EXPECT_EQ(expected_fail_output, buffer.str());
  // clean the `buffer
  buffer.str(std::string());

  // Post a chirp
  ret = command_tool::Chirp(username, std::string("text"));
  // This should be successful
  EXPECT_EQ(ServiceClient::OK, ret);
  // check whether the output message is correct
  std::regex expected_single_output(std::string("Posted a chirp as ") + username + std::string(": ") + command_tool::service_client.ErrorMsgs[ServiceClient::OK] + std::string("\n\nID: [0-9]*\n@") + username + " \u00B7 .*\ntext\n");
  EXPECT_TRUE(std::regex_match(buffer.str(), expected_single_output));
  // record its chirp id for later use
  uint64_t chirp_id = GetChirpIdFromStr(buffer.str());
  // clean the `buffer
  buffer.str(std::string());

  // Reply a non-existed chirp
  ret = command_tool::Chirp(username, std::string("text"), UINT_MAX);
  // This should fail
  EXPECT_EQ(ServiceClient::REPLY_ID_NOT_FOUND, ret);
  // check whether the output message is correct
  std::string expected_fail_reply_output = std::string("Posted a chirp as ") + username + std::string(": ") + command_tool::service_client.ErrorMsgs[ServiceClient::REPLY_ID_NOT_FOUND] + std::string("\n");
  EXPECT_EQ(expected_fail_reply_output, buffer.str());
  // clean the `buffer
  buffer.str(std::string());

  // Reply a chirp
  ret = command_tool::Chirp(username, std::string("text"), chirp_id);
  // This should be successful
  EXPECT_EQ(ServiceClient::OK, ret);
  // check whether the output message is correct
  std::regex expected_reply_output(std::string("Posted a chirp as ") + username + std::string(": ") + command_tool::service_client.ErrorMsgs[ServiceClient::OK] + std::string("\n\nID: [0-9]*\n@") + username + " \u00B7 .*\nReply: " + std::to_string(chirp_id) + "\ntext\n");
  EXPECT_TRUE(std::regex_match(buffer.str(), expected_reply_output));
  // clean the `buffer
  buffer.str(std::string());

  // restore the standard output
  std::cout.rdbuf(old);
}

// This tests follow operation
TEST_F(CommandLineTest, FollowTest) {
  // Set up for redirecting standard output
  std::stringstream buffer;
  std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());
  std::string output;

  // ensure the username is registered
  command_tool::Register(username);
  // clean the `buffer
  buffer.str(std::string());

  // follow a non-existed username
  std::string non_existed("non-existed");
  // ServiceClient::ReturnCodes
  auto ret = command_tool::Follow(username, non_existed);
  // This should fail
  EXPECT_EQ(ServiceClient::FOLLOWEE_NOT_FOUND, ret);
  // check whether the output message is correct
  std::string expected_fail_output = std::string("Followed ") + non_existed + std::string(" as ") + username + std::string(": ") + command_tool::service_client.ErrorMsgs[ServiceClient::FOLLOWEE_NOT_FOUND] + std::string("\n");
  EXPECT_EQ(expected_fail_output, buffer.str());
  // clean the `buffer
  buffer.str(std::string());

  // follow itself
  ret = command_tool::Follow(username, username);
  // This should be successful
  EXPECT_EQ(ServiceClient::OK, ret);
  // check whether the output message is correct
  std::string expected_output = std::string("Followed ") + username + std::string(" as ") + username + std::string(": ") + command_tool::service_client.ErrorMsgs[ServiceClient::OK] + std::string("\n");
  EXPECT_EQ(expected_output, buffer.str());
  // clean the `buffer`
  buffer.str(std::string());

  // restore the standard output
  std::cout.rdbuf(old);
}

// This tests read operation
TEST_F(CommandLineTest, ReadTest) {
  // Set up for redirecting standard output
  std::stringstream buffer;
  std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());
  std::string output;

  // Set up the chirps
  // Post a top-level chirp
  command_tool::Chirp(username, std::string("text"));
  // record its chirp id for later use
  uint64_t chirp_id_top = GetChirpIdFromStr(buffer.str());
  // clean the `buffer
  buffer.str(std::string());
  // Post two second-level chirps
  command_tool::Chirp(username, std::string("text"), chirp_id_top);
  // record its chirp id for later use
  uint64_t chirp_id_second = GetChirpIdFromStr(buffer.str());
  command_tool::Chirp(username, std::string("text"), chirp_id_top);
  // clean the `buffer
  buffer.str(std::string());
  // Post two third-level chirps
  command_tool::Chirp(username, std::string("text"), chirp_id_second);
  command_tool::Chirp(username, std::string("text"), chirp_id_second);
  // clean the `buffer
  buffer.str(std::string());

  // Read the top-level chirp
  // ServiceClient::ReturnCodes
  auto ret = command_tool::Read(chirp_id_top);
  // This should be successful
  EXPECT_EQ(ServiceClient::OK, ret);
  // check whether the output chirps are correct
  std::regex expected_output(
      std::string("Read a chirp with id [0-9]*: ") + command_tool::service_client.ErrorMsgs[ServiceClient::OK] + "\n" +
                  "\n" +
                  "(-*)\n" +
                  "ID: [0-9]*\n" +
                  "@" + username + " \u00B7 .*\n" +
                  "text\n" +
                  "(-*)\n" +
                  ".ID: [0-9]*\n" +
                  ".@" + username + " \u00B7 .*\n" +
                  ".Reply: [0-9]*\n" +
                  ".text\n" +
                  "(-*)\n" +
                  "..ID: [0-9]*\n" +
                  "..@" + username + " \u00B7 .*\n" +
                  "..Reply: [0-9]*\n" +
                  "..text\n" +
                  "(-*)\n" +
                  "..ID: [0-9]*\n" +
                  "..@" + username + " \u00B7 .*\n" +
                  "..Reply: [0-9]*\n" +
                  "..text\n" +
                  "(-*)\n" +
                  ".ID: [0-9]*\n" +
                  ".@" + username + " \u00B7 .*\n" +
                  ".Reply: [0-9]*\n" +
                  ".text\n" +
                  "(-*)\n");
  EXPECT_TRUE(std::regex_match(buffer.str(), expected_output)) << buffer.str() << std::endl;

  // restore the standard output
  std::cout.rdbuf(old);
}

} // end of namespace

GTEST_API_ int main(int argc, char **argv) {
  std::cout << "Running command-line tool test from " << __FILE__ << std::endl;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
