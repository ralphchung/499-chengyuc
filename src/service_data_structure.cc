#include "service_data_structure.h"

#include <climits>
#include <sys/time.h>
#include <sstream>

ServiceDataStructure::UserSession::UserSession(struct User * const user)
    : user_(user) {}

bool ServiceDataStructure::UserSession::Follow(const std::string &username) {
  // The specifed user is found
  if (username_to_user_map_.find(username) != username_to_user_map_.end()) {
    user_->following_list.insert(username);
    return true;
  } else {
    return false;
  }
}

bool ServiceDataStructure::UserSession::Unfollow(const std::string &username) {
  // If the specifed user is erased
  // the return value will be greater than 0
  // which is true
  return (user_->following_list.erase(username));
}

struct ServiceDataStructure::Chirp* const ServiceDataStructure::UserSession::PostChirp(
    const std::string &text,
    const std::string &parent_id) {

  struct Chirp *chirp = GenerateNewChirp();

  // The generation fails
  if (chirp == nullptr) {
    return nullptr;
  }

  // If the `parent_id` is specified
  if (!parent_id.empty()) {
    chirpid_to_chirp_map_[parent_id].children_ids.push_back(chirp->id);
  }

  chirp->user = user_->username;
  chirp->parent_id = parent_id;
  chirp->text = text;

  user_->chirp_list.insert(chirp->id);

  return chirp;
}

struct ServiceDataStructure::Chirp* const ServiceDataStructure::UserSession::EditChirp(
    const std::string &id,
    const std::string &text) {

  auto it = chirpid_to_chirp_map_.find(id);

  // If the chirp is found and its posting user is the user in this session
  if (it != chirpid_to_chirp_map_.end() && it->second.user == user_->username) {
    it->second.text = text;
    return &(it->second);
  } else {
    return nullptr;
  }
}

bool ServiceDataStructure::UserSession::DeleteChirp(const std::string &id) {
  auto it = chirpid_to_chirp_map_.find(id);

  // If the chirp is found and its posting user is the user in this session
  if (it != chirpid_to_chirp_map_.end() && it->second.user == user_->username) {
    user_->chirp_list.erase(id);
    chirpid_to_chirp_map_.erase(it);
    return true;
  } else {
    return false;
  }
}

bool ServiceDataStructure::UserRegister(const std::string &username) {
  // Invalid username
  if (username.empty()) {
    return false;
  }

  // If the specified username is not found
  if (username_to_user_map_.count(username) == 0) {
    auto ret = username_to_user_map_.emplace(username, User(username));
    // returns true if emplace succeeds
    return (ret.second);
  } else {
    return false;
  }
}

std::unique_ptr<ServiceDataStructure::UserSession> ServiceDataStructure::UserLogin(const std::string &username) {
  auto it = username_to_user_map_.find(username);

  // If the specified username is found
  if (it != username_to_user_map_.end()) {
    return std::unique_ptr<ServiceDataStructure::UserSession>(new ServiceDataStructure::UserSession(&(it->second)));
  } else {
    return nullptr;
  }
}

struct ServiceDataStructure::Chirp const *ServiceDataStructure::ReadChirp(const std::string &id) {
  auto it = chirpid_to_chirp_map_.find(id);

  // If the specified chirp is found
  if (it != chirpid_to_chirp_map_.end()) {
    return &(it->second);
  } else {
    return nullptr;
  }
}

ServiceDataStructure::User::User(const std::string &username)
    : username(username),
    following_list(),
    chirp_list() {}

void ServiceDataStructure::IncreaseNextChirpId() {
  if (next_chirp_id_.back() < CHAR_MAX) {
    ++next_chirp_id_.back();
  } else {
    next_chirp_id_.push_back(CHAR_MIN);
  }
}

struct ServiceDataStructure::Chirp *ServiceDataStructure::GenerateNewChirp() {
  struct timeval time;
  gettimeofday(&time, nullptr);

  chirpid_to_chirp_map_[next_chirp_id_] = {
    next_chirp_id_,
    "", // blank user field
    "", // blank parent user field
    "", // blank text field
    {time.tv_sec, time.tv_usec}
  };

  struct ServiceDataStructure::Chirp *ret = &(chirpid_to_chirp_map_[next_chirp_id_]);
  IncreaseNextChirpId();

  return ret;
}

// Static members initializations
std::string ServiceDataStructure::next_chirp_id_({CHAR_MIN});
std::map<std::string, struct ServiceDataStructure::User> ServiceDataStructure::username_to_user_map_;
std::unordered_map<std::string, struct ServiceDataStructure::Chirp> ServiceDataStructure::chirpid_to_chirp_map_;
