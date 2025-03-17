#ifndef BASE_STRINGS_TO_STRING_H_
#define BASE_STRINGS_TO_STRING_H_

#include <string>
#include <sstream>

namespace base {

template <typename T>
std::string ToString(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

} // namespace base

#endif // BASE_STRINGS_TO_STRING_H_