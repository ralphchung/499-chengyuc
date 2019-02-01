#include "service_data_structure.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <sys/time.h>

ServiceDataStructure::UserSession::UserSession(const std::string &username) {
  bool ok = GetUser(username, &(this->user_));
  assert(ok);
}

bool ServiceDataStructure::UserSession::Follow(const std::string &username) {
  std::set<std::string> following_list;
  bool ok = GetUserFollowingList(user_.username, &following_list);
  assert(ok);

  bool user_found = GetUser(username, nullptr);

  // The specifed user is found
  if (user_found) {
    following_list.insert(username);
    return SaveUserFollowingList(user_.username, following_list);
  } else {
    return false;
  }
}

bool ServiceDataStructure::UserSession::Unfollow(const std::string &username) {
  std::set<std::string> following_list;
  bool ok = GetUserFollowingList(user_.username, &following_list);
  assert(ok);

  // If the specifed user is erased
  bool erased = following_list.erase(username);
  return erased && SaveUserFollowingList(user_.username, following_list);
}

std::string ServiceDataStructure::UserSession::PostChirp(
    const std::string &text,
    const std::string &parent_id) {

  struct Chirp chirp(user_.username, parent_id, text);

  // If the `parent_id` is specified
  if (!parent_id.empty()) {
    struct Chirp parent_chirp;
    bool parent_found = GetChirp(parent_id, &parent_chirp);
    if (!parent_found) {
      return "";
    }

    parent_chirp.children_ids.push_back(chirp.id);
    bool ok = SaveChirp(parent_chirp.id, parent_chirp);
    if (!ok) {
      return "";
    }
  }

  bool ok = SaveChirp(chirp.id, chirp);
  if (!ok) {
    return "";
  }

  // Update the information of this user
  user_.last_update_chirp_time = chirp.time;
  ok = SaveUser(user_.username, user_);
  if (!ok) {
    return "";
  }

  std::set<std::string> chirp_list;
  ok = GetUserChirpList(user_.username, &chirp_list);
  assert(ok);
  chirp_list.insert(chirp.id);
  ok = SaveUserChirpList(user_.username, chirp_list);
  if (!ok) {
    return "";
  }

  return chirp.id;
}

bool ServiceDataStructure::UserSession::EditChirp(
    const std::string &id,
    const std::string &text) {

  struct Chirp chirp;
  bool ok = GetChirp(id, &chirp);

  // If the chirp is found and its posting user is the user in this session
  if (ok && chirp.user == user_.username) {
    chirp.text = text;
    ok = SaveChirp(id, chirp);
    return ok;
  }

  return false;
}

bool ServiceDataStructure::UserSession::DeleteChirp(const std::string &id) {
  struct Chirp chirp;
  bool ok = GetChirp(id, &chirp);

  // If the chirp is found and its posting user is the user in this session
  if (ok && chirp.user == user_.username) {
    std::set<std::string> chirp_list;
    ok = GetUserChirpList(user_.username, &chirp_list);
    assert(ok);

    chirp_list.erase(chirp.id);
    ok = ServiceDataStructure::DeleteChirp(id);
    ok &= SaveUserChirpList(user_.username, chirp_list);
    return ok;
  }

  return false;
}

std::set<std::string> ServiceDataStructure::UserSession::MonitorFrom(struct timeval * const from) {
  struct timeval now;
  gettimeofday(&now, nullptr);

  std::set<std::string> ret;

  std::set<std::string> user_following_list;
  bool ok = GetUserFollowingList(user_.username, &user_following_list);
  assert(ok);

  // TODO: may open threads to do the following things
  for(const auto &username : user_following_list) {
    struct User user;
    ok = GetUser(username, &user);
    assert(ok);

    if (timercmp(&(user.last_update_chirp_time), from, >)) {
      // Do push_backs
      std::set<std::string> user_chirp_list;
      ok = GetUserChirpList(user.username, &user_chirp_list);
      assert(ok);

      for(const auto &chirp_id : user_chirp_list) {
        struct Chirp chirp;
        ok = GetChirp(chirp_id, &chirp);
        assert(ok);

        if (timercmp(&(chirp.time), from, >)) {
          ret.insert(chirp_id);
        }
      }
    }
  }

  *from = now;
  return ret;
}

bool ServiceDataStructure::UserRegister(const std::string &username) {
  // Invalid username
  if (username.empty()) {
    return false;
  }

  bool user_found = GetUser(username, nullptr);
  // If the specified username is not found
  if (!user_found) {
    struct User new_user(username);
    std::set<std::string> chirp_list_or_following_list;
    bool ok = SaveUser(username, new_user) &&
              SaveUserChirpList(username, chirp_list_or_following_list) &&
              SaveUserFollowingList(username, chirp_list_or_following_list);
    if (!ok) {
      DeleteUser(username);
      DeleteUserChirpList(username);
      DeleteUserFollowingList(username);
    }
    return ok;
  }

  return false;
}

std::unique_ptr<ServiceDataStructure::UserSession> ServiceDataStructure::UserLogin(const std::string &username) {
  bool user_found = GetUser(username, nullptr);

  // If the specified username is found
  if (user_found) {
    return std::unique_ptr<ServiceDataStructure::UserSession>(new UserSession(username));
  } else {
    return nullptr;
  }
}

bool ServiceDataStructure::ReadChirp(const std::string &id, struct ServiceDataStructure::Chirp * const chirp) {
  return GetChirp(id, chirp);
}

ServiceDataStructure::User::User(const std::string &username) : username(username) {
  gettimeofday(&last_update_chirp_time, nullptr);
}

ServiceDataStructure::Chirp::Chirp(const std::string &user, const std::string &parent_id, const std::string &text)
    : user(user), parent_id(parent_id), text(text) {
  id = GetNextChirpId();
  gettimeofday(&time, nullptr);
}

#ifdef DEBUG

// Static members initializations
std::string ServiceDataStructure::next_chirp_id_({CHAR_MIN});
std::map<std::string, struct ServiceDataStructure::User> ServiceDataStructure::username_to_user_map_;
std::map<std::string, std::set<std::string> > ServiceDataStructure::username_to_following_map_;
std::map<std::string, std::set<std::string> > ServiceDataStructure::username_to_chirp_map_;
std::unordered_map<std::string, struct ServiceDataStructure::Chirp> ServiceDataStructure::chirpid_to_chirp_map_;

#endif /* DEBUG */

// Wrapper functions
// Wrapper function to get `next_chirp_id`
std::string ServiceDataStructure::GetNextChirpId() {
  #ifdef DEBUG
  std::string ret = next_chirp_id_;
  IncreaseNextChirpId(&next_chirp_id_);
  return ret;
  #else /* DEBUG */
  // TODO: grpc
  #endif /* DEBUG */
}

// Wrapper function to get a specified user object
bool ServiceDataStructure::GetUser(const std::string &username, struct ServiceDataStructure::User * const user) {
  #ifdef DEBUG
  auto it = username_to_user_map_.find(username);
  if (it != username_to_user_map_.end()) {
    if (user != nullptr) {
      *user = it->second;
    }
    return true;
  } else {
    return false;
  }
  #else
  // TODO: grpc
  #endif /* DEBUG */
}

// Wrapper function to save a specified user object
bool ServiceDataStructure::SaveUser(const std::string &username, const struct ServiceDataStructure::User &user) {
  #ifdef DEBUG
  username_to_user_map_[username] = user;
  return true;
  #else
  // TODO: grpc
  #endif /* DEBUG */
}

// Wrapper function to delete a specified user object
bool ServiceDataStructure::DeleteUser(const std::string &username) {
  #ifdef DEBUG
  return username_to_user_map_.erase(username);
  #else
  // TODO: grpc
  #endif /* DEBUG */
}

// Wrapper function to get the following list of a specified user
bool ServiceDataStructure::GetUserFollowingList(const std::string &username, std::set<std::string> * const following_list) {
  #ifdef DEBUG
  auto it = username_to_following_map_.find(username);
  if (it != username_to_following_map_.end()) {
    if (following_list != nullptr) {
      *following_list = it->second;
    }
    return true;
  } else {
    return false;
  }
  #else
  // TODO: grpc
  #endif /* DEBUG */
}

// Wrapper function to save the following list of a specified user
bool ServiceDataStructure::SaveUserFollowingList(const std::string &username, const std::set<std::string> &following_list) {
  #ifdef DEBUG
  username_to_following_map_[username] = following_list;
  return true;
  #else
  // TODO: grpc
  #endif /* DEBUG */
}

// Wrapper function to delete the following list of a specified user
bool ServiceDataStructure::DeleteUserFollowingList(const std::string &username) {
  #ifdef DEBUG
  return username_to_following_map_.erase(username);
  #else
  // TODO: grpc
  #endif /* DEBUG */
}

// Wrapper function to get the chirp list of a specified user
bool ServiceDataStructure::GetUserChirpList(const std::string &username, std::set<std::string> * const chirp_list) {
  #ifdef DEBUG
  auto it = username_to_chirp_map_.find(username);
  if (it != username_to_chirp_map_.end()) {
    if (chirp_list != nullptr) {
      *chirp_list = it->second;
    }
    return true;
  } else {
    return false;
  }
  #else
  // TODO: grpc
  #endif /* DEBUG */
}

// Wrapper function to save the chirp list of a specified user
bool ServiceDataStructure::SaveUserChirpList(const std::string &username, const std::set<std::string> &chirp_list) {
  #ifdef DEBUG
  username_to_chirp_map_[username] = chirp_list;
  return true;
  #else
  // TODO: grpc
  #endif /* DEBUG */
}

// Wrapper function to delete the chirp list of a specified user
bool ServiceDataStructure::DeleteUserChirpList(const std::string &username) {
  #ifdef DEBUG
  return username_to_chirp_map_.erase(username);
  #else
  // TODO: grpc
  #endif /* DEBUG */
}

// Wrapper function to get a chirp
bool ServiceDataStructure::GetChirp(const std::string &chirp_id, struct ServiceDataStructure::Chirp * const chirp) {
  #ifdef DEBUG
  auto it = chirpid_to_chirp_map_.find(chirp_id);
  if (it != chirpid_to_chirp_map_.end()) {
    if (chirp != nullptr) {
      *chirp = it->second;
    }
    return true;
  } else {
    return false;
  }
  #else
  // TODO: grpc
  #endif /* DEBUG */
}

// Wrapper function to save a chirp
bool ServiceDataStructure::SaveChirp(const std::string &chirp_id, const struct ServiceDataStructure::Chirp &chirp) {
  #ifdef DEBUG
  chirpid_to_chirp_map_[chirp_id] = chirp;
  return true;
  #else
  // TODO: grpc
  #endif /* DEBUG */
}

// Wrapper function to delete a chirp
bool ServiceDataStructure::DeleteChirp(const std::string &chirp_id) {
  #ifdef DEBUG
  return chirpid_to_chirp_map_.erase(chirp_id);
  #else
  // TODO: grpc
  #endif /* DEBUG */
}
