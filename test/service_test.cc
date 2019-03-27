#include <sys/time.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <glog/logging.h>
#include "gtest/gtest.h"

#include "service_client_lib.h"
#include "service_data_structure.h"

namespace {

// Constants
const size_t kNumOfUsersPreset = 10;
const size_t kNumOfUsersTotal = 20;
// The number of chirps posted
const size_t kNumOfChirps = 10;
// The number of chirps which is going to be deleted
const size_t kHalfNumOfChirps = kNumOfChirps / 2;
const char *kShortText = "short";
const char *kLongText =
    "longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglo"
    "nglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong"
    "longlonglonglonglong";

// This test cases on the `ServiceDataStructure` to check whether their
// interfaces work correctly.
class ServiceTestDataStructure : public ::testing::Test {
 protected:
  void SetUp() override {
    chirp_connect_backend::backend_client_.reset(new BackendClientDebug());

    // Set up users to be used in the sub-tests
    for (size_t i = 0; i < kNumOfUsersTotal; ++i) {
      user_list_.push_back(std::string("user") + std::to_string(i));
      // some users should be registered in advance
      if (i < kNumOfUsersPreset) {
        service_data_structure_.UserRegister(user_list_[i]);
      }
    }

    // Set up expected contents for initial posts
    for (size_t i = 0; i < kNumOfChirps; ++i) {
      chirps_content.push_back(std::string("Chirp #") + std::to_string(i) +
                               kShortText);
    }

    // Set up expected contents for posts after editing
    chirps_content_after_edit = chirps_content;
    for (size_t i = 0; i < kNumOfChirps; ++i) {
      chirps_content_after_edit[i] =
          std::string("Chirp #") + std::to_string(i) + kLongText;
    }

    // Set up expected contents for posts after deleting
    chirps_content_after_delete = chirps_content;
    for (size_t i = 0; i < kHalfNumOfChirps; ++i) {
      chirps_content_after_delete.erase(chirps_content_after_delete.begin());
    }
  }

  // The container to store the usernames
  std::vector<std::string> user_list_;

  // The container to store expected contents of chirps
  std::vector<std::string> chirps_content;
  std::vector<std::string> chirps_content_after_edit;
  std::vector<std::string> chirps_content_after_delete;

  // The object that this test is going to test
  ServiceDataStructure service_data_structure_;
};

// This tests on the `UserRegister()` when users already existed
TEST_F(ServiceTestDataStructure, UserRegisterExistedUsers) {
  // Try to register existed usernames which already done in the `SetUp()`
  for (size_t i = 0; i < kNumOfUsersPreset; ++i) {
    // ServiceDataStructure::ReturnCodes
    auto ret = service_data_structure_.UserRegister(user_list_[i]);
    // This should fail since the username specified has already been registered
    // in the `SetUp` process above
    EXPECT_EQ(ServiceDataStructure::USER_EXISTS, ret);
  }
}

// This tests on the `UserRegister()` registering new users
TEST_F(ServiceTestDataStructure, UserRegisterNewUsers) {
  // Try to register non-existed usernames
  for (size_t i = kNumOfUsersPreset; i < kNumOfUsersTotal; ++i) {
    // ServiceDataStructure::ReturnCodes
    auto ret = service_data_structure_.UserRegister(user_list_[i]);
    // This should succeed since the username specified is not registered
    EXPECT_EQ(ServiceDataStructure::OK, ret);
  }
}

// This tests on the `UserLogin()` logging in registered users
TEST_F(ServiceTestDataStructure, UserLoginExisted) {
  // Try to login to those usernames that have been registered
  for (size_t i = 0; i < kNumOfUsersPreset; ++i) {
    // std::unique_ptr<ServiceDataStructure::UserSession>
    auto session_1 = service_data_structure_.UserLogin(user_list_[i]);
    // std::unique_ptr<ServiceDataStructure::UserSession>
    auto session_2 = service_data_structure_.UserLogin(user_list_[i]);
    EXPECT_NE(nullptr, session_1);
    EXPECT_NE(nullptr, session_2);
    // Multiple logins are allowed
    EXPECT_NE(session_1, session_2);

    // Check username is identical
    EXPECT_EQ(user_list_[i], session_1->SessionGetUsername());
    EXPECT_EQ(user_list_[i], session_2->SessionGetUsername());
  }
}

// This tests on the `UserLogin()` logging in a non-existing username
TEST_F(ServiceTestDataStructure, UserLoginNonexisted) {
  // Try to login to a non-existing username
  // std::unique_ptr<ServiceDataStructure::UserSession>
  auto session = service_data_structure_.UserLogin("nonexist");
  EXPECT_EQ(nullptr, session);
}

// This tests on the `PostChirp()`
TEST_F(ServiceTestDataStructure, ChirpPost) {
  // Tests for every user
  for (size_t i = 0; i < kNumOfUsersPreset; ++i) {
    // std::unique_ptr<ServiceDataStructure::UserSession>
    auto session = service_data_structure_.UserLogin(user_list_[i]);
    std::vector<uint64_t> chirp_ids;

    // Login should be successful
    ASSERT_NE(nullptr, session);
    // Check the user_session is not broken
    EXPECT_EQ(user_list_[i], session->SessionGetUsername());

    // Test Posting
    uint64_t parent_id = 0;
    for (size_t j = 0; j < kNumOfChirps; ++j) {
      uint64_t chirp_id;
      auto ret = session->PostChirp(chirps_content[j], &chirp_id, parent_id);
      // Posting should be successful
      ASSERT_EQ(ServiceDataStructure::OK, ret);
      parent_id = chirp_id;

      // collect the chirp ids created
      chirp_ids.push_back(chirp_id);
    }

    // Read from backend
    // to see if the results are identical
    std::vector<std::string> chirps_content_from_backend;
    uint64_t last_id = 0;
    for (const auto &id : session->SessionGetUserChirpList()) {
      struct ServiceDataStructure::Chirp chirp;
      // ServiceDataStructure::ReturnCodes
      auto ret = service_data_structure_.ReadChirp(id, &chirp);
      // Reading should be successful
      ASSERT_EQ(ServiceDataStructure::OK, ret);
      EXPECT_EQ(last_id, chirp.get_parent_id());
      last_id = chirp.get_id();

      // colect the texts posted
      chirps_content_from_backend.push_back(chirp.get_text());
    }
    // Check if the contents are identical
    EXPECT_EQ(chirps_content, chirps_content_from_backend);
  }
}

// This tests on the `EditChirp()`
TEST_F(ServiceTestDataStructure, ChirpEdit) {
  // Tests for every user
  for (size_t i = 0; i < kNumOfUsersPreset; ++i) {
    // std::unique_ptr<ServiceDataStructure::UserSession>
    auto session = service_data_structure_.UserLogin(user_list_[i]);
    std::vector<uint64_t> chirp_ids;

    // Login should be successful
    ASSERT_NE(nullptr, session);
    // Check the user_session is not broken
    EXPECT_EQ(user_list_[i], session->SessionGetUsername());

    // No need to test posting
    // So assume all posting will be successful
    // These postings are just setting up for editing
    for (size_t j = 0; j < kNumOfChirps; ++j) {
      uint64_t chirp_id;
      auto ret = session->PostChirp(chirps_content[j], &chirp_id, 0);
      // Posting should be successful
      ASSERT_EQ(ServiceDataStructure::OK, ret);

      // collect the chirp ids created
      chirp_ids.push_back(chirp_id);
    }

    // Test Editing
    std::vector<std::string> chirps_content_from_backend;
    for (size_t j = 0; j < chirp_ids.size(); ++j) {
      // ServiceDataStructure::ReturnCodes
      auto ret = session->EditChirp(chirp_ids[j], chirps_content_after_edit[j]);
      // Editing should be successful
      ASSERT_EQ(ServiceDataStructure::OK, ret);
    }

    // Read from backend
    // to see if the results are identical
    for (const auto &id : session->SessionGetUserChirpList()) {
      struct ServiceDataStructure::Chirp chirp;
      // ServiceDataStructure::ReturnCodes
      auto ret = service_data_structure_.ReadChirp(id, &chirp);
      // Reading should be successful
      ASSERT_EQ(ServiceDataStructure::OK, ret);

      // colect the texts posted
      chirps_content_from_backend.push_back(chirp.get_text());
    }
    // Check if the contents are identical
    EXPECT_EQ(chirps_content_after_edit, chirps_content_from_backend);
  }
}

// This tests on the `DeleteChirp()`
TEST_F(ServiceTestDataStructure, ChirpDelete) {
  // Tests for every user
  for (size_t i = 0; i < kNumOfUsersPreset; ++i) {
    // std::unique_ptr<ServiceDataStructure::UserSession>
    auto session = service_data_structure_.UserLogin(user_list_[i]);
    std::vector<uint64_t> chirp_ids;

    // Login should be successful
    ASSERT_NE(nullptr, session);
    // Check the user_session is not broken
    EXPECT_EQ(user_list_[i], session->SessionGetUsername());

    // No need to test posting
    // So assume all posting will be successful
    // These postings are just setting up for deleting
    for (size_t j = 0; j < kNumOfChirps; ++j) {
      uint64_t chirp_id;
      auto ret = session->PostChirp(chirps_content[j], &chirp_id, 0);
      // Posting should be successful
      ASSERT_EQ(ServiceDataStructure::OK, ret);

      // collect the chirp ids created
      chirp_ids.push_back(chirp_id);
    }

    // Test Deleting
    std::vector<std::string> chirps_content_from_backend;
    for (size_t j = 0; j < kHalfNumOfChirps; ++j) {
      // ServiceDataStructure::ReturnCodes
      auto ret = session->DeleteChirp(chirp_ids[j]);
      // Deleting should be successful
      ASSERT_EQ(ServiceDataStructure::OK, ret);
      ret = session->DeleteChirp(chirp_ids[j]);
      // Deleting should be unsuccessful since this chirp is already deleted
      EXPECT_EQ(ServiceDataStructure::CHIRP_ID_NOT_FOUND, ret);
    }

    // Read from backend
    // to see if the results are identical
    for (const auto &id : session->SessionGetUserChirpList()) {
      struct ServiceDataStructure::Chirp chirp;
      // ServiceDataStructure::ReturnCodes
      auto ret = service_data_structure_.ReadChirp(id, &chirp);
      // Reading should be successful
      ASSERT_EQ(ServiceDataStructure::OK, ret);

      // colect the texts posted
      chirps_content_from_backend.push_back(chirp.get_text());
    }
    // Check if the contents are identical
    EXPECT_EQ(chirps_content_after_delete, chirps_content_from_backend);
  }
}

TEST_F(ServiceTestDataStructure, FollowUsers) {
  // Make each user follows the next user
  for (size_t i = 0; i < kNumOfUsersPreset; ++i) {
    // std::unique_ptr<ServiceDataStructure::UserSession>
    auto session = service_data_structure_.UserLogin(user_list_[i]);
    // Login should be successful
    ASSERT_NE(nullptr, session);
    // ServiceDataStructure::ReturnCodes
    auto ret = session->Follow(user_list_[(i + 1) % kNumOfUsersPreset]);
    // Following should be successful
    EXPECT_EQ(ServiceDataStructure::OK, ret);
  }
}

TEST_F(ServiceTestDataStructure, FollowNonexisted) {
  auto session = service_data_structure_.UserLogin(user_list_.front());
  // Login should be successful
  ASSERT_NE(nullptr, session);
  // ServiceDataStructure::ReturnCodes
  auto ret = session->Follow("non-existed");
  // Should not follow a non-existed user
  EXPECT_EQ(ServiceDataStructure::FOLLOWEE_NOT_FOUND, ret);
}

TEST_F(ServiceTestDataStructure, Monitor) {
  // Each user monitors their following users
  for (size_t i = 0; i < kNumOfUsersPreset; ++i) {
    // std::unique_ptr<ServiceDataStructure::UserSession>
    auto session = service_data_structure_.UserLogin(user_list_[i]);
    // Login should be successful
    ASSERT_NE(nullptr, session);
    auto ret = session->Follow(user_list_[(i + 1) % kNumOfUsersPreset]);
    // Following should be successful
    ASSERT_EQ(ServiceDataStructure::OK, ret);

    // std::unique_ptr<ServiceDataStructure::UserSession>
    auto session_user_followed = service_data_structure_.UserLogin(
        user_list_[(i + 1) % kNumOfUsersPreset]);
    // Login as the followed user should be successful
    ASSERT_NE(nullptr, session_user_followed);

    // Post some dont-care chirps from the followed user
    for (size_t j = 0; j < 5; ++j) {
      auto ret = session_user_followed->PostChirp(kShortText, nullptr);
      // Posting should be successful
      EXPECT_EQ(ServiceDataStructure::OK, ret);
    }

    // Timestamp the current time and back it up
    struct timeval now;
    gettimeofday(&now, nullptr);
    struct timeval backup_now = now;

    // Collect the chirp ids after the above timestamp
    std::set<uint64_t> chirp_collector;
    for (size_t j = 0; j < 5; ++j) {
      uint64_t chirp_id;
      auto ret = session_user_followed->PostChirp(kShortText, &chirp_id);
      // Posting should be successful
      ASSERT_EQ(ServiceDataStructure::OK, ret);
      chirp_collector.insert(chirp_id);
    }

    // Test Monitoring from
    auto monitor_result = session->MonitorFrom(&now);
    // `now` should be modified by the `Monitor` function
    // so it should be different from the one I have backed up
    EXPECT_TRUE(timercmp(&backup_now, &now, !=));
    // Check if the contents are identical
    EXPECT_EQ(chirp_collector, monitor_result);
  }
}

// TODO: Not sure whether I should keep the following tests, so make it disabled
// for now This test cases on the Service Server to check whether their
// interfaces work correctly.
class DISABLED_ServiceTestServer : public ::testing::Test {
 protected:
  void SetUp() override {
    chirp_connect_backend::backend_client_.reset(new BackendClientDebug());

    // Set up users to be used in the sub-tests
    for (size_t i = 0; i < kNumOfUsersTotal; ++i) {
      user_list_.push_back(std::string("User") + std::to_string(i));

      if (i < kNumOfUsersPreset) {
        service_client_.SendRegisterUserRequest(user_list_[i]);
      }
    }
  }

  std::vector<std::string> user_list_;
  ServiceClient service_client_;
};

// This tests on `registeruser` in Service Server
TEST_F(DISABLED_ServiceTestServer, RegisterUser) {
  // Try to register existed usernames
  for (size_t i = 0; i < kNumOfUsersPreset; ++i) {
    // ServiceClient::ReturnCodes
    auto ret = service_client_.SendRegisterUserRequest(user_list_[i]);
    // This should fail since the username specified has already been registered
    // in the `SetUp` process above
    EXPECT_EQ(ServiceClient::USER_EXISTS, ret)
        << "This should fail since the username specified has been registered.";
  }

  // Try to register non-existed usernames
  for (size_t i = kNumOfUsersPreset; i < kNumOfUsersTotal; ++i) {
    // ServiceClient::ReturnCodes
    auto ret = service_client_.SendRegisterUserRequest(user_list_[i]);
    // This should succeed since the username specified is not registered
    EXPECT_EQ(ServiceClient::OK, ret) << "This registration should succeed.";
  }

  // Try to register all usernames again
  for (size_t i = 0; i < kNumOfUsersTotal; ++i) {
    // ServiceClient::ReturnCodes
    auto ret = service_client_.SendRegisterUserRequest(user_list_[i]);
    // This should fail since all these usernames are already registered
    EXPECT_EQ(ServiceClient::USER_EXISTS, ret)
        << "This should fail since the username specified has been registered.";
  }
}

// This tests on `chirp` in Service Server
TEST_F(DISABLED_ServiceTestServer, Chirp) {
  // Every user posts a chirp
  uint64_t last_id = 0;
  for (size_t i = 0; i < kNumOfUsersTotal; ++i) {
    struct ServiceClient::Chirp chirp;
    // ServiceClient::ReturnCodes
    auto ret = service_client_.SendChirpRequest(user_list_[i], kShortText,
                                                last_id, &chirp);
    EXPECT_EQ(ServiceClient::OK, ret);
    // The three lines below check if the data in a chirp data structure
    // corresponds to the data provided above
    EXPECT_EQ(user_list_[i], chirp.username);
    EXPECT_EQ(kShortText, chirp.text);
    EXPECT_EQ(last_id, chirp.parent_id);
    last_id = chirp.id;
  }
}

// This tests on `follow` in Service Server
TEST_F(DISABLED_ServiceTestServer, Follow) {
  // Every user follows its next user
  for (size_t i = 0; i < kNumOfUsersTotal; ++i) {
    // ServiceClient::ReturnCodes
    auto ret = service_client_.SendFollowRequest(
        user_list_[i], user_list_[(i + 1) % kNumOfUsersTotal]);
    // This should be successful
    EXPECT_EQ(ServiceClient::OK, ret);
    // This should be unsuccessful since it tries to follow a non-existed user
    ret = service_client_.SendFollowRequest(user_list_[i], "non-existed");
    EXPECT_EQ(ServiceClient::FOLLOWEE_NOT_FOUND, ret);
  }
}

// This tests on `read` in Service Server
TEST_F(DISABLED_ServiceTestServer, Read) {
  // The container of chirp ids
  std::vector<uint64_t> corrected_chirps;

  for (size_t i = 0; i < kNumOfUsersTotal; ++i) {
    corrected_chirps.clear();
    ServiceClient::Chirp chirp;

    // In the following three layers, I try to build a nested chirps like this
    // #01 - #02 - #05
    //           - #06
    //           - #07
    //     - #03
    //     - #04

    // Layer 1
    // ServiceClient::ReturnCodes
    auto ret =
        service_client_.SendChirpRequest(user_list_[i], kShortText, 0, &chirp);
    // Posting a chirp should be successful
    EXPECT_EQ(ServiceClient::OK, ret);
    corrected_chirps.push_back(chirp.id);

    // Layer 2
    for (size_t j = 0; j < 3; ++j) {
      ret = service_client_.SendChirpRequest(user_list_[i], kShortText,
                                             corrected_chirps[0], &chirp);
      // Posting a chirp should be successful
      EXPECT_EQ(ServiceClient::OK, ret);
      corrected_chirps.push_back(chirp.id);
    }

    // Layer 3
    std::vector<uint64_t> tmp;
    for (size_t j = 0; j < 3; ++j) {
      ret = service_client_.SendChirpRequest(user_list_[i], kShortText,
                                             corrected_chirps[1], &chirp);
      // Posting a chirp should be successful
      EXPECT_EQ(ServiceClient::OK, ret);
      tmp.push_back(chirp.id);
    }
    corrected_chirps.insert(corrected_chirps.begin() + 2, tmp.begin(),
                            tmp.end());

    // Prepare to read
    std::vector<struct ServiceClient::Chirp> reply;
    ret = service_client_.SendReadRequest(corrected_chirps[0], &reply);
    // Reading should be successful
    EXPECT_EQ(ServiceClient::OK, ret);
    // Check if the ids from reading and the ids in the container are identical
    for (size_t j = 0; j < corrected_chirps.size(); ++j) {
      EXPECT_EQ(user_list_[i], reply[j].username);
      EXPECT_EQ(corrected_chirps[j], reply[j].id);
    }
  }
}

// This tests on `monitor` in Service Server
TEST_F(DISABLED_ServiceTestServer, Monitor) {
  // Make the last user to follow all other users
  for (size_t i = 0; i < kNumOfUsersTotal - 1; ++i) {
    // Don't care about the return value of this
    service_client_.SendFollowRequest(user_list_.back(), user_list_[i]);
  }

  // Container of chirp ids
  std::vector<uint64_t> chirpids;

  // Keep posting chirps in another thread
  std::thread posting_chirps([&]() {
    // Every user takes turn to post
    for (size_t i = 0; i < kNumOfUsersTotal - 1; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      ServiceClient::Chirp chirp;
      // ServiceClient::ReturnCodes
      auto ret = service_client_.SendChirpRequest(user_list_[i], kShortText, 0,
                                                  &chirp);
      // Posting chirps should be successful
      EXPECT_EQ(ServiceClient::OK, ret);
      chirpids.push_back(chirp.id);
    }
  });

  // Send `monitor` request simultaneously
  std::vector<ServiceClient::Chirp> chirps;
  service_client_.SendMonitorRequest(user_list_.back(), &chirps);

  // Wait for the thread to finish
  posting_chirps.join();

  // Check if the results are identical
  for (size_t i = 0; i < chirpids.size(); ++i) {
    EXPECT_EQ(chirpids[i], chirps[i].id) << " i is " << i << std::endl;
  }
}

}  // end of namespace

GTEST_API_ int main(int argc, char **argv) {
  // glog initialization
  FLAGS_log_dir = "./log";
  google::InitGoogleLogging(argv[0]);

  LOG(INFO) << "glog on testing starts.";

  std::cout << "Running service tests from " << __FILE__ << std::endl;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
