#ifndef CHIRP_SRC_SERVICE_DATA_STRUCTURE_H_
#define CHIRP_SRC_SERVICE_DATA_STRUCTURE_H_

#include <sys/time.h>
#include <climits>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <glog/logging.h>

#include "backend_client_lib.h"
#include "service_data.pb.h"
#include "utility.h"

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

    INTERNAL_BACKEND_ERROR,

    UNKOWN_ERROR = INT_MAX
  };

  class User {
   public:
    // Constructor that initializes the `username`.
    // Other elements are initialized by their default constructors
    User() = default;
    User(const std::string &username);

    // Deserialization
    void ImportBinary(const std::string &input);
    // Serialization
    const std::string ExportBinary() const;

    // accessors
    inline const std::string &get_username() { return user_.username(); }
    inline void set_username(const std::string &username) {
      user_.set_username(username);
    }
    inline const struct timeval &get_last_update() { return last_update_; }
    inline void set_last_update(const struct timeval &time) {
      last_update_ = time;
      auto from_protobuf = user_.mutable_last_update();
      from_protobuf->set_seconds(time.tv_sec);
      from_protobuf->set_useconds(time.tv_usec);
    }
    inline void set_last_update(const uint64_t &seconds,
                                const uint64_t &useconds) {
      struct timeval time;
      time.tv_sec = seconds;
      time.tv_usec = useconds;
      set_last_update(time);
    }

   private:
    // the protobuf object
    ServiceData::User user_;
    // last update timestamp
    // maintaining a `struct timeval` here makes it easier to use some libs
    struct timeval last_update_;
  };

  // This inherits from set<string>. Use protobuf only on deserialization and
  // serialization.
  class UserFollowingList : public std::set<std::string> {
   public:
    // Deserialization
    void ImportBinary(const std::string &input);
    // Serialization
    const std::string ExportBinary() const;
  };

  // This inherits from set<uint64_t>. Use protobuf only on deserialization and
  // serialization.
  class UserChirpList : public std::set<uint64_t> {
   public:
    // Deserialization
    void ImportBinary(const std::string &input);
    // Serialization
    const std::string ExportBinary() const;
  };

  class Chirp {
   public:
    Chirp() = default;
    Chirp(const std::string &user, const uint64_t &parent_id,
          const std::string &text);

    // Deserialization
    void ImportBinary(const std::string &input);
    // Serialization
    const std::string ExportBinary() const;

    inline const uint64_t get_id() const { return chirp_.id(); }
    inline const std::string &get_username() const { return chirp_.username(); }
    inline void set_username(const std::string &username) {
      chirp_.set_username(username);
    }
    inline const uint64_t get_parent_id() const { return chirp_.parent_id(); }
    inline const std::string &get_text() const { return chirp_.text(); }
    inline void set_text(const std::string &text) { chirp_.set_text(text); }
    inline const struct timeval &get_time() const { return time_; }
    inline const std::set<uint64_t> &get_children_ids() const {
      return children_ids_;
    }
    inline void insert_children_id(const uint64_t &id) {
      children_ids_.insert(id);
    }
    inline void erase_children_id(const uint64_t &id) {
      children_ids_.erase(id);
    }

   private:
    ServiceData::Chirp chirp_;
    // The timestamp of this chirp
    struct timeval time_;
    std::set<uint64_t> children_ids_;
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
    // The newly posted chirp id will be set to `chirp_id` if this operation
    // succeeds.
    // returns OK if this operation succeeds
    // returns other return codes otherwise
    ReturnCodes PostChirp(const std::string &text, uint64_t *const chirp_id,
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
    std::set<uint64_t> MonitorFrom(struct timeval *const from);

    // This returns the username
    inline const std::string &SessionGetUsername() {
      return this->user_.get_username();
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
    // so that it can call its constructor
    friend class ServiceDataStructure;

    // The `User` that logs in in this session
    User user_;
  };

  // User register operation
  // returns OK if this operation succeeds
  // returns other return codes otherwise
  ReturnCodes UserRegister(const std::string &username);

  // User login operation
  // returns a pointer to the newly created `UserSession`
  // returns nullptr if this operation fails
  std::unique_ptr<UserSession> UserLogin(const std::string &username);

  // Chirp read operation
  // returns OK if this operation succeeds
  // returns other return codes otherwise
  ReturnCodes ReadChirp(const uint64_t &id, Chirp *const chirp);
};

namespace chirp_connect_backend {
// Declaration for the `BackendClient` object
extern std::unique_ptr<BackendClient> backend_client_;

// Wrapper function to get `next_chirp_id`
uint64_t GetNextChirpId();

// Wrapper function to get a specified user object
bool GetUser(const std::string &username,
             struct ServiceDataStructure::User *const user);

// Wrapper function to save a specified user object
bool SaveUser(const std::string &username,
              const struct ServiceDataStructure::User &user);

// Wrapper function to delete a specified user object
bool DeleteUser(const std::string &username);

// Wrapper function to get the following list of a specified user
bool GetUserFollowingList(
    const std::string &username,
    ServiceDataStructure::UserFollowingList *const following_list);

// Wrapper function to save the following list of a specified user
bool SaveUserFollowingList(
    const std::string &username,
    const ServiceDataStructure::UserFollowingList &following_list);

// Wrapper function to delete the following list of a specified user
bool DeleteUserFollowingList(const std::string &username);

// Wrapper function to get the chirp list of a specified user
bool GetUserChirpList(const std::string &username,
                      ServiceDataStructure::UserChirpList *const chirp_list);

// Wrapper function to save the chirp list of a specified user
bool SaveUserChirpList(const std::string &username,
                       const ServiceDataStructure::UserChirpList &chirp_list);

// Wrapper function to delete the chirp list of a specified user
bool DeleteUserChirpList(const std::string &username);

// Wrapper function to get a chirp
bool GetChirp(const uint64_t &chirp_id,
              struct ServiceDataStructure::Chirp *const chirp);

// Wrapper function to save a chirp
bool SaveChirp(const uint64_t &chirp_id,
               const struct ServiceDataStructure::Chirp &chirp);

// Wrapper function to delete a chirp
bool DeleteChirp(const uint64_t &chirp_id);
} /* namespace chirp_connect_backend */

inline const ServiceDataStructure::UserFollowingList
ServiceDataStructure::UserSession::SessionGetUserFollowingList() {
  ServiceDataStructure::UserFollowingList ret;
  bool ok =
      chirp_connect_backend::GetUserFollowingList(user_.get_username(), &ret);
  CHECK(ok) << "The user following list for user `" << user_.get_username()
            << "` should exist.";
  return ret;
}

inline const ServiceDataStructure::UserChirpList
ServiceDataStructure::UserSession::SessionGetUserChirpList() {
  ServiceDataStructure::UserChirpList ret;
  bool ok = chirp_connect_backend::GetUserChirpList(user_.get_username(), &ret);
  CHECK(ok) << "The user chirp list for user `" << user_.get_username()
            << "` should exist.";
  return ret;
}

inline ServiceDataStructure::ReturnCodes ServiceDataStructure::ReadChirp(
    const uint64_t &id, struct ServiceDataStructure::Chirp *const chirp) {
  bool ok = chirp_connect_backend::GetChirp(id, chirp);
  if (!ok) {
    return ServiceDataStructure::CHIRP_ID_NOT_FOUND;
  }

  return ServiceDataStructure::OK;
}

#endif /* CHIRP_SRC_SERVICE_DATA_STRUCTURE_H_ */
