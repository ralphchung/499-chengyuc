#ifndef CHIRP_SRC_SERVICE_DATA_STRUCTURE_H_
#define CHIRP_SRC_SERVICE_DATA_STRUCTURE_H_

#include <cassert>
#include <climits>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <sys/time.h>
#include <unordered_map>
#include <vector>

#define DEBUG

// The data structure for the service layer
// This stores users and chirps data.
// It provides [Register, Login, Logout] for user operations and [Read] for chirps.
// Further operations should be taken under `class UserSession`.
class ServiceDataStructure {
 private:
  // The `struct User` is a private struct in the `ServiceDataStructure`
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

 public:
  // The `struct Chirp` is a public struct in the `ServiceDataStructure`.
  // Therefore, it should be handled carefully.
  struct Chirp {
    // The id of this chirp
    std::string id;
    // The username of the posting user
    std::string user;
    // The chirp id that this chirp replys to
    // This field leaves if this chirp replys to no chirp
    std::string parent_id;
    // The text of this chirp
    std::string text;
    // The timestamp of this chirp
    struct timeval time;

    // The ids that reply to this chirp
    std::vector<std::string> children_ids;

    Chirp() : id(), user(), parent_id(), text(), time(), children_ids() {}
    Chirp(const std::string &user, const std::string &parent_id, const std::string &text);
  };

  // This user session is used for a user that has logged in
  // Since this is a public class, its constructor is private
  // This means it can only be created by `ServiceDataStructure`
  class UserSession {
  public:
    // Follow a specified user
    // returns true if this operation succeeds
    // returns false otherwise
    bool Follow(const std::string &username);

    // Unfollow a specifed user
    // returns true if this operation succeeds
    // returns false otherwise
    bool Unfollow(const std::string &username);

    // Post a chirp
    // If the `parent_id` is not specified, its default value will be an empty string.
    // returns the newly posted chirp id if this operation succeeds
    // returns an empty string otherwise
    std::string PostChirp(const std::string &text, const std::string &parent_id = "");

    // Edit a chirp
    // returns true if this operation succeeds
    // returns false otherwise
    bool EditChirp(const std::string &id, const std::string &text);

    // Delete a chirp
    // returns true if this operation succeeds
    // returns false otherwise
    bool DeleteChirp(const std::string &id);

    // Monitor from a specified time to now
    // returns a set containing chirp ids
    // the `struct timeval` passing in will be changed to the current time
    std::set<std::string> MonitorFrom(struct timeval * const from);

    // This returns the user's following list
    inline const std::set<std::string> SessionGetUserFollowingList() {
      std::set<std::string> ret;
      bool ok = GetUserFollowingList(user_.username, &ret);
      assert(ok);
      return ret;
    }

    // This returns the user's chirp list
    inline const std::set<std::string> SessionGetUserChirpList() {
      std::set<std::string> ret;
      bool ok = GetUserChirpList(user_.username, &ret);
      assert(ok);
      return ret;
    }

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
  // returns true if this operation succeeds
  // return false otherwise
  bool UserRegister(const std::string &username);

  // User login operation
  // returns a pointer to a newly created `UserSession`
  // returns nullptr if this operation fails
  std::unique_ptr<UserSession> UserLogin(const std::string &username);

  // Chirp read operation
  // returns true if this operation succeeds
  // return false otherwise
  bool ReadChirp(const std::string &id, struct Chirp * const chirp);

 private:
  // This utility function helps maintain the `next_chirp_id_`
  inline static void IncreaseNextChirpId(std::string *next_chirp_id) {
    if (next_chirp_id->back() < CHAR_MAX) {
      ++next_chirp_id->back();
    } else {
      next_chirp_id->push_back(CHAR_MIN);
    }
  }

  #ifdef DEBUG

  // This stores the next chirp id which is maintained by `IncreaseNextChirpId()`
  // and used by `GenerateNewChirp()`
  static std::string next_chirp_id_;
  // This data maps username to its corresponding `User` object
  static std::map<std::string, struct User> username_to_user_map_;
  // This data maps username to its following list
  static std::map<std::string, std::set<std::string> > username_to_following_map_;
  // This data maps username to its chirp list
  static std::map<std::string, std::set<std::string> > username_to_chirp_map_;
  // This data maps chirp id to its corresponding `Chirp` object
  static std::unordered_map<std::string, struct Chirp> chirpid_to_chirp_map_;

  #endif /* DEBUG */

  // Wrapper function to get `next_chirp_id`
  static std::string GetNextChirpId();

  // Wrapper function to get a specified user object
  static bool GetUser(const std::string &username, struct User * const user);

  // Wrapper function to save a specified user object
  static bool SaveUser(const std::string &username, const struct User &user);

  // Wrapper function to delete a specified user object
  static bool DeleteUser(const std::string &username);

  // Wrapper function to get the following list of a specified user
  static bool GetUserFollowingList(const std::string &username, std::set<std::string> * const following_list);

  // Wrapper function to save the following list of a specified user
  static bool SaveUserFollowingList(const std::string &username, const std::set<std::string> &following_list);

  // Wrapper function to delete the following list of a specified user
  static bool DeleteUserFollowingList(const std::string &username);

  // Wrapper function to get the chirp list of a specified user
  static bool GetUserChirpList(const std::string &username, std::set<std::string> * const chirp_list);

  // Wrapper function to save the chirp list of a specified user
  static bool SaveUserChirpList(const std::string &username, const std::set<std::string> &chirp_list);

  // Wrapper function to delete the chirp list of a specified user
  static bool DeleteUserChirpList(const std::string &username);

  // Wrapper function to get a chirp
  static bool GetChirp(const std::string &chirp_id, struct Chirp * const chirp);

  // Wrapper function to save a chirp
  static bool SaveChirp(const std::string &chirp_id, const struct Chirp &chirp);

  // Wrapper function to delete a chirp
  static bool DeleteChirp(const std::string &chirp_id);
};

#endif /* CHIRP_SRC_SERVICE_DATA_STRUCTURE_H_ */
