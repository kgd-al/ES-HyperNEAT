#ifndef KGD_FIXED_SIZE_STRING_HPP
#define KGD_FIXED_SIZE_STRING_HPP

#include <array>

namespace utils {

template <size_t SIZE>
class fixed_string {
  std::array<char, SIZE> _data;

public:
  fixed_string (void) {
    _data.fill('\0');
  }

  fixed_string (const char *data) : fixed_string() {
    std::copy_n(data, std::min(strlen(data), SIZE), _data.begin());
  }
  fixed_string (const fixed_string &that) = default;
  fixed_string (fixed_string &&that) = default;

  explicit fixed_string (const std::string &s) : fixed_string(s.data()) {}

  ~fixed_string (void) = default;

  fixed_string& operator= (const fixed_string &that) = default;
  fixed_string& operator= (fixed_string &&that) = default;

  explicit operator std::string (void) const {
    auto last = std::prev(_data.end());
    while (*last == '\0') {
      if (last == _data.begin())  return std::string();
      --last;
    }
    return std::string(_data.begin(), std::next(last));
  }

  bool empty (void) const {
    return _data.front() == '\0';
  }

  friend void to_json(nlohmann::json &j, const fixed_string &s) {
    j = (std::string)s;
  }

  friend void from_json(const nlohmann::json &j, fixed_string &s) {
    s = fixed_string(j.get<std::string>());
  }

  friend std::ostream& operator<< (std::ostream &os, const fixed_string &s) {
    return os << (std::string)s;
  }

  friend std::istream& operator>> (std::istream &is, fixed_string &fs) {
    std::string s;
    is >> s;
    fs = fixed_string(s);
    return is;
  }

  friend bool operator< (const fixed_string &lhs, const fixed_string &rhs) {
    return (std::string)lhs < (std::string)rhs;
  }

  friend bool operator== (const fixed_string &lhs, const fixed_string &rhs) {
    return lhs._data == rhs._data;
  }

  friend bool operator!= (const fixed_string &lhs, const fixed_string &rhs) {
    return !(lhs == rhs);
  }

  friend void assertEqual (const fixed_string &lhs, const fixed_string &rhs,
                           bool &ok) {
    using utils::assertEqual;
    assertEqual(lhs._data, rhs._data, ok);
  }
};

} // end of namespace utils

#endif // KGD_FIXED_SIZE_STRING_HPP
