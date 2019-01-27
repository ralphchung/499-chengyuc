#ifndef CHIRP_SRC_SERVICE_DATA_STRUCTURE_H_
#define CHIRP_SRC_SERVICE_DATA_STRUCTURE_H_

#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

// The data structure for the service layer
// This stores users and chirps data.
// It provides [Register, Login, Logout] for user operations and [Read] for chirps.
// Further operations should be taken under `class UserSession`.
class ServiceDataStructure {
 public:
  // default constructor which initialize the only private data `running_user_sessions_`
  ServiceDataStructure();
  // destructor which frees all memory allocation created for `UserSession`s
  ~ServiceDataStructure();

  // declaration for `struct Chirp`
  struct Chirp;
  // declaration for `class UserSession`
  class UserSession;

  // User register operation
  // returns true if this operation succeeds
  // return false otherwise
  bool UserRegister(const std::string &username);
  // User login operation
  // returns a pointer to a newly created `UserSession`
  // returns nullptr if this operation fails
  // The memory that the returned pointer points to will be handled by the destructor
  // Callers should not try to delete it manually
  UserSession *UserLogin(const std::string &username);
  // User logout operation
  // This frees the memory that the passed pointer points to.
  void UserLogout(const UserSession * const user_session);

  // Chirp read operation
  // returns a pointer to the specified chirp
  // returns nullptr if the specified id could not be found
  // The caller should not be able to modify the content in the specified chirp
  struct Chirp const *ReadChirp(const std::string &id);

 private:
  // declaration for `struct User`
  struct User;

  // This utility function helps maintain the `next_chirp_id_`
  static void IncreaseNextChirpId();
  // This utility function generates a new chirp, insert it into the `chirpid_to_chirp_map_`,
  // and returns it
  static struct Chirp *GenerateNewChirp();

  // This stores the next chirp id which is maintained by `IncreaseNextChirpId()`
  // and used by `GenerateNewChirp()`
  static std::string next_chirp_id_;
  // This data maps username to its corresponding `User` object
  static std::map<std::string, struct User> username_to_user_map_;
  // This data maps chirp id to its corresponding `Chirp` object
  static std::unordered_map<std::string, struct Chirp> chirpid_to_chirp_map_;

  // This maintains all running user sesssions pointers
  // The destructor can free these memory according to this list
  std::list<const UserSession*> running_user_sessions_;
};

// The `struct User` is a private struct in the `ServiceDataStructure`
struct ServiceDataStructure::User {
  // The username of this user
  std::string username;
  // The following list stores the usernames that this user follows
  std::set<std::string> following_list;
  // The chirp list stores the chirp ids that this user has posted
  std::set<std::string> chirp_list;

  // Constructor that initializes the `username`.
  // Other elements are initialized by their default constructors
  User(const std::string &username);
  // Destructor does nothing
  ~User();
};

// The `struct Chirp` is a public struct in the `ServiceDataStructure`.
// Therefore, it should be handled carefully.
struct ServiceDataStructure::Chirp {
  // The nested definition for the Timestamp struct
  struct Timestamp {
    int64_t seconds;
    int64_t useconds;
  };

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
  struct Timestamp time;

  // The ids that reply to this chirp
  std::vector<std::string> children_ids;
};

// This user session is used for a user that has logged in
// Since this is a public class, its constructor is private
// This means it can only be created by `ServiceDataStructure`
class ServiceDataStructure::UserSession {
  public:
  // Default destructor that does nothing
  ~UserSession();

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
  struct Chirp* const PostChirp(const std::string &text, const std::string &parent_id = "");
  // Edit a chirp
  // returns the edited chirp pointer if this operation succeeds
  // returns nullptr otherwise
  struct Chirp* const EditChirp(const std::string &id, const std::string &text);
  // Delete a chirp
  // returns true if this operation succeeds
  // returns false otherwise
  bool DeleteChirp(const std::string &id);

  // This returns the user's following list
  inline const std::set<std::string> &get_following_list() {
    return user_->following_list;
  }
  // This returns the user's chirp list
  inline const std::set<std::string> &get_chirp_list() {
    return user_->chirp_list;
  }
  
  private:
  // Private constructor
  // This initialize the member data `user_`
  UserSession(struct User * const user);

  // Befriend with `ServiceDataStructure`
  // so that it can call the constructor
  friend class ServiceDataStructure;

  // The `User` that logs in in this session
  struct User * const user_;
};

#endif /* CHIRP_SRC_SERVICE_DATA_STRUCTURE_H_ */
