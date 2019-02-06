#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <sys/time.h>
#include <thread>
#include <vector>

#include <glog/logging.h>
#include "gtest/gtest.h"

#include "service_client_lib.h"
#include "service_data_structure.h"

namespace {

uint64_t PrintId(const std::string &id) {
  return *(reinterpret_cast<const uint64_t*>(id.c_str()));
}

const size_t kNumOfUsersPreset = 10;
const size_t kNumOfUsersTotal = 20;
const char *kShortText = "short";
const char *kLongText = "longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong";

class ServiceTestDataStructure : public ::testing::Test {
 protected:
  void SetUp() override {
    // Set up users
    for(size_t i = 0; i < kNumOfUsersTotal; ++i) {
      user_list_.push_back(std::string("user") + std::to_string(i));
      if (i < kNumOfUsersPreset) {
        bool ok = service_data_structure_.UserRegister(user_list_[i]);
      }
    }
  }

  std::vector<std::string> user_list_;
  ServiceDataStructure service_data_structure_;
};

TEST_F(ServiceTestDataStructure, UserRegisterAndLoginTest) {
  // Try to register existed usernames
  for(size_t i = 0; i < kNumOfUsersPreset; ++i) {
    bool ok = service_data_structure_.UserRegister(user_list_[i]);
    // This should fail since the username specified has already been registered in the `SetUp` process above
    EXPECT_FALSE(ok);
  }

  // Try to register non-existed usernames
  for(size_t i = kNumOfUsersPreset; i < kNumOfUsersTotal; ++i) {
    bool ok = service_data_structure_.UserRegister(user_list_[i]);
    // This should succeed since the username specified is not registered
    EXPECT_TRUE(ok);
  }

  // Try to login to those usernames that have been registered
  for(size_t i = 0; i < kNumOfUsersTotal; ++i) {
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

  // Try to login to a non-existing username
  // std::unique_ptr<ServiceDataStructure::UserSession>
  auto session = service_data_structure_.UserLogin("nonexist");
  EXPECT_EQ(nullptr, session);
}

TEST_F(ServiceTestDataStructure, PostEditAndDeleteTest) {
  const size_t kTestCase = 10;
  const size_t kHalfTestCase = kTestCase / 2;

  // Set up expected contents for initial posts
  std::vector<std::string> chirps_content;
  for(size_t i = 0; i < kTestCase; ++i) {
    chirps_content.push_back(std::string("Chirp #") + std::to_string(i) + kShortText);
  }

  // Set up expected contents for posts after editing
  std::vector<std::string> chirps_content_after_edit(chirps_content);
  for(size_t i = 0; i < kTestCase; ++i) {
    chirps_content_after_edit[i] = std::string("Chirp #") + std::to_string(i) + kLongText;
  }

  // Set up expected contents for posts after deleting
  std::vector<std::string> chirps_content_after_delete(chirps_content_after_edit);
  for(size_t i = 0; i < kHalfTestCase; ++i) {
    chirps_content_after_delete.erase(chirps_content_after_delete.begin());
  }

  // Tests for every user
  for(size_t i = 0; i < kNumOfUsersPreset; ++i) {
    // std::unique_ptr<ServiceDataStructure::UserSession>
    auto session = service_data_structure_.UserLogin(user_list_[i]);
    std::vector<uint64_t> chirp_ids;

    ASSERT_NE(nullptr, session);

    EXPECT_EQ(user_list_[i], session->SessionGetUsername());

    // Test Posting
    uint64_t parent_id = 0;
    for(size_t j = 0; j < kTestCase; ++j) {
      uint64_t chirp_id = session->PostChirp(chirps_content[j], parent_id);
      // Posting is successful
      ASSERT_NE(0, chirp_id);
      parent_id = chirp_id;
      chirp_ids.push_back(chirp_id);
    }

    // Read from backend
    // to see if the results are identical
    std::vector<std::string> chirps_content_from_backend;
    uint64_t last_id = 0;
    for(const auto& id : session->SessionGetUserChirpList()) {
      struct ServiceDataStructure::Chirp chirp;
      bool ok = service_data_structure_.ReadChirp(id, &chirp);
      // Reading is successful
      ASSERT_TRUE(ok);
      EXPECT_EQ(last_id, chirp.parent_id);
      last_id = chirp.id;
      chirps_content_from_backend.push_back(chirp.text);
    }
    // Check if the contents are identical
    EXPECT_EQ(chirps_content, chirps_content_from_backend); 

    // Test Editing
    chirps_content_from_backend.clear();
    for(size_t j = 0; j < chirp_ids.size(); ++j) {
      auto ok = session->EditChirp(chirp_ids[j], chirps_content_after_edit[j]);
      // Editing is successful
      ASSERT_TRUE(ok);
    }

    // Read from backend
    // to see if the results are identical
    for(auto& id : session->SessionGetUserChirpList()) {
      struct ServiceDataStructure::Chirp chirp;
      bool ok = service_data_structure_.ReadChirp(id, &chirp);
      ASSERT_TRUE(ok);
      chirps_content_from_backend.push_back(chirp.text);
    }
    // Check if the contents are identical
    EXPECT_EQ(chirps_content_after_edit, chirps_content_from_backend);

    // Test Deleting
    chirps_content_from_backend.clear();
    for(size_t j = 0; j < kHalfTestCase; ++j) {
      bool ok = session->DeleteChirp(chirp_ids[j]);
      // Deleting is successful
      EXPECT_TRUE(ok);
      ok = session->DeleteChirp(chirp_ids[j]);
      // Deleting is unsuccessful since it is already deleted
      EXPECT_FALSE(ok);
    }

    // Read from backend
    // to see if the results are identical
    for(auto& id : session->SessionGetUserChirpList()) {
      struct ServiceDataStructure::Chirp chirp;
      bool ok = service_data_structure_.ReadChirp(id, &chirp);
      // Reading is successful
      ASSERT_TRUE(ok);
      chirps_content_from_backend.push_back(chirp.text);
    }
    // Check if the contents are identical
    EXPECT_EQ(chirps_content_after_delete, chirps_content_from_backend);
  }
}

TEST_F(ServiceTestDataStructure, FollowAndMonitorTest) {
  // Each user follows the next user
  for(size_t i = 0; i < kNumOfUsersPreset; ++i) {
    // std::unique_ptr<ServiceDataStructure::UserSession>
    auto session = service_data_structure_.UserLogin(user_list_[i]);
    // Login is successful
    EXPECT_NE(nullptr, session);
    bool ok = session->Follow(user_list_[(i + 1) % kNumOfUsersPreset]);
    // Following is successful
    EXPECT_TRUE(ok);
  }

  // Each user monitors their following users
  for(size_t i = 0; i < kNumOfUsersPreset; ++i) {
    // std::unique_ptr<ServiceDataStructure::UserSession>
    auto session_user_followed = service_data_structure_.UserLogin(user_list_[(i + 1) % kNumOfUsersPreset]);
    // Login as the followed user successful
    EXPECT_NE(nullptr, session_user_followed);

    // Post some dont-care chirps from the followed user
    for(size_t j = 0; j < 5; ++j) {
      uint64_t chirp_id = session_user_followed->PostChirp(kShortText);
      // Posting successful
      EXPECT_NE(0, chirp_id);
    }

    // Timestamp the current time and back it up
    struct timeval now;
    gettimeofday(&now, nullptr);
    struct timeval backup_now = now;

    // sleep a little while to ensure that the time has passed at least 1 usec.
    std::this_thread::sleep_for(std::chrono::microseconds(1));

    // Collect the chirp ids after the above timestamp
    std::set<uint64_t> chirp_collector;
    for(size_t j = 0; j < 5; ++j) {
      auto chirp_id = session_user_followed->PostChirp(kShortText);
      // Posting is successful
      EXPECT_NE(0, chirp_id);
      chirp_collector.insert(chirp_id);
    }

    // Test Monitoring
    // std::unique_ptr<ServiceDataStructure::UserSession>
    auto session = service_data_structure_.UserLogin(user_list_[i]);
    // Login is successful
    EXPECT_NE(nullptr, session);
    auto monitor_result = session->MonitorFrom(&now);
    // `now` should be modified by the `Monitor` function
    EXPECT_TRUE(timercmp(&backup_now, &now, !=));
    // Check if the contents are identical
    EXPECT_EQ(chirp_collector, monitor_result);
  }
}

class ServiceTestServer : public ::testing::Test {
 protected:
  void SetUp() override {
    // Set up users list
    for (size_t i = 0; i < kNumOfUsersTotal; ++i) {
      user_list_.push_back(std::string("User") + std::to_string(i));

      if (i < kNumOfUsersPreset) {
        bool ok = service_client_.SendRegisterUserRequest(user_list_[i]);
      }
    }
  }

  std::vector<std::string> user_list_;
  ServiceClient service_client_;
};

TEST_F(ServiceTestServer, RegisterUser) {
  // Try to register existed usernames
  for(size_t i = 0; i < kNumOfUsersPreset; ++i) {
    bool ok = service_client_.SendRegisterUserRequest(user_list_[i]);
    // This should fail since the username specified has already been registered in the `SetUp` process above
    EXPECT_FALSE(ok) << "This should fail since the username specified has been registered.";
  }

  // Try to register non-existed usernames
  for(size_t i = kNumOfUsersPreset; i < kNumOfUsersTotal; ++i) {
    bool ok = service_client_.SendRegisterUserRequest(user_list_[i]);
    // This should succeed since the username specified is not registered
    EXPECT_TRUE(ok) << "This registration should succeed.";
  }

  // Try to register all usernames again
  for(size_t i = 0; i < kNumOfUsersTotal; ++i) {
    bool ok = service_client_.SendRegisterUserRequest(user_list_[i]);
    EXPECT_FALSE(ok) << "This should fail since the username specified has been registered.";
  }
}

TEST_F(ServiceTestServer, Chirp) {
  // Every user posts a chirp
  uint64_t last_id = 0;
  for (size_t i = 0; i < kNumOfUsersTotal; ++i) {
    struct ServiceClient::Chirp chirp;
    bool ok = service_client_.SendChirpRequest(user_list_[i], kShortText, last_id, &chirp);
    EXPECT_EQ(user_list_[i], chirp.username);
    EXPECT_EQ(kShortText, chirp.text);
    EXPECT_EQ(last_id, chirp.parent_id);
    last_id = chirp.id;
  }
}

TEST_F(ServiceTestServer, Follow) {
  for(size_t i = 0; i < kNumOfUsersTotal; ++i) {
    bool ok = service_client_.SendFollowRequest(user_list_[i], user_list_[(i + 1) % kNumOfUsersTotal]);
    EXPECT_TRUE(ok);
    ok = service_client_.SendFollowRequest(user_list_[i], "non-existed");
    EXPECT_FALSE(ok);
  }
}

TEST_F(ServiceTestServer, Read) {
  std::vector<uint64_t> corrected_chirps;
  for(size_t i = 0; i < 1; ++i) {
    corrected_chirps.clear();
    ServiceClient::Chirp chirp;

    // Layer 1
    bool ok = service_client_.SendChirpRequest(user_list_[i], kShortText, 0, &chirp);
    EXPECT_TRUE(ok);
    corrected_chirps.push_back(chirp.id);

    // Layer 2
    for (size_t j = 0; j < 3; ++j) {
      ok = service_client_.SendChirpRequest(user_list_[i], kShortText, corrected_chirps[0], &chirp);
      EXPECT_TRUE(ok);
      corrected_chirps.push_back(chirp.id);
    }

    // Layer 3
    std::vector<uint64_t> tmp;
    for(size_t j = 0; j < 3; ++j) {
      ok = service_client_.SendChirpRequest(user_list_[i], kShortText, corrected_chirps[1], &chirp);
      EXPECT_TRUE(ok);
      tmp.push_back(chirp.id);
    }
    corrected_chirps.insert(corrected_chirps.begin() + 2, tmp.begin(), tmp.end());

    std::vector<struct ServiceClient::Chirp> reply;
    ok = service_client_.SendReadRequest(corrected_chirps[0], &reply);
    EXPECT_TRUE(ok);
    for(size_t j = 0; j < corrected_chirps.size(); ++j) {
      EXPECT_EQ(user_list_[i], reply[j].username);
      EXPECT_EQ(corrected_chirps[j], reply[j].id);
    }
  }
}

TEST_F(ServiceTestServer, Monitor) {
  // Make the last user to follow all other users
  for(size_t i = 0; i < kNumOfUsersTotal - 1; ++i) {
    // Don't care about the return value of this
    service_client_.SendFollowRequest(user_list_.back(), user_list_[i]);
  }

  std::vector<uint64_t> chirpids;
  std::thread posting_chirps([&]() {
    for(size_t i = 0; i < kNumOfUsersTotal - 1; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      ServiceClient::Chirp chirp;
      service_client_.SendChirpRequest(user_list_[i], kShortText, 0, &chirp);
      chirpids.push_back(chirp.id);
    }
  });

  std::vector<ServiceClient::Chirp> chirps;
  service_client_.SendMonitorRequest(user_list_.back(), &chirps);

  posting_chirps.join();

  for(size_t i = 0; i < chirpids.size(); ++i) {
    EXPECT_EQ(chirpids[i], chirps[i].id) << " i is " << i << std::endl;
  }
}

} // end of namespace

GTEST_API_ int main(int argc, char **argv) {
  // glog initialization
  FLAGS_log_dir = "./log";
  google::InitGoogleLogging(argv[0]);

  LOG(INFO) << "glog on testing starts.";

  std::cout << "Running service tests from " << __FILE__ << std::endl;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
