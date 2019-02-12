#ifndef CHIRP_SRC_SERVICE_DATA_STRUCTURE_H_
#define CHIRP_SRC_SERVICE_DATA_STRUCTURE_H_

#include <climits>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <sys/time.h>
#include <unordered_map>
#include <vector>

#include <glog/logging.h>

#include "backend_client_lib.h"

using UserChirpList = std::set<uint64_t>;
using UserFollowingList = std::set<std::string>;

// The data structure for the service layer
// This stores users and chirps data.
// It provides [Register, Login, Logout] for user operations.
// Further operations should be taken under `class UserSession`.
class ServiceDataStructure {
 public:
  // Definition of return codes, showing different error messages
  enum ReturnCodes : int {
    OK = 0,
    INVALID_ARGUMENT,
    USER_EXISTS,
    FOLLOWEE_NOT_FOUND,
    CHIRP_ID_NOT_FOUND,
    REPLY_ID_NOT_FOUND,
    PERMISSION_DENIED,

    INTENRAL_BACKEND_ERROR,

    UNKOWN_ERROR = INT_MAX
  };

  struct User {
    // The username of this user
    std::string username;
    // The last posting time of this user
    struct timeval last_update_chirp_time;

    // Constructor that initializes the `username`.
    // Other elements are initialized by their default constructors
    User() : username(), last_update_chirp_time() {}
    User(const std::string &username);
  };

  // The `struct Chirp` is a public struct in the `ServiceDataStructure`.
  // Therefore, it should be handled carefully.
  struct Chirp {
    // The id of this chirp
    uint64_t id;
    // The username of the posting user
    std::string user;
    // The chirp id that this chirp replys to
    // This field leaves if this chirp replys to no chirp
    uint64_t parent_id;
    // The text of this chirp
    std::string text;
    // The timestamp of this chirp
    struct timeval time;

    // The ids that reply to this chirp
    std::vector<uint64_t> children_ids;

    Chirp() : id(), user(), parent_id(), text(), time(), children_ids() {}
    Chirp(const std::string &user, const uint64_t &parent_id, const std::string &text);
  };

  // This user session is used for a user that has logged in
  // Since this is a public class, its constructor is private
  // This means it can only be created by `ServiceDataStructure`
  class UserSession {
   public:
    // Follow a specified user
    // returns OK if this operation succeeds
    // returns other return codes otherwise
    ReturnCodes Follow(const std::string &username);

    // Unfollow a specifed user
    // returns OK if this operation succeeds
    // returns other return codes otherwise
    ReturnCodes Unfollow(const std::string &username);

    // Post a chirp
    // If the `parent_id` is not specified, its default value will be 0.
    // the newly posted chirp id will be set to `chirp_id` if this operation succeeds
    // returns OK if this operation succeeds
    // returns other return codes otherwise
    ReturnCodes PostChirp(const std::string &text,
                          uint64_t * const chirp_id,
                          const uint64_t &parent_id = 0);

    // Edit a chirp
    // returns OK if this operation succeeds
    // returns other return codes otherwise
    ReturnCodes EditChirp(const uint64_t &id, const std::string &text);

    // Delete a chirp
    // returns OK if this operation succeeds
    // returns other return codes otherwise
    ReturnCodes DeleteChirp(const uint64_t &id);

    // Monitor from a specified time to now
    // returns a set containing chirp ids
    // the `struct timeval` passing in will be changed to the current time
    std::set<uint64_t> MonitorFrom(struct timeval * const from);

    // This returns the username
    inline const std::string &SessionGetUsername() {
      return this->user_.username;
    }

    // This returns the user's following list
    const UserFollowingList SessionGetUserFollowingList();

    // This returns the user's chirp list
    const UserChirpList SessionGetUserChirpList();

   private:
    // Private constructor
    // This initializes the member data `user_`
    UserSession(const std::string &username);

    // Befriend with `ServiceDataStructure`
    // so that it can call the constructor
    friend class ServiceDataStructure;

    // The `User` that logs in in this session
    struct User user_;
  };

  // User register operation
  // returns OK if this operation succeeds
  // returns other return codes otherwise
  ReturnCodes UserRegister(const std::string &username);

  // User login operation
  // returns a pointer to a newly created `UserSession`
  // returns nullptr if this operation fails
  std::unique_ptr<UserSession> UserLogin(const std::string &username);

  // Chirp read operation
  // returns OK if this operation succeeds
  // returns other return codes otherwise
  ReturnCodes ReadChirp(const uint64_t &id, struct Chirp * const chirp);
};

namespace chirp_connect_backend {
// Declaration for the `BackendClient` object
extern std::unique_ptr<BackendClient> backend_client_;

// Binary `User` format:
// 0x0 - sizeof(struct timeval): struct timeval: last_update_chirp_time
// Remaining:  username
//
// Translate binary data to `struct User`
void DecomposeBinaryUser(const std::string &input, struct ServiceDataStructure::User * const user);
// Translate `struct User` to binary data
void ComposeBinaryUser(const struct ServiceDataStructure::User &user, std::string * const output);

// Binary `following list` format:
// 0x0 - 0x7 bytes: uint64_t: length of this list
// Remaining: array of pairs: (username.size(), username)
//
// Translate binary data to `UserFollowingList`
void DecomposeBinaryUserFollowing(const std::string &input, UserFollowingList * const following_list);
// Translate `UserFollowingList` to binary data
void ComposeBinaryUserFollowing(const UserFollowingList &following_list, std::string * const output);

// Binary `chirp list` format:
// 0x0 - 0x7 bytes: uint64_t: length of this list
// Remaining: uint64_t[]: array of chirp ids
//
// Translate binary data to `UserChirpList`
void DecomposeBinaryUserChirp(const std::string &input, UserChirpList * const chirp_list);
// Translate `UserChirpList` to binary data
void ComposeBinaryUserChirp(const UserChirpList &chirp_list, std::string * const output);

// Binary `chirp` format:
// 0x0 - 0x7 bytes: uint64_t: chirp id
// 0x8 - 0xF bytes: uint64_t: parent_id
// 0x10 - 0x10 + sizeof(struct timeval) bytes: struct timeval: time
//     - +8bytes: uint64_t: length of username
//     -        : string: username
//     - +8bytes: uint64_t: length of text
//     -        : string: text
//     - +8bytes: uint64_t: length of vector of children chirp ids
// Remaining: uint64_t[]: array of children chirp ids
//
// Translate binary data to `struct Chirp`
void DecomposeBinaryChirp(const std::string &input, struct ServiceDataStructure::Chirp * const chirp);
// Translate `struct Chirp` to binary data
void ComposeBinaryChirp(const struct ServiceDataStructure::Chirp &chirp, std::string * const output);

// Wrapper function to get `next_chirp_id`
uint64_t GetNextChirpId();

// Wrapper function to get a specified user object
bool GetUser(const std::string &username, struct ServiceDataStructure::User * const user);

// Wrapper function to save a specified user object
bool SaveUser(const std::string &username, const struct ServiceDataStructure::User &user);

// Wrapper function to delete a specified user object
bool DeleteUser(const std::string &username);

// Wrapper function to get the following list of a specified user
bool GetUserFollowingList(const std::string &username, UserFollowingList * const following_list);

// Wrapper function to save the following list of a specified user
bool SaveUserFollowingList(const std::string &username, const UserFollowingList &following_list);

// Wrapper function to delete the following list of a specified user
bool DeleteUserFollowingList(const std::string &username);

// Wrapper function to get the chirp list of a specified user
bool GetUserChirpList(const std::string &username, UserChirpList * const chirp_list);

// Wrapper function to save the chirp list of a specified user
bool SaveUserChirpList(const std::string &username, const UserChirpList &chirp_list);

// Wrapper function to delete the chirp list of a specified user
bool DeleteUserChirpList(const std::string &username);

// Wrapper function to get a chirp
bool GetChirp(const uint64_t &chirp_id, struct ServiceDataStructure::Chirp * const chirp);

// Wrapper function to save a chirp
bool SaveChirp(const uint64_t &chirp_id, const struct ServiceDataStructure::Chirp &chirp);

// Wrapper function to delete a chirp
bool DeleteChirp(const uint64_t &chirp_id);
} /* namespace chirp_connect_backend */

inline const UserFollowingList ServiceDataStructure::UserSession::SessionGetUserFollowingList() {
  UserFollowingList ret;
  bool ok = chirp_connect_backend::GetUserFollowingList(user_.username, &ret);
  CHECK(ok) << "The user following list for user `" << user_.username << "` should exist.";
  return ret;
}

inline const UserChirpList ServiceDataStructure::UserSession::SessionGetUserChirpList() {
  UserChirpList ret;
  bool ok = chirp_connect_backend::GetUserChirpList(user_.username, &ret);
  CHECK(ok) << "The user chirp list for user `" << user_.username << "` should exist.";
  return ret;
}

inline ServiceDataStructure::ReturnCodes ServiceDataStructure::ReadChirp(
    const uint64_t &id,
    struct ServiceDataStructure::Chirp * const chirp) {

  bool ok = chirp_connect_backend::GetChirp(id, chirp);
  if (ok) {
    return ServiceDataStructure::OK;
  } else {
    return ServiceDataStructure::CHIRP_ID_NOT_FOUND;
  }
}

#endif /* CHIRP_SRC_SERVICE_DATA_STRUCTURE_H_ */
