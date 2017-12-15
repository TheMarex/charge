#ifndef CHARGE_COMMON_FILE_HPP
#define CHARGE_COMMON_FILE_HPP

#include <cstdint>
#include <fstream>
#include <string>

namespace charge {
namespace common {

inline bool file_exists(const std::string &path)
{
    std::ifstream s(path);
    return s.good();
}

class BinaryReader {
   public:
    BinaryReader(const std::string &path) : stream(path, std::ios::binary) {
        stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    }

    template <typename T>
    void read(T& value, const std::size_t count) {
        stream.read(reinterpret_cast<char*>(&value), sizeof(T) * count);
    }

    std::size_t position() {
        return stream.tellg();
    }

    std::size_t size() {
        stream.seekg(0, std::ios::end);
        auto size = stream.tellg();
        stream.seekg(0, std::ios::beg);
        return size;
    }

   private:
    std::ifstream stream;
};

class BinaryWriter {
   public:
    BinaryWriter(const std::string &path) : stream(path, std::ios::binary) {
        stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    }

    template <typename T>
    void write(const T& value, const std::size_t count) {
        stream.write(reinterpret_cast<const char*>(&value), sizeof(T) * count);
    }

    std::size_t position() {
        return stream.tellp();
    }

   private:
    std::ofstream stream;
};
}
}

#endif
