#ifndef STRING_FORMAT_H
#define STRING_FORMAT_H

#include <string>
#include <memory>

namespace util {

    template<typename ... Args>
    std::string string_format(std::string_view format, Args ... args) {
        INT size_s = std::snprintf(nullptr, 0, format.data(), args ...) + 1; // Extra space for '\0'
        if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
        auto                    size = static_cast<SIZE_T>( size_s );
        std::unique_ptr<char[]> buf(new char[size]);
        std::snprintf(buf.get(), size, format.data(), args ...);
        return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    }

} // namespace util
#endif //STRING_FORMAT_H