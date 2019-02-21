#include "backend_data_structure.h"

BackendDataStructure::BackendDataStructure() : key_value_map_() {}

bool BackendDataStructure::Put(const std::string &key,
                               const std::string &value) {
  key_value_map_[key] = value;
  return true;
}

bool BackendDataStructure::Get(const std::string &key,
                               std::string *output_value) {
  auto it = key_value_map_.find(key);
  if (it != key_value_map_.end()) {
    if (output_value != nullptr) {
      *output_value = it->second;
    }
    return true;
  } else {
    return false;
  }
}

bool BackendDataStructure::DeleteKey(const std::string &key) {
  auto ok = key_value_map_.erase(key);
  return ok;
}
