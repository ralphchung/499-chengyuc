#include "service_data_structure.h"

#include <sys/time.h>
#include <algorithm>
#include <memory>

#include <glog/logging.h>

#include "backend_client_lib.h"
#include "utility.h"

ServiceDataStructure::User::User(const std::string &username) {
  user_.set_username(username);

  // get current time
  gettimeofday(&last_update_, nullptr);
  // translate `struct timeval` to message `Timestamp` in proto definition
  auto from_protobuf = user_.mutable_last_update();
  from_protobuf->set_seconds(last_update_.tv_sec);
  from_protobuf->set_useconds(last_update_.tv_usec);
}

void ServiceDataStructure::User::ImportBinary(const std::string &input) {
  // Besides retrieving protobuf message, maintain the `last_update_`
  user_.ParseFromString(input);
  last_update_.tv_sec = user_.last_update().seconds();
  last_update_.tv_usec = user_.last_update().useconds();
}

const std::string ServiceDataStructure::User::ExportBinary() const {
  std::string ret;
  user_.SerializeToString(&ret);
  return ret;
}

void ServiceDataStructure::UserFollowingList::ImportBinary(
    const std::string &input) {
  // Temporary protobuf message to build this set
  ServiceData::UserFollowingList tmp;
  tmp.ParseFromString(input);

  this->clear();
  for (int i = 0; i < tmp.username_size(); ++i) {
    this->insert(tmp.username(i));
  }
}

const std::string ServiceDataStructure::UserFollowingList::ExportBinary()
    const {
  // Temporary protobuf message collecting all usernames in this set
  ServiceData::UserFollowingList tmp;
  for (const std::string &user : *this) {
    tmp.add_username(user);
  }

  std::string ret;
  tmp.SerializeToString(&ret);
  return ret;
}

void ServiceDataStructure::UserChirpList::ImportBinary(
    const std::string &input) {
  // Temporary protobuf message to build this set
  ServiceData::UserChirpList tmp;
  tmp.ParseFromString(input);

  this->clear();
  for (int i = 0; i < tmp.chirp_id_size(); ++i) {
    this->insert(tmp.chirp_id(i));
  }
}

const std::string ServiceDataStructure::UserChirpList::ExportBinary() const {
  // Temporary protobuf message collecting all chirp ids in this set
  ServiceData::UserChirpList tmp;
  for (const uint64_t &chirp_id : *this) {
    tmp.add_chirp_id(chirp_id);
  }

  std::string ret;
  tmp.SerializeToString(&ret);
  return ret;
}

ServiceDataStructure::Chirp::Chirp(const std::string &user,
                                   const uint64_t &parent_id,
                                   const std::string &text) {
  chirp_.set_id(chirp_connect_backend::GetNextChirpId());
  chirp_.set_username(user);
  chirp_.set_parent_id(parent_id);
  chirp_.set_text(text);
  gettimeofday(&time_, nullptr);
  chirp_.mutable_time()->set_seconds(time_.tv_sec);
  chirp_.mutable_time()->set_useconds(time_.tv_usec);
}

void ServiceDataStructure::Chirp::ImportBinary(const std::string &input) {
  chirp_.ParseFromString(input);

  // fill in children ids from the proto message
  for (int i = 0; i < chirp_.children_ids_size(); ++i) {
    children_ids_.insert(chirp_.children_ids(i));
  }

  // fill in struct timeval
  time_.tv_sec = chirp_.time().seconds();
  time_.tv_usec = chirp_.time().useconds();
}

const std::string ServiceDataStructure::Chirp::ExportBinary() const {
  // fill in children ids to the proto message
  // here discard qualifier since the need to copy the data from the maintained
  // `std::set` to the protobuf message
  const_cast<ServiceDataStructure::Chirp *>(this)->chirp_.clear_children_ids();
  for (const uint64_t &id : children_ids_) {
    const_cast<ServiceDataStructure::Chirp *>(this)->chirp_.add_children_ids(
        id);
  }

  std::string ret;
  chirp_.SerializeToString(&ret);

  return ret;
}

ServiceDataStructure::UserSession::UserSession(const std::string &username) {
  bool ok = chirp_connect_backend::GetUser(username, &(this->user_));
  CHECK(ok) << "User `" << username << "` should exist.";
}

ServiceDataStructure::ReturnCodes ServiceDataStructure::UserSession::Follow(
    const std::string &username) {
  UserFollowingList following_list;
  bool ok = chirp_connect_backend::GetUserFollowingList(user_.get_username(),
                                                        &following_list);
  CHECK(ok) << "The user following list for user `" << username
            << "` should exist.";

  bool user_found = chirp_connect_backend::GetUser(username, nullptr);

  // The specifed user is found
  if (!user_found) {
    return FOLLOWEE_NOT_FOUND;
  }

  following_list.insert(username);
  if (!chirp_connect_backend::SaveUserFollowingList(user_.get_username(),
                                                    following_list)) {
    return INTERNAL_BACKEND_ERROR;
  }
  return OK;
}

ServiceDataStructure::ReturnCodes ServiceDataStructure::UserSession::Unfollow(
    const std::string &username) {
  UserFollowingList following_list;
  bool ok = chirp_connect_backend::GetUserFollowingList(user_.get_username(),
                                                        &following_list);
  CHECK(ok) << "The user following list for user `" << username
            << "` should exist.";

  // If the specifed user is erased
  bool erased = following_list.erase(username);
  if (!erased) {
    return FOLLOWEE_NOT_FOUND;
  } else if (!chirp_connect_backend::SaveUserFollowingList(user_.get_username(),
                                                           following_list)) {
    return INTERNAL_BACKEND_ERROR;
  }

  return OK;
}

ServiceDataStructure::ReturnCodes ServiceDataStructure::UserSession::PostChirp(
    const std::string &text, uint64_t *const chirp_id,
    const uint64_t &parent_id) {
  Chirp chirp(user_.get_username(), parent_id, text);

  // If the `parent_id` is specified
  if (parent_id > 0) {
    Chirp parent_chirp;
    bool parent_found =
        chirp_connect_backend::GetChirp(parent_id, &parent_chirp);
    if (!parent_found) {
      return REPLY_ID_NOT_FOUND;
    }

    parent_chirp.insert_children_id(chirp.get_id());
    bool ok =
        chirp_connect_backend::SaveChirp(parent_chirp.get_id(), parent_chirp);
    if (!ok) {
      return INTERNAL_BACKEND_ERROR;
    }
  }

  bool ok = chirp_connect_backend::SaveChirp(chirp.get_id(), chirp);
  if (!ok) {
    return INTERNAL_BACKEND_ERROR;
  }

  // Update the information of this user
  user_.set_last_update(chirp.get_time());
  ok = chirp_connect_backend::SaveUser(user_.get_username(), user_);
  if (!ok) {
    return INTERNAL_BACKEND_ERROR;
  }

  UserChirpList chirp_list;
  ok = chirp_connect_backend::GetUserChirpList(user_.get_username(),
                                               &chirp_list);
  CHECK(ok) << "The user chirp list for user `" << user_.get_username()
            << "` should exist.";
  chirp_list.insert(chirp.get_id());
  ok = chirp_connect_backend::SaveUserChirpList(user_.get_username(),
                                                chirp_list);
  if (!ok) {
    return INTERNAL_BACKEND_ERROR;
  }

  if (chirp_id != nullptr) {
    *chirp_id = chirp.get_id();
  }
  return OK;
}

ServiceDataStructure::ReturnCodes ServiceDataStructure::UserSession::EditChirp(
    const uint64_t &id, const std::string &text) {
  Chirp chirp;
  bool ok = chirp_connect_backend::GetChirp(id, &chirp);

  if (!ok) {
    return CHIRP_ID_NOT_FOUND;
  } else if (chirp.get_username() != user_.get_username()) {
    return PERMISSION_DENIED;
  }

  // If the chirp is found and its posting user is the user in this session
  chirp.set_text(text);
  ok = chirp_connect_backend::SaveChirp(id, chirp);
  return OK;
}

ServiceDataStructure::ReturnCodes
ServiceDataStructure::UserSession::DeleteChirp(const uint64_t &id) {
  Chirp chirp;
  bool ok = chirp_connect_backend::GetChirp(id, &chirp);

  if (!ok) {
    return CHIRP_ID_NOT_FOUND;
  } else if (chirp.get_username() != user_.get_username()) {
    return PERMISSION_DENIED;
  }

  // If the chirp is found and its posting user is the user in this session
  UserChirpList chirp_list;
  ok = chirp_connect_backend::GetUserChirpList(user_.get_username(),
                                               &chirp_list);
  CHECK(ok) << "The user chirp list for user `" << user_.get_username()
            << "` should exist.";

  // if parent id is defined
  if (chirp.get_parent_id() > 0) {
    Chirp parent_chirp;
    bool parent_found =
        chirp_connect_backend::GetChirp(chirp.get_parent_id(), &parent_chirp);
    if (!parent_found) {
      return REPLY_ID_NOT_FOUND;
    }
    parent_chirp.erase_children_id(chirp.get_id());
    ok = chirp_connect_backend::SaveChirp(parent_chirp.get_id(), parent_chirp);
    if (!ok) {
      return INTERNAL_BACKEND_ERROR;
    }
  }

  chirp_list.erase(chirp.get_id());
  ok = chirp_connect_backend::DeleteChirp(id);
  ok &= chirp_connect_backend::SaveUserChirpList(user_.get_username(),
                                                 chirp_list);

  if (!ok) {
    return INTERNAL_BACKEND_ERROR;
  }
  return OK;
}

std::set<uint64_t> ServiceDataStructure::UserSession::MonitorFrom(
    struct timeval *const from) {
  struct timeval now;
  gettimeofday(&now, nullptr);

  std::set<uint64_t> ret;

  UserFollowingList user_following_list;
  bool ok = chirp_connect_backend::GetUserFollowingList(user_.get_username(),
                                                        &user_following_list);
  CHECK(ok) << "The user following list for user `" << user_.get_username()
            << "` should exist.";

  // TODO: may open threads to do the following things
  for (const auto &username : user_following_list) {
    struct User user;
    ok = chirp_connect_backend::GetUser(username, &user);
    CHECK(ok) << "User `" << username << "` should exist.";

    // to check if the `user.last_update_` is later or equal to the
    // `from`
    if (user.get_last_update() >= *from) {
      // Do push_backs
      UserChirpList user_chirp_list;
      ok = chirp_connect_backend::GetUserChirpList(user.get_username(),
                                                   &user_chirp_list);
      CHECK(ok) << "The user chirp list for user `" << user_.get_username()
                << "` should exist.";

      for (const auto &chirp_id : user_chirp_list) {
        Chirp chirp;
        ok = chirp_connect_backend::GetChirp(chirp_id, &chirp);
        CHECK(ok) << "The chirp with chirp_id `" << chirp_id
                  << "` should exist.";

        // to check if the `chirp.time` is later or equal to the `from` and
        // `chirp.time` is earlier than `now`
        if (chirp.get_time() >= *from && chirp.get_time() < now) {
          ret.insert(chirp_id);
        }
      }
    }
  }

  *from = now;
  return ret;
}

ServiceDataStructure::ReturnCodes ServiceDataStructure::UserRegister(
    const std::string &username) {
  // Invalid username
  if (username.empty()) {
    return INVALID_ARGUMENT;
  }

  bool user_found = chirp_connect_backend::GetUser(username, nullptr);
  if (user_found) {
    return USER_EXISTS;
  }

  // If the specified username is not found
  struct User new_user(username);
  UserChirpList chirp_list;
  UserFollowingList following_list;
  bool ok =
      chirp_connect_backend::SaveUser(username, new_user) &&
      chirp_connect_backend::SaveUserChirpList(username, chirp_list) &&
      chirp_connect_backend::SaveUserFollowingList(username, following_list);
  if (!ok) {
    chirp_connect_backend::DeleteUser(username);
    chirp_connect_backend::DeleteUserChirpList(username);
    chirp_connect_backend::DeleteUserFollowingList(username);
    return INTERNAL_BACKEND_ERROR;
  }

  return OK;
}

std::unique_ptr<ServiceDataStructure::UserSession>
ServiceDataStructure::UserLogin(const std::string &username) {
  bool user_found = chirp_connect_backend::GetUser(username, nullptr);

  if (!user_found) {
    return nullptr;
  }
  // If the specified username is found
  return std::unique_ptr<ServiceDataStructure::UserSession>(
      new UserSession(username));
}

// Type identifier for serializing data
const std::string kTypeNextChirpId({0, 0, 0, char(1)});
const std::string kTypeUsernameToUserPrefix({0, 0, 0, char(2)});
const std::string kTypeUsernameToFollowingPrefix({0, 0, 0, char(3)});
const std::string kTypeUsernameToChirpPrefix({0, 0, 0, char(4)});
const std::string kTypeChirpidToChirpPrefix({0, 0, 0, char(5)});

// Definition of `backend_client`
// The default version for this will communicate through grpc
std::unique_ptr<BackendClient> chirp_connect_backend::backend_client_(
    new BackendClientStandard());

// Wrapper functions
// Wrapper function to get `next_chirp_id`
uint64_t chirp_connect_backend::GetNextChirpId() {
  std::vector<std::string> reply;
  bool ok = chirp_connect_backend::backend_client_->SendGetRequest(
      std::vector<std::string>({kTypeNextChirpId}), &reply);
  CHECK(ok) << "Get request should be successful.";

  // Get the next chirp id from backend
  uint64_t ret;
  ServiceData::NowChirpId tmp;
  if (!reply[0].empty()) {
    tmp.ParseFromString(reply[0]);
    ret = tmp.now_id();
  } else {
    // No previous chirps
    ret = 0;
  }

  // update the next chirp id to the backend
  ++ret;
  tmp.set_now_id(ret);
  std::string binary;
  tmp.SerializeToString(&binary);
  ok = chirp_connect_backend::backend_client_->SendPutRequest(kTypeNextChirpId,
                                                              binary);
  CHECK(ok) << "Put request should be successful.";
  return ret;
}

// Wrapper function to get a specified user object
bool chirp_connect_backend::GetUser(
    const std::string &username,
    struct ServiceDataStructure::User *const user) {
  std::string key = kTypeUsernameToUserPrefix + username;
  std::vector<std::string> reply;
  bool ok = chirp_connect_backend::backend_client_->SendGetRequest(
      std::vector<std::string>(1, key), &reply);
  CHECK(ok) << "Get request should be successful.";
  if (reply[0].empty()) {
    return false;
  }

  if (user != nullptr) {
    user->ImportBinary(reply[0]);
  }
  return true;
}

// Wrapper function to save a specified user object
bool chirp_connect_backend::SaveUser(
    const std::string &username,
    const struct ServiceDataStructure::User &user) {
  std::string key = kTypeUsernameToUserPrefix + username;
  bool ok = chirp_connect_backend::backend_client_->SendPutRequest(
      key, user.ExportBinary());
  return ok;
}

// Wrapper function to delete a specified user object
bool chirp_connect_backend::DeleteUser(const std::string &username) {
  std::string key = kTypeUsernameToUserPrefix + username;
  bool ok = chirp_connect_backend::backend_client_->SendDeleteKeyRequest(key);
  return ok;
}

// Wrapper function to get the following list of a specified user
bool chirp_connect_backend::GetUserFollowingList(
    const std::string &username,
    ServiceDataStructure::UserFollowingList *const following_list) {
  std::string key = kTypeUsernameToFollowingPrefix + username;
  std::vector<std::string> reply;
  bool ok = chirp_connect_backend::backend_client_->SendGetRequest(
      std::vector<std::string>(1, key), &reply);

  if (!ok) {
    return false;
  }

  if (following_list != nullptr) {
    following_list->ImportBinary(reply[0]);
  }
  return true;
}

// Wrapper function to save the following list of a specified user
bool chirp_connect_backend::SaveUserFollowingList(
    const std::string &username,
    const ServiceDataStructure::UserFollowingList &following_list) {
  std::string key = kTypeUsernameToFollowingPrefix + username;
  bool ok = chirp_connect_backend::backend_client_->SendPutRequest(
      key, following_list.ExportBinary());
  return ok;
}

// Wrapper function to delete the following list of a specified user
bool chirp_connect_backend::DeleteUserFollowingList(
    const std::string &username) {
  std::string key = kTypeUsernameToFollowingPrefix + username;
  bool ok = chirp_connect_backend::backend_client_->SendDeleteKeyRequest(key);
  return ok;
}

// Wrapper function to get the chirp list of a specified user
bool chirp_connect_backend::GetUserChirpList(
    const std::string &username,
    ServiceDataStructure::UserChirpList *const chirp_list) {
  std::string key = kTypeUsernameToChirpPrefix + username;
  std::vector<std::string> reply;
  bool ok = chirp_connect_backend::backend_client_->SendGetRequest(
      std::vector<std::string>(1, key), &reply);
  if (!ok) {
    return false;
  }

  if (chirp_list != nullptr) {
    chirp_list->ImportBinary(reply[0]);
  }
  return true;
}

// Wrapper function to save the chirp list of a specified user
bool chirp_connect_backend::SaveUserChirpList(
    const std::string &username,
    const ServiceDataStructure::UserChirpList &chirp_list) {
  std::string key = kTypeUsernameToChirpPrefix + username;
  bool ok = chirp_connect_backend::backend_client_->SendPutRequest(
      key, chirp_list.ExportBinary());
  return ok;
}

// Wrapper function to delete the chirp list of a specified user
bool chirp_connect_backend::DeleteUserChirpList(const std::string &username) {
  std::string key = kTypeUsernameToChirpPrefix + username;
  bool ok = chirp_connect_backend::backend_client_->SendDeleteKeyRequest(key);
  return ok;
}

// Wrapper function to get a chirp
bool chirp_connect_backend::GetChirp(
    const uint64_t &chirp_id, struct ServiceDataStructure::Chirp *const chirp) {
  std::string key = kTypeChirpidToChirpPrefix + Uint64ToBinary(chirp_id);
  std::vector<std::string> reply;
  bool ok = chirp_connect_backend::backend_client_->SendGetRequest(
      std::vector<std::string>(1, key), &reply);
  CHECK(ok) << "Get request should be successful.";
  if (reply[0].empty()) {
    return false;
  }

  if (chirp != nullptr) {
    chirp->ImportBinary(reply[0]);
  }
  return true;
}

// Wrapper function to save a chirp
bool chirp_connect_backend::SaveChirp(
    const uint64_t &chirp_id, const struct ServiceDataStructure::Chirp &chirp) {
  std::string key = kTypeChirpidToChirpPrefix + Uint64ToBinary(chirp_id);
  bool ok = chirp_connect_backend::backend_client_->SendPutRequest(
      key, chirp.ExportBinary());
  return ok;
}

// Wrapper function to delete a chirp
bool chirp_connect_backend::DeleteChirp(const uint64_t &chirp_id) {
  std::string key = kTypeChirpidToChirpPrefix + Uint64ToBinary(chirp_id);
  bool ok = chirp_connect_backend::backend_client_->SendDeleteKeyRequest(key);
  return ok;
}
