
#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "backend_client_lib.h"
#include "backend_server.h"

namespace {

// The following test is on the backend data structure
// This test puts 20 pairs of keys and values.
// Then it gets the values from the backend data structure to see if the values match
// Also, the whole process should not trigger any error
TEST(BackendDataStructureTest, PutAndGet) {
  BackendDataStructure backend_data_structure;

  // Put
  for (int i = 0; i < 20; ++i) {
    // Just a randomly picked key and value generation
    std::string key = {char(2 * i), char(3 * i), char(4 * i), char(5 * i)};
    std::string value = {char(255 ^ i), char(255 ^ i), char(255 ^ i)};

    bool ret = backend_data_structure.Put(key, value);
    // Put operations should be successful here
    EXPECT_EQ(ret, true);
  }

  // Get
  for(int i = 0; i < 20; ++i) {
    // Key and value generation same as above
    std::string key = {char(2 * i), char(3 * i), char(4 * i), char(5 * i)};
    std::string value = {char(255 ^ i), char(255 ^ i), char(255 ^ i)};
    std::string from_data_structure;

    bool ret = backend_data_structure.Get(key, &from_data_structure);
    // Get operations should never fail
    EXPECT_EQ(ret, true);
    // Check if the value matches
    EXPECT_EQ(value, from_data_structure);
  }
}

// The following test is on the backend data structure as well
// This test puts 20 pairs of keys and values.
// Then it deletes half of the pairs.
// Then it gets the values from the backend data structure to see if the values match
// Also, errors should happen when trying to delete the identical keys twice
TEST(BackendDataStructureTest, PutGetAndDelete) {
  BackendDataStructure backend_data_structure;

  // Put
  for (int i = 0; i < 20; ++i) {
    // Just a randomly picked key and value generation
    std::string key = {char(2 * i), char(3 * i), char(4 * i), char(5 * i)};
    std::string value = {char(255 ^ i), char(255 ^ i), char(255 ^ i)};

    bool ret = backend_data_structure.Put(key, value);
    // Put operations should be successful here
    EXPECT_EQ(ret, true);
  }

  // Get
  for(int i = 0; i < 20; ++i) {
    // Key and value generation same as above
    std::string key = {char(2 * i), char(3 * i), char(4 * i), char(5 * i)};
    std::string value = {char(255 ^ i), char(255 ^ i), char(255 ^ i)};
    std::string from_data_structure;

    bool ret = backend_data_structure.Get(key, &from_data_structure);
    // Get operations should never fail
    EXPECT_EQ(ret, true);
    // Check if the value matches
    EXPECT_EQ(value, from_data_structure);
  }

  // Delete half of data
  for(int i = 1; i < 20; i += 2) {
    // regenerate the key sequence just like above
    std::string key = {char(2 * i), char(3 * i), char(4 * i), char(5 * i)};

    bool ret = backend_data_structure.DeleteKey(key);
    // Delete operations should be successful
    EXPECT_EQ(ret, true);

    ret = backend_data_structure.DeleteKey(key);
    // Delete operations should fail here
    // because a key cannot be deleted twice
    EXPECT_EQ(ret, false);
  }

  // Get again
  for(int i = 0; i < 20; ++i) {
    std::string key = {char(2 * i), char(3 * i), char(4 * i), char(5 * i)};
    std::string value = {char(255 ^ i), char(255 ^ i), char(255 ^ i)};
    std::string from_data_structure;

    bool ret = backend_data_structure.Get(key, &from_data_structure);

    if (i % 2 == 1) {
      // Get operations fail since the keys have already been deleted
      EXPECT_EQ(ret, false);
    }
    else {
      // Get operation should get the right values
      EXPECT_EQ(ret, true);
      EXPECT_EQ(value, from_data_structure);
    }
  }
}

// This test is similar to the BackendDataStructureTest.PutAndGet above.
// The difference is this tests use grpc to communicate with the backend server.
// Therefore, this test requires the server process to run simultaneously
TEST(BackendServerTest, PutAndGet) {
  BackendClient client;

  // Put
  for(int i = 0; i < 20; ++i) {
    // Just a randomly picked key and value generation
    std::string key = {char(2 * i), char(3 * i), char(4 * i), char(5 * i)};
    std::string value = {char(255 ^ i), char(255 ^ i), char(255 ^ i)};

    bool ret = client.SendPutRequest(key, value);
    // Put operations should be successful here
    EXPECT_EQ(ret, true);
  }

  // Get
  std::vector<std::string> keys;
  std::vector<std::string> correct_output_values;
  for(int i = 0; i < 20; ++i) {
    // Key and value generation same as above
    keys.push_back({char(2 * i), char(3 * i), char(4 * i), char(5 * i)});
    correct_output_values.push_back({char(255 ^ i), char(255 ^ i), char(255 ^ i)});
  }

  std::vector<std::string> output_values;
  bool ret = client.SendGetRequest(keys, &output_values);
  // Get operations should never fail
  EXPECT_EQ(ret, true);
  // Check if the values match 
  EXPECT_EQ(correct_output_values, output_values);
}

// This test is similar to the BackendDataStructureTest.PutGetAndDelete above.
// The difference is this tests use grpc to communicate with the backend server.
// Therefore, this test requires the server process to run simultaneously
TEST(BackendServerTest, PutGetAndDelete) {
  BackendClient client;

  // Put
  for(int i = 0; i < 20; ++i) {
    // Just a randomly picked key and value generation
    std::string key = {char(2 * i), char(3 * i), char(4 * i), char(5 * i)};
    std::string value = {char(255 ^ i), char(255 ^ i), char(255 ^ i)};

    bool ret = client.SendPutRequest(key, value);
    // Put operations should be successful here
    EXPECT_EQ(ret, true);
  }

  // Get
  std::vector<std::string> keys;
  std::vector<std::string> correct_output_values;
  for(int i = 0; i < 20; ++i) {
    // Key and value generation same as above
    keys.push_back({char(2 * i), char(3 * i), char(4 * i), char(5 * i)});
    correct_output_values.push_back({char(255 ^ i), char(255 ^ i), char(255 ^ i)});
  }

  std::vector<std::string> output_values;
  bool ret = client.SendGetRequest(keys, &output_values);
  // Get operations should never fail
  EXPECT_EQ(ret, true);
  // Check if the values match
  EXPECT_EQ(correct_output_values, output_values);

  // Delete half of data
  for(int i = 1; i < 20; i += 2) {
    // regenerate the key sequence just like above
    std::string key = {char(2 * i), char(3 * i), char(4 * i), char(5 * i)};

    ret = client.SendDeleteKeyRequest(key);
    // Delete operations should be successful
    EXPECT_EQ(ret, true);

    ret = client.SendDeleteKeyRequest(key);
    // Delete operations should fail here
    // because a key cannot be deleted twice
    EXPECT_EQ(ret, false);
  }

  // Get again
  // Re-use the `keys` and `correct_output_values` created above
  // but modify the values whose key has been deleted
  for(int i = 1; i < 20; i += 2) {
    correct_output_values[i] = std::string();
  }

  output_values.clear();
  ret = client.SendGetRequest(keys, &output_values);
  // Get operations should be successful
  EXPECT_EQ(ret, true);
  // Check if the values match
  EXPECT_EQ(correct_output_values, output_values);
}

} // end of namespace

GTEST_API_ int main(int argc, char **argv)
{
    std::cout << "Running backend tests from " << __FILE__ << std::endl;
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
