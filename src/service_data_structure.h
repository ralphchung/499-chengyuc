#ifndef CHIRP_SRC_SERVICE_DATA_STRUCTURE_H_
#define CHIRP_SRC_SERVICE_DATA_STRUCTURE_H_

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <sys/time.h>
#include <unordered_map>
#include <vector>

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
    // The following list stores the usernames that this user follows
    std::set<std::string> following_list;
    // The chirp list stores the chirp ids that this user has posted
    std::set<std::string> chirp_list;
    // The last posting time of this user
    struct timeval last_update_chirp_time;

    // Constructor that initializes the `username`.
    // Other elements are initialized by their default constructors
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
    // returns the newly posted chirp pointer if this operation succeeds
    // returns nullptr otherwise
    const struct Chirp* PostChirp(const std::string &text, const std::string &parent_id = "");

    // Edit a chirp
    // returns the edited chirp pointer if this operation succeeds
    // returns nullptr otherwise
    const struct Chirp* EditChirp(const std::string &id, const std::string &text);

    // Delete a chirp
    // returns true if this operation succeeds
    // returns false otherwise
    bool DeleteChirp(const std::string &id);

    // Monitor from a specified time to now
    // returns a vector containing chirp ids
    // the `struct timeval` passing in will be changed to the current time
    std::vector<std::string> MonitorFrom(struct timeval * const from);

    // This returns the user's following list
    inline const std::set<std::string> &GetUserFollowingList() {
      return user_->following_list;
    }

    // This returns the user's chirp list
    inline const std::set<std::string> &GetUserChirpList() {
      return user_->chirp_list;
    }

  private:
    // Private constructor
    // This initializes the member data `user_`
    UserSession(struct User * const user);

    // Befriend with `ServiceDataStructure`
    // so that it can call the constructor
    friend class ServiceDataStructure;

    // The `User` that logs in in this session
    struct User * const user_;
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
  // returns a pointer to the specified chirp stored in the private member map
  // returns nullptr if the specified id could not be found
  // The caller should not be able to modify the content in the specified chirp
  struct Chirp const *ReadChirp(const std::string &id);

 private:
  // This utility function helps maintain the `next_chirp_id_`
  static void IncreaseNextChirpId();

  // This utility function generates a new chirp, insert it into the `chirpid_to_chirp_map_`,
  // and returns the pointer pointing to it
  static struct Chirp *GenerateNewChirp();

  // This stores the next chirp id which is maintained by `IncreaseNextChirpId()`
  // and used by `GenerateNewChirp()`
  static std::string next_chirp_id_;
  // This data maps username to its corresponding `User` object
  static std::map<std::string, struct User> username_to_user_map_;
  // This data maps chirp id to its corresponding `Chirp` object
  static std::unordered_map<std::string, struct Chirp> chirpid_to_chirp_map_;
};

#endif /* CHIRP_SRC_SERVICE_DATA_STRUCTURE_H_ */
