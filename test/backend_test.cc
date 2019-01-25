
#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "backend_client_lib.h"
#include "backend_server.h"

namespace {

#define NUM_OF_PAIRS 20

// Setup the same data for multiple tests
// This setup generates 20 keys and its corresponding correct values
class BackendTest : public ::testing::Test {
 protected:
  void SetUp() override {
    for(int i = 0; i < NUM_OF_PAIRS; ++i) {
      keys.push_back(std::string({char(2 * i), char(3 * i), char(4 * i), char(5 * i)}));
      correct_values_full.push_back(std::string({char(255 ^ i), char(255 ^ i), char(255 ^ i)}));
      if (i % 2 == 1) {
        correct_values_after_delete.push_back(std::string());
        keys_to_be_deleted.push_back(std::string({char(2 * i), char(3 * i), char(4 * i), char(5 * i)}));
      }
      else {
        correct_values_after_delete.push_back(std::string({char(255 ^ i), char(255 ^ i), char(255 ^ i)}));
      }
    }
  }

  std::vector<std::string> keys;
  std::vector<std::string> keys_to_be_deleted;
  std::vector<std::string> correct_values_full;
  std::vector<std::string> correct_values_after_delete;
};

// The following test is on the backend data structure
// to see if the put and get operations work correctly.
// The whole process should not trigger any error
TEST_F(BackendTest, DataStructurePutAndGet) {
  BackendDataStructure backend_data_structure;

  // Put
  for(int i = 0; i < NUM_OF_PAIRS; ++i) {
    bool ok = backend_data_structure.Put(keys[i], correct_values_full[i]);
    // Put operations should be successful here
    EXPECT_EQ(ok, true);
  }

  // Get
  for(int i = 0; i < NUM_OF_PAIRS; ++i) {
    std::string from_data_structure;

    bool ok = backend_data_structure.Get(keys[i], &from_data_structure);
    // Get operations should never fail
    EXPECT_EQ(ok, true);
    // Check if the value matches
    EXPECT_EQ(correct_values_full[i], from_data_structure);
  }
}

// The following test is on the backend data structure as well
// to see if delete operations work correctly.
// Then it gets the values from the backend data structure to see if the values match
// Also, errors should happen when trying to delete the identical keys twice
TEST_F(BackendTest, DataStructurPutGetAndDelete) {
  BackendDataStructure backend_data_structure;

  // Put
  for(int i = 0; i < NUM_OF_PAIRS; ++i) {
    bool ok = backend_data_structure.Put(keys[i], correct_values_full[i]);
    // Put operations should be successful here
    EXPECT_EQ(ok, true);
  }

  // Get
  for(int i = 0; i < NUM_OF_PAIRS; ++i) {
    std::string from_data_structure;

    bool ok = backend_data_structure.Get(keys[i], &from_data_structure);
    // Get operations should never fail
    EXPECT_EQ(ok, true);
    // Check if the value matches
    EXPECT_EQ(correct_values_full[i], from_data_structure);
  }

  // Delete keys
  for(const std::string& key : keys_to_be_deleted) {
    bool ok = backend_data_structure.DeleteKey(key);
    // Delete operations should be successful
    EXPECT_EQ(ok, true);

    ok = backend_data_structure.DeleteKey(key);
    // Delete operations should fail here
    // because a key cannot be deleted twice
    EXPECT_EQ(ok, false);
  }

  // Get again
  for(int i = 0; i < NUM_OF_PAIRS; ++i) {
    std::string from_data_structure;

    bool ok = backend_data_structure.Get(keys[i], &from_data_structure);

    if (i % 2 == 1) {
      // Get operations fail since the keys have already been deleted
      EXPECT_EQ(ok, false);
    }
    else {
      // Get operation should get the right values
      EXPECT_EQ(ok, true);
      EXPECT_EQ(correct_values_after_delete[i], from_data_structure);
    }
  }
}

// This test is similar to the DataStructurePutAndGet above.
// The difference is this tests use grpc to communicate with the backend server.
// Therefore, this test requires the server process to run simultaneously
TEST_F(BackendTest, ServerPutAndGet) {
  BackendClient client;

  // Put
  for(int i = 0; i < NUM_OF_PAIRS; ++i) {
    bool ok = client.SendPutRequest(keys[i], correct_values_full[i]);
    // Put operations should be successful here
    EXPECT_EQ(ok, true);
  }

  // Get
  std::vector<std::string> output_values;
  bool ok = client.SendGetRequest(keys, &output_values);
  // Get operations should never fail
  EXPECT_EQ(ok, true);
  EXPECT_EQ(correct_values_full, output_values);
}

// This test is similar to the DataStructurPutGetAndDelete above.
// The difference is this tests use grpc to communicate with the backend server.
// Therefore, this test requires the server process to run simultaneously

TEST_F(BackendTest, ServerPutGetAndDelete) {
  BackendClient client;

  // Put
  for(int i = 0; i < NUM_OF_PAIRS; ++i) {
    bool ok = client.SendPutRequest(keys[i], correct_values_full[i]);
    // Put operations should be successful here
    EXPECT_EQ(ok, true);
  }

  // Get
  std::vector<std::string> output_values;
  bool ok = client.SendGetRequest(keys, &output_values);
  // Get operations should never fail
  EXPECT_EQ(ok, true);
  // Check if the values match
  EXPECT_EQ(correct_values_full, output_values);

  // Delete keys
  for(const std::string& key : keys_to_be_deleted) {
    bool ok = client.SendDeleteKeyRequest(key);
    // Delete operations should be successful
    EXPECT_EQ(ok, true);

    ok = client.SendDeleteKeyRequest(key);
    // Delete operations should fail here
    // because a key cannot be deleted twice
    EXPECT_EQ(ok, false);
  }

  // Get again
  output_values.clear();
  ok = client.SendGetRequest(keys, &output_values);
  // Get operations should be successful
  EXPECT_EQ(ok, true);
  // Check if the values match
  EXPECT_EQ(correct_values_after_delete, output_values);
}

} // end of namespace

GTEST_API_ int main(int argc, char **argv) {
    std::cout << "Running backend tests from " << __FILE__ << std::endl;
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
