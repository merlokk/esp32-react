#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <mutex>

using LockGuard = std::lock_guard<std::mutex>;

#define PACKED __attribute__((packed))

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size <= 0 )
        return "";
    std::unique_ptr<char[]> buf( new char[ size ] );
    snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

#define MAC_FORMAT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_LIST_6_BYTES(value) value[0], value[1], value[2], value[3], value[4], value[5]

inline std::string string_format_mac(uint8_t *mac) {
    return string_format(MAC_FORMAT, MAC_LIST_6_BYTES(mac));
}

template <typename T>
void VectorMoveItemToBack(std::vector<T>& v, size_t itemIndex) {
    auto it = v.begin() + itemIndex;
    std::rotate(it, it + 1, v.end());
}

const char * const hexdigits = "0123456789abcdef";
template<typename T> std::string to_hexstring(T x){
    char *p = reinterpret_cast<char*>(&x);
    std::string s(2 * sizeof(T), '_');
    int k = 0;
    for (size_t i = sizeof(T); i--;) {
        s[k++] = hexdigits[(p[i] >> 4) & 0xf];
        s[k++] = hexdigits[(p[i]     ) & 0xf];
    }
    return s;
}

#endif // UTILS_H
