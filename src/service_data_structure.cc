#include "service_data_structure.h"

#include <algorithm>
#include <memory>
#include <sys/time.h>

#include <glog/logging.h>

#include "backend_client_lib.h"

ServiceDataStructure::UserSession::UserSession(const std::string &username) {
  bool ok = chirp_connect_backend::GetUser(username, &(this->user_));
  CHECK(ok) << "User `" << username << "` should exist.";
}

bool ServiceDataStructure::UserSession::Follow(const std::string &username) {
  UserFollowingList following_list;
  bool ok = chirp_connect_backend::GetUserFollowingList(user_.username, &following_list);
  CHECK(ok) << "The user following list for user `" << username << "` should exist.";

  bool user_found = chirp_connect_backend::GetUser(username, nullptr);

  // The specifed user is found
  if (user_found) {
    following_list.insert(username);
    return chirp_connect_backend::SaveUserFollowingList(user_.username, following_list);
  } else {
    return false;
  }
}

bool ServiceDataStructure::UserSession::Unfollow(const std::string &username) {
  UserFollowingList following_list;
  bool ok = chirp_connect_backend::GetUserFollowingList(user_.username, &following_list);
  CHECK(ok) << "The user following list for user `" << username << "` should exist.";

  // If the specifed user is erased
  bool erased = following_list.erase(username);
  return erased && chirp_connect_backend::SaveUserFollowingList(user_.username, following_list);
}

uint64_t ServiceDataStructure::UserSession::PostChirp(
    const std::string &text,
    const uint64_t &parent_id) {

  struct Chirp chirp(user_.username, parent_id, text);

  // If the `parent_id` is specified
  if (parent_id > 0) {
    struct Chirp parent_chirp;
    bool parent_found = chirp_connect_backend::GetChirp(parent_id, &parent_chirp);
    if (!parent_found) {
      return 0;
    }

    parent_chirp.children_ids.push_back(chirp.id);
    bool ok = chirp_connect_backend::SaveChirp(parent_chirp.id, parent_chirp);
    if (!ok) {
      return 0;
    }
  }

  bool ok = chirp_connect_backend::SaveChirp(chirp.id, chirp);
  if (!ok) {
    return 0;
  }

  // Update the information of this user
  user_.last_update_chirp_time = chirp.time;
  ok = chirp_connect_backend::SaveUser(user_.username, user_);
  if (!ok) {
    return 0;
  }

  UserChirpList chirp_list;
  ok = chirp_connect_backend::GetUserChirpList(user_.username, &chirp_list);
  CHECK(ok) << "The user chirp list for user `" << user_.username << "` should exist.";
  chirp_list.insert(chirp.id);
  ok = chirp_connect_backend::SaveUserChirpList(user_.username, chirp_list);
  if (!ok) {
    return 0;
  }

  return chirp.id;
}

bool ServiceDataStructure::UserSession::EditChirp(
    const uint64_t &id,
    const std::string &text) {

  struct Chirp chirp;
  bool ok = chirp_connect_backend::GetChirp(id, &chirp);

  // If the chirp is found and its posting user is the user in this session
  if (ok && chirp.user == user_.username) {
    chirp.text = text;
    ok = chirp_connect_backend::SaveChirp(id, chirp);
    return ok;
  }

  return false;
}

bool ServiceDataStructure::UserSession::DeleteChirp(const uint64_t &id) {
  struct Chirp chirp;
  bool ok = chirp_connect_backend::GetChirp(id, &chirp);

  // If the chirp is found and its posting user is the user in this session
  if (ok && chirp.user == user_.username) {
    UserChirpList chirp_list;
    ok = chirp_connect_backend::GetUserChirpList(user_.username, &chirp_list);
    CHECK(ok) << "The user chirp list for user `" << user_.username << "` should exist.";

    chirp_list.erase(chirp.id);
    ok = chirp_connect_backend::DeleteChirp(id);
    ok &= chirp_connect_backend::SaveUserChirpList(user_.username, chirp_list);
    return ok;
  }

  return false;
}

std::set<uint64_t> ServiceDataStructure::UserSession::MonitorFrom(struct timeval * const from) {
  struct timeval now;
  gettimeofday(&now, nullptr);

  std::set<uint64_t> ret;

  UserFollowingList user_following_list;
  bool ok = chirp_connect_backend::GetUserFollowingList(user_.username, &user_following_list);
  CHECK(ok) << "The user following list for user `" << user_.username << "` should exist.";

  // TODO: may open threads to do the following things
  for(const auto &username : user_following_list) {
    struct User user;
    ok = chirp_connect_backend::GetUser(username, &user);
    CHECK(ok) << "User `" << username << "` should exist.";

    if (timercmp(&(user.last_update_chirp_time), from, >=)) {
      // Do push_backs
      UserChirpList user_chirp_list;
      ok = chirp_connect_backend::GetUserChirpList(user.username, &user_chirp_list);
      CHECK(ok) << "The user chirp list for user `" << user_.username << "` should exist.";

      for(const auto &chirp_id : user_chirp_list) {
        struct Chirp chirp;
        ok = chirp_connect_backend::GetChirp(chirp_id, &chirp);
        CHECK(ok) << "The chirp with chirp_id `" << chirp_id << "` should exist.";

        if (timercmp(&(chirp.time), from, >=) && timercmp(&(chirp.time), &now, <)) {
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

  bool user_found = chirp_connect_backend::GetUser(username, nullptr);
  // If the specified username is not found
  if (!user_found) {
    struct User new_user(username);
    UserChirpList chirp_list;
    UserFollowingList following_list;
    bool ok = chirp_connect_backend::SaveUser(username, new_user) &&
              chirp_connect_backend::SaveUserChirpList(username, chirp_list) &&
              chirp_connect_backend::SaveUserFollowingList(username, following_list);
    if (!ok) {
      chirp_connect_backend::DeleteUser(username);
      chirp_connect_backend::DeleteUserChirpList(username);
      chirp_connect_backend::DeleteUserFollowingList(username);
    }
    return ok;
  }

  return false;
}

std::unique_ptr<ServiceDataStructure::UserSession> ServiceDataStructure::UserLogin(const std::string &username) {
  bool user_found = chirp_connect_backend::GetUser(username, nullptr);

  // If the specified username is found
  if (user_found) {
    return std::unique_ptr<ServiceDataStructure::UserSession>(new UserSession(username));
  } else {
    return nullptr;
  }
}

ServiceDataStructure::User::User(const std::string &username) : username(username) {
  gettimeofday(&last_update_chirp_time, nullptr);
}

ServiceDataStructure::Chirp::Chirp(const std::string &user, const uint64_t &parent_id, const std::string &text)
    : user(user), parent_id(parent_id), text(text) {
  id = chirp_connect_backend::GetNextChirpId();
  gettimeofday(&time, nullptr);
}

// Type identifier for serializing data
const std::string kTypeNextChirpId({0, 0, 0, char(1)});
const std::string kTypeUsernameToUserPrefix({0, 0, 0, char(2)});
const std::string kTypeUsernameToFollowingPrefix({0, 0, 0, char(3)});
const std::string kTypeUsernameToChirpPrefix({0, 0, 0, char(4)});
const std::string kTypeChirpidToChirpPrefix({0, 0, 0, char(5)});

void chirp_connect_backend::DecomposeBinaryUser(const std::string &input, struct ServiceDataStructure::User * const user) {
  if (user != nullptr) {
    user->last_update_chirp_time = *(reinterpret_cast<const struct timeval *>(input.c_str()));
    user->username = input.substr(sizeof(struct timeval));
  }
}

void chirp_connect_backend::ComposeBinaryUser(const struct ServiceDataStructure::User &user, std::string * const output) {
  if (output != nullptr) {
    output->clear();
    output->append(reinterpret_cast<const char*>(&(user.last_update_chirp_time)),
                   sizeof(struct timeval));
    output->append(user.username);
  }
}

void chirp_connect_backend::DecomposeBinaryUserFollowing(const std::string &input, UserFollowingList * const following_list) {
  if (following_list != nullptr) {
    following_list->clear();

    const char* it = input.c_str();
    uint64_t list_len = *(reinterpret_cast<const uint64_t *>(it));

    it += sizeof(uint64_t);
    for(uint64_t i = 0; i < list_len; ++i) {
      uint64_t username_length = *(reinterpret_cast<const uint64_t*>(it));
      it += sizeof(uint64_t);
      following_list->insert(input.substr(it - input.c_str(), username_length));
      it += username_length;
    }
  }
}

void chirp_connect_backend::ComposeBinaryUserFollowing(const UserFollowingList &following_list, std::string * const output) {
  if (output != nullptr) {
    output->clear();

    uint64_t list_len = following_list.size();
    output->append(reinterpret_cast<const char *>(&list_len), sizeof(uint64_t));

    for(const std::string &username : following_list) {
      uint64_t username_length = username.size();
      output->append(reinterpret_cast<const char*>(&username_length), sizeof(uint64_t));
      output->append(username);
    }
  }
}

void chirp_connect_backend::DecomposeBinaryUserChirp(const std::string &input, UserChirpList * const chirp_list) {
  if (chirp_list != nullptr) {
    chirp_list->clear();

    const char *it = input.c_str();
    uint64_t list_len = *(reinterpret_cast<const uint64_t*>(it));

    it += sizeof(uint64_t);
    for(uint64_t i = 0; i < list_len; ++i, it += sizeof(uint64_t)) {
      chirp_list->insert(*(reinterpret_cast<const uint64_t*>(it)));
    }
  }
}

void chirp_connect_backend::ComposeBinaryUserChirp(const UserChirpList &chirp_list, std::string * const output) {
  if (output != nullptr) {
    output->clear();

    uint64_t list_length = chirp_list.size();
    output->append(reinterpret_cast<const char*>(&list_length), sizeof(uint64_t));

    for(const uint64_t &chirp_id : chirp_list) {
      output->append(reinterpret_cast<const char*>(&chirp_id), sizeof(uint64_t));
    }
  }
}

void chirp_connect_backend::DecomposeBinaryChirp(const std::string &input, struct ServiceDataStructure::Chirp * const chirp) {
  if (chirp != nullptr) {
    const char *it = input.c_str();
    chirp->id = *(reinterpret_cast<const uint64_t*>(it));
    it += sizeof(uint64_t);

    chirp->parent_id = *(reinterpret_cast<const uint64_t*>(it));
    it += sizeof(uint64_t);

    chirp->time = *(reinterpret_cast<const struct timeval*>(it));
    it += sizeof(struct timeval);

    uint64_t username_length = *(reinterpret_cast<const uint64_t*>(it));
    it += sizeof(uint64_t);

    chirp->user = std::string(it, username_length);
    it += username_length;

    uint64_t text_length = *(reinterpret_cast<const uint64_t*>(it));
    it += sizeof(uint64_t);

    chirp->text = std::string(it, text_length);
    it += text_length;

    chirp->children_ids.clear();
    uint64_t children_id_length = *(reinterpret_cast<const uint64_t*>(it));
    it += sizeof(uint64_t);
    for(uint64_t i = 0; i < children_id_length; ++i, it += sizeof(uint64_t)) {
      chirp->children_ids.push_back(*(reinterpret_cast<const uint64_t*>(it)));
    }
  }
}

void chirp_connect_backend::ComposeBinaryChirp(const struct ServiceDataStructure::Chirp &chirp, std::string * const output) {
  if (output != nullptr) {
    output->clear();

    output->append(reinterpret_cast<const char*>(&(chirp.id)), sizeof(uint64_t));
    output->append(reinterpret_cast<const char*>(&(chirp.parent_id)), sizeof(uint64_t));
    output->append(reinterpret_cast<const char*>(&(chirp.time)), sizeof(struct timeval));

    uint64_t username_length = chirp.user.size();
    output->append(reinterpret_cast<const char*>(&username_length), sizeof(uint64_t));
    output->append(chirp.user);

    uint64_t text_length = chirp.text.size();
    output->append(reinterpret_cast<const char*>(&text_length), sizeof(uint64_t));
    output->append(chirp.text);

    uint64_t children_id_length = chirp.children_ids.size();
    output->append(reinterpret_cast<const char*>(&children_id_length), sizeof(uint64_t));
    for(const uint64_t &child_id : chirp.children_ids) {
      output->append(reinterpret_cast<const char*>(&child_id), sizeof(uint64_t));
    }
  }
}

BackendClient backend_client_;

// Wrapper functions
// Wrapper function to get `next_chirp_id`
uint64_t chirp_connect_backend::GetNextChirpId() {
  std::vector<std::string> reply;
  bool ok = backend_client_.SendGetRequest(std::vector<std::string>({kTypeNextChirpId}), &reply);
  CHECK(ok) << "Get request should be successful.";

  // Get the next chirp id from backend
  uint64_t ret;
  if (!reply[0].empty()) {
    ret = *(reinterpret_cast<const uint64_t*>(reply[0].c_str()));
  } else {
    ret = 1;
  }

  // update the next chirp id to the backend
  ++ret;
  ok = backend_client_.SendPutRequest(
      kTypeNextChirpId,
      std::string(reinterpret_cast<const char*>(&ret), sizeof(uint64_t)));
  CHECK(ok) << "Put request should be successful.";
  return --ret;
}

// Wrapper function to get a specified user object
bool chirp_connect_backend::GetUser(const std::string &username, struct ServiceDataStructure::User * const user) {
  std::string key = kTypeUsernameToUserPrefix + username;
  std::vector<std::string> reply;
  bool ok = backend_client_.SendGetRequest(std::vector<std::string>(1, key), &reply);
  CHECK(ok) << "Get request should be successful.";
  if (!reply[0].empty()) {
    DecomposeBinaryUser(reply[0], user);
    return true;
  } else {
    return false;
  }
}

// Wrapper function to save a specified user object
bool chirp_connect_backend::SaveUser(const std::string &username, const struct ServiceDataStructure::User &user) {
  std::string key = kTypeUsernameToUserPrefix + username;
  std::string value;
  ComposeBinaryUser(username, &value);
  bool ok = backend_client_.SendPutRequest(key, value);
  return ok;
}

// Wrapper function to delete a specified user object
bool chirp_connect_backend::DeleteUser(const std::string &username) {
  std::string key = kTypeUsernameToUserPrefix + username;
  bool ok = backend_client_.SendDeleteKeyRequest(key);
  return ok;
}

// Wrapper function to get the following list of a specified user
bool chirp_connect_backend::GetUserFollowingList(const std::string &username, UserFollowingList * const following_list) {
  std::string key = kTypeUsernameToFollowingPrefix + username;
  std::vector<std::string> reply;
  bool ok = backend_client_.SendGetRequest(std::vector<std::string>(1, key), &reply);
  CHECK(ok) << "Get request should be successful.";
  if (!reply[0].empty()) {
    DecomposeBinaryUserFollowing(reply[0], following_list);
    return true;
  } else {
    return false;
  }
}

// Wrapper function to save the following list of a specified user
bool chirp_connect_backend::SaveUserFollowingList(const std::string &username, const UserFollowingList &following_list) {
  std::string key = kTypeUsernameToFollowingPrefix + username;
  std::string value;
  ComposeBinaryUserFollowing(following_list, &value);
  bool ok = backend_client_.SendPutRequest(key, value);
  return ok;
}

// Wrapper function to delete the following list of a specified user
bool chirp_connect_backend::DeleteUserFollowingList(const std::string &username) {
  std::string key = kTypeUsernameToFollowingPrefix + username;
  bool ok = backend_client_.SendDeleteKeyRequest(key);
  return ok;
}

// Wrapper function to get the chirp list of a specified user
bool chirp_connect_backend::GetUserChirpList(const std::string &username, UserChirpList * const chirp_list) {
  std::string key = kTypeUsernameToChirpPrefix + username;
  std::vector<std::string> reply;
  bool ok = backend_client_.SendGetRequest(std::vector<std::string>(1, key), &reply);
  CHECK(ok) << "Get request should be successful.";
  if (!reply[0].empty()) {
    DecomposeBinaryUserChirp(reply[0], chirp_list);
    return true;
  } else {
    return false;
  }
}

// Wrapper function to save the chirp list of a specified user
bool chirp_connect_backend::SaveUserChirpList(const std::string &username, const UserChirpList &chirp_list) {
  std::string key = kTypeUsernameToChirpPrefix + username;
  std::string value;
  ComposeBinaryUserChirp(chirp_list, &value);
  bool ok = backend_client_.SendPutRequest(key, value);
  return ok;
}

// Wrapper function to delete the chirp list of a specified user
bool chirp_connect_backend::DeleteUserChirpList(const std::string &username) {
  std::string key = kTypeUsernameToChirpPrefix + username;
  bool ok = backend_client_.SendDeleteKeyRequest(key);
  return ok;
}

// Wrapper function to get a chirp
bool chirp_connect_backend::GetChirp(const uint64_t &chirp_id, struct ServiceDataStructure::Chirp * const chirp) {
  std::string key = kTypeChirpidToChirpPrefix + std::string(reinterpret_cast<const char*>(&chirp_id), sizeof(uint64_t));
  std::vector<std::string> reply;
  bool ok = backend_client_.SendGetRequest(std::vector<std::string>(1, key), &reply);
  CHECK(ok) << "Get request should be successful.";
  if (!reply[0].empty()) {
    DecomposeBinaryChirp(reply[0], chirp);
    return true;
  } else {
    return false;
  }
}

// Wrapper function to save a chirp
bool chirp_connect_backend::SaveChirp(const uint64_t &chirp_id, const struct ServiceDataStructure::Chirp &chirp) {
  std::string key = kTypeChirpidToChirpPrefix + std::string(reinterpret_cast<const char*>(&chirp_id), sizeof(uint64_t));
  std::string value;
  ComposeBinaryChirp(chirp, &value);
  bool ok = backend_client_.SendPutRequest(key, value);
  return ok;
}

// Wrapper function to delete a chirp
bool chirp_connect_backend::DeleteChirp(const uint64_t &chirp_id) {
  std::string key = kTypeChirpidToChirpPrefix + std::string(reinterpret_cast<const char*>(&chirp_id), sizeof(uint64_t));
  bool ok = backend_client_.SendDeleteKeyRequest(key);
  return ok;
}
