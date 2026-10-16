#pragma once
#include <cstring>

namespace scales {
struct HomeCredentials {
  char ssid[33]{};
  char password[65]{};
};

inline bool validHomeCredentials(const HomeCredentials &credentials) {
  if (!std::memchr(credentials.ssid, 0, sizeof(credentials.ssid)) || !credentials.ssid[0] ||
      !std::memchr(credentials.password, 0, sizeof(credentials.password))) return false;
  size_t length = std::strlen(credentials.password);
  if (!length || (length >= 8 && length <= 63)) return true;
  if (length != 64) return false;
  for (char c : credentials.password) {
    if (!c) break;
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return false;
  }
  return true;
}
} // namespace scales
