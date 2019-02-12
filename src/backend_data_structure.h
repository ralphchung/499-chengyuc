#ifndef CHIRP_SRC_BACKEND_DATA_STRUCTURE_H_
#define CHIRP_SRC_BACKEND_DATA_STRUCTURE_H_

#include <map>
#include <string>

// This is the backend data structure.
// It stores the key-value mapping
// It takes [get, put, deletekey] operations
class BackendDataStructure {
 public:
  BackendDataStructure();
  virtual ~BackendDataStructure();

  // Put operation
  // returns true if this operation succeeds
  // returns false otherwise
  bool Put(const std::string &key, const std::string &value);
  // Get operation
  // This is a single get operation instead of a stream of get operations
  // returns true if this operation succeeds
  // returns false otherwise
  bool Get(const std::string &key, std::string *output_value);
  // Delete key operation
  // returns true if this operation succeeds
  // returns false otherwise
  bool DeleteKey(const std::string &key);
 private:
  // This is where the data store
  std::map<std::string, std::string> key_value_map_;
};

#endif /* CHIRP_SRC_BACKEND_DATA_STRUCTURE_H_ */
