#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <sys/time.h>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "service_data_structure.h"

namespace {

std::string PrintId(const std::string &id) {
  std::string ret;
  for(char c : id) {
    ret += std::to_string(int(c));
    ret += ' ';
  }
}

const size_t kNumOfUsersPreset = 10;
const size_t kNumOfUsersTotal = 20;
const char *kShortText = "short";
const char *kLongText = "longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong";

class ServiceTest : public ::testing::Test {
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

TEST_F(ServiceTest, UserRegisterAndLoginTest) {
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
  }

  // Try to login to a non-existing username
  // std::unique_ptr<ServiceDataStructure::UserSession>
  auto session = service_data_structure_.UserLogin("nonexist");
  EXPECT_EQ(nullptr, session);
}

TEST_F(ServiceTest, PostEditAndDeleteTest) {
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
    std::vector<std::string> chirp_ids;

    ASSERT_NE(nullptr, session);

    // Test Posting
    std::string parent_id("");
    for(size_t j = 0; j < kTestCase; ++j) {
      std::string chirp_id = session->PostChirp(chirps_content[j], parent_id);
      // Posting is successful
      ASSERT_FALSE(chirp_id.empty());
      parent_id = chirp_id;
      chirp_ids.push_back(chirp_id);
    }

    // Read from backend
    // to see if the results are identical
    std::vector<std::string> chirps_content_from_backend;
    std::string last_id("");
    for(auto& id : session->SessionGetUserChirpList()) {
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

TEST_F(ServiceTest, FollowAndMonitorTest) {
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
      std::string chirp_id = session_user_followed->PostChirp(kShortText);
      // Posting successful
      EXPECT_FALSE(chirp_id.empty());
    }

    // Timestamp the current time and back it up
    struct timeval now;
    gettimeofday(&now, nullptr);
    struct timeval backup_now = now;

    // sleep a little while to ensure that the time has passed at least 1 usec.
    std::this_thread::sleep_for(std::chrono::microseconds(1));

    // Collect the chirp ids after the above timestamp
    std::set<std::string> chirp_collector;
    for(size_t j = 0; j < 5; ++j) {
      auto chirp_id = session_user_followed->PostChirp(kShortText);
      // Posting is successful
      EXPECT_FALSE(chirp_id.empty());
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

} // end of namespace

GTEST_API_ int main(int argc, char **argv) {
  std::cout << "Running service tests from " << __FILE__ << std::endl;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
