#ifndef RIRUD_H
#define RIRUD_H

#include <cstdint>
#include <vector>
#include <string>
#include "buff_string.h"

class RakanSocket {
public:
    constexpr static std::string_view RIRUD = "rakan";

    enum class CODE : uint8_t {
        OK = 0,
        FAILED = 1,
    };

    bool valid() const {
        return fd_ != -1;
    }

    RakanSocket(unsigned retries = 1);

    template<typename T>
    std::enable_if_t<std::is_fundamental_v<T> || std::is_enum_v<T>, bool>
    Read(T &obj) const {
        return Read(reinterpret_cast<void *>(&obj), sizeof(T));
    }

    bool Read(std::string &str) const;

    template<typename T>
    std::enable_if_t<std::is_fundamental_v<T> || std::is_enum_v<T>, bool>
    Write(const T &obj) const {
        return Write(&obj, sizeof(T));
    }

    bool Write(std::string_view str) const;

    ~RakanSocket();

private:
    RakanSocket(const RakanSocket &) = delete;

    RakanSocket operator=(const RakanSocket &) = delete;

    bool Write(const void *buf, size_t len) const;

    bool Read(void *buf, size_t len) const;

    int fd_ = -1;
};

#endif //RIRUD_H