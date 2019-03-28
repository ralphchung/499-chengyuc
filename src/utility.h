#ifndef CHIRP_UTILITY_H_
#define CHIRP_UTILITY_H_

#include <sys/time.h>
#include <string>

inline uint64_t BinaryToUint64(const std::string &input) {
  return *(reinterpret_cast<const uint64_t *>(input.c_str()));
}

inline const std::string Uint64ToBinary(const uint64_t &input) {
  return std::string(reinterpret_cast<const char *>(&input), sizeof(uint64_t));
}

// overload operators to make `struct timeval` comparison more readable
inline bool operator!=(const struct timeval &lhs, const struct timeval &rhs) {
  return timercmp(&lhs, &rhs, !=);
}

inline bool operator<(const struct timeval &lhs, const struct timeval &rhs) {
  return timercmp(&lhs, &rhs, <);
}

inline bool operator>=(const struct timeval &lhs, const struct timeval &rhs) {
  return timercmp(&lhs, &rhs, >=);
}

#endif /* CHIRP_UTILITY_H_ */