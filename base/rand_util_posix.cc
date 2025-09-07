#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/posix/eintr_wrapper.h"
#include "base/rand_util.h"
#include "build/build_config.h"
#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <unistd.h>

#if BUILDFLAG(IS_MAC)
#include <sys/random.h>
#endif

namespace base {

namespace {

// We keep the file descriptor for /dev/urandom around so we don't need to reopen it (which is
// expensive), and since we may not even be able to reopen it if we are later put in a sandbox.
// This class wraps the file descriptor so we can use a static-local variable to handle opening it
// on the first access.
class URandomFd {
public:
    URandomFd() : fd_(HANDLE_EINTR(open("/dev/urandom", O_RDONLY | O_CLOEXEC))) {
        CHECK(fd_ >= 0);
    }

    ~URandomFd() { close(fd_); }

    int fd() const { return fd_; }

private:
    const int fd_;
};

int GetUrandomFD() {
    static NoDestructor<URandomFd> urandom_fd;
    return urandom_fd->fd();
}

}  // namespace

void rand_bytes(std::span<uint8_t> output) {
#if BUILDFLAG(IS_MAC)
    if (getentropy(output.data(), output.size()) == 0) {
        return;
    }
#elif BUILDFLAG(IS_LINUX)
    if (getrandom(output.data(), output.size())) {
        return;
    }
#endif

    // If the OS-specific mechanisms didn't work, fall through to reading from urandom.
    const int urandom_fd = GetUrandomFD();
    const bool success = ReadFromFD(urandom_fd, output);
    CHECK(success);
}

}  // namespace base
