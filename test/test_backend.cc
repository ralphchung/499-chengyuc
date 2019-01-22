#include "backend_client_lib.h"
#include "gtest/gtest.h"
#include <iostream>
#include <string>

namespace {

TEST(BackendTest, All) {
    backend_client client;

    // put
    for(int i = 0; i < 20; ++i) {
        std::string key = {char(2 * i), char(3 * i), char(4 * i), char(5 * i)};
        std::string value = {char(255 ^ i), char(255 ^ i), char(255 ^ i)};

        client.send_put_request(key, value);
    }

    // get
    std::vector<std::string> get_keys;
    for(int i = 0; i < 20; ++i) {
        get_keys.push_back({char(2 * i), char(3 * i), char(4 * i), char(5 * i)});
    }
    std::vector<std::string> result;
    client.send_get_request(get_keys, result);
    for(int i = 0; i < 20; ++i) {
        std::string expect_str = {char(255 ^ i), char(255 ^ i), char(255 ^ i)};
        EXPECT_EQ(expect_str, result[i]);
    }

    // delete
    for(int i = 1; i < 20; i += 2) {
        std::string key = {char(2 * i), char(3 * i), char(4 * i), char(5 * i)};

        client.send_deletekey_request(key);
    }

    // reuse get keys again
    result.clear();
    client.send_get_request(get_keys, result);
    for(int i = 0; i < 20; ++i) {
        if (i % 2 == 1) {
            EXPECT_EQ(std::string(), result[i]);
        }
        else {
            std::string expect_str = {char(255 ^ i), char(255 ^ i), char(255 ^ i)};
            EXPECT_EQ(expect_str, result[i]);
        }
    }
}

} // end of namespace

GTEST_API_ int main(int argc, char **argv)
{
    std::cout << "Running backend tests from " << __FILE__ << std::endl;
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
