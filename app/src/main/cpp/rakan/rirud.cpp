#include "rirud.h"
#include <malloc.h>
#include "socket.h"
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>
#include <errno.h>

bool RakanSocket::Write(std::string_view str) const {
    auto count = str.size();
    const auto *buf = str.data();
    return Write<uint32_t>(str.size()) && Write(buf, count);
}

bool RakanSocket::Read(std::string &str) const {
    uint32_t size;
    if (!Read(size) || size < 0) return false;
    str.resize(size);
    return Read(str.data(), size);
}

RakanSocket::RakanSocket(unsigned retries) {
    if ((fd_ = socket(PF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0) {
        return;
    }

    struct sockaddr_un addr{
            .sun_family = AF_UNIX,
            .sun_path={0}
    };
    strncpy(addr.sun_path + 1, RIRUD.data(), RIRUD.size());
    socklen_t socklen = sizeof(sa_family_t) + strlen(addr.sun_path + 1) + 1;

    while (retries-- > 0) {
        if (connect(fd_, reinterpret_cast<struct sockaddr *>(&addr), socklen) != -1) return;
        sleep(1);
    }
    close(fd_);
    fd_ = -1;
}

RakanSocket::~RakanSocket() {
    if (fd_ != -1) {
        close(fd_);
    }
}

bool RakanSocket::Write(const void *buf, size_t len) const {
    auto count = len;
    while (count > 0) {
        ssize_t size = write(fd_, buf, count < SSIZE_MAX ? count : SSIZE_MAX);
        if (size == -1) {
            if (errno == EINTR) continue;
            else return false;
        }
        buf = static_cast<const char *>(buf) + size;
        count -= size;
    }
    return true;
}

bool RakanSocket::Read(void *out, size_t len) const {
    while (len > 0) {
        ssize_t ret = read(fd_, out, len);
        if (ret <= 0) {
            if (errno == EINTR) continue;
            else return false;
        }
        out = static_cast<char *>(out) + ret;
        len -= ret;
    }
    return true;
}