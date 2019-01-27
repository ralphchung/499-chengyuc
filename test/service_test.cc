#include <iostream>
#include <memory>
#include <string>
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
        service_data_structure_.UserRegister(user_list_[i]);
      }
    }
  }

  std::vector<std::string> user_list_;
  ServiceDataStructure service_data_structure_;
};

TEST_F(ServiceTest, UserTestAndPostTest) {
  for(size_t i = 0; i < kNumOfUsersTotal; ++i) {
    bool ok = service_data_structure_.UserRegister(user_list_[i]);
    // The first `kNumOfUsersPreset` users should have already registered
    if (i < kNumOfUsersPreset) {
      EXPECT_EQ(ok, false) << "i is " << i << std::endl;
    }
    else {
      EXPECT_EQ(ok, true) << "i is " << i << std::endl;
    }
  }

  for(size_t i = 0; i < kNumOfUsersTotal; ++i) {
    // Login once
    ServiceDataStructure::UserSession *session_1 = service_data_structure_.UserLogin(user_list_[i]);
    EXPECT_NE(session_1, nullptr);
    // Login twice since multiple logins is allowed
    ServiceDataStructure::UserSession *session_2 = service_data_structure_.UserLogin(user_list_[i]);
    EXPECT_NE(session_2, nullptr);
    EXPECT_NE(session_1, session_2);

    service_data_structure_.UserLogout(session_1);
    session_1 = nullptr;

    bool ok = session_2->Follow("nonexisted");
    // The above operation should fail since it follows an nonexisted user.
    EXPECT_EQ(ok, false);
    ok = session_2->Follow(user_list_[(i + 5) % kNumOfUsersTotal]);
    // This time the operation should succeed
    EXPECT_EQ(ok, true);

    struct ServiceDataStructure::Chirp *chirp_1 = session_2->PostChirp(kShortText);
    // The post operation should succeed
    EXPECT_NE(chirp_1, nullptr);
    struct ServiceDataStructure::Chirp *chirp_2 = session_2->PostChirp(kShortText);
    // The post operation should succeed
    EXPECT_NE(chirp_2, nullptr);
    struct ServiceDataStructure::Chirp *chirp_1_edit = session_2->EditChirp(chirp_1->id, kLongText);
    // The edit operation should succeed and
    // its operation should modify the data in place instead of creating a new chirp
    EXPECT_EQ(chirp_1, chirp_1_edit);
    EXPECT_EQ(session_2->get_chirp_list().size(), 2);
  
    struct ServiceDataStructure::Chirp *chirp_3 = session_2->PostChirp(kLongText);
    // The post operation should succeed
    EXPECT_NE(chirp_3, nullptr);

    ok = session_2->DeleteChirp(chirp_1->id);
    // The delete operation should succeed
    EXPECT_EQ(ok, true);
    ok = session_2->DeleteChirp(chirp_1->id);
    // The delete operation should fail since the same chirp cannot be deleted twice
    EXPECT_EQ(ok, false);
    
    // Now two chirps should remain
    EXPECT_EQ(session_2->get_chirp_list().size(), 2);

    // Now we should be able to find `chirp_1` in the chirp list
    EXPECT_NE(session_2->get_chirp_list().find(chirp_2->id), session_2->get_chirp_list().end());
    // Now we should be able to find `chirp_3` in the chirp list
    EXPECT_NE(session_2->get_chirp_list().find(chirp_3->id), session_2->get_chirp_list().end());
    
    // The text in the chirp we find should corresponds to the text we have posted
    EXPECT_EQ(service_data_structure_.ReadChirp(chirp_2->id)->text, kShortText);
    EXPECT_EQ(service_data_structure_.ReadChirp(chirp_3->id)->text, kLongText);
  }
}

} // end of namespace

GTEST_API_ int main(int argc, char **argv) {
  std::cout << "Running service tests from " << __FILE__ << std::endl;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
