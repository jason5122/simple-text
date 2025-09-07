#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/posix/eintr_wrapper.h"
#include <fcntl.h>
#include <unistd.h>

namespace base {

FilePath MakeAbsoluteFilePath(const FilePath& input) {
    char full_path[PATH_MAX];
    if (realpath(input.value().c_str(), full_path) == nullptr) {
        return FilePath();
    }
    return FilePath(full_path);
}

bool PathExists(const FilePath& path) { return access(path.value().c_str(), F_OK) == 0; }

bool PathIsReadable(const FilePath& path) { return access(path.value().c_str(), R_OK) == 0; }

bool PathIsWritable(const FilePath& path) { return access(path.value().c_str(), W_OK) == 0; }

bool DirectoryExists(const FilePath& path) {
    stat_wrapper_t file_info;
    if (File::Stat(path, &file_info) != 0) {
        return false;
    }
    return S_ISDIR(file_info.st_mode);
}

bool ReadSymbolicLink(const FilePath& symlink_path, FilePath* target_path) {
    char buf[PATH_MAX];
    ssize_t count = ::readlink(symlink_path.value().c_str(), buf, std::size(buf));

    bool error = count <= 0;
    if (error) {
        target_path->clear();
        return false;
    }

    *target_path = FilePath(FilePath::StringType(buf, static_cast<size_t>(count)));
    return true;
}

namespace {

#if !BUILDFLAG(IS_MAC)
// Appends |mode_char| to |mode| before the optional character set encoding; see
// https://www.gnu.org/software/libc/manual/html_node/Opening-Streams.html for
// details.
std::string AppendModeCharacter(std::string_view mode, char mode_char) {
    std::string result(mode);
    size_t comma_pos = result.find(',');
    result.insert(comma_pos == std::string::npos ? result.length() : comma_pos, 1, mode_char);
    return result;
}
#endif

}  // namespace

FILE* OpenFile(const FilePath& filename, const char* mode) {
    // 'e' is unconditionally added below, so be sure there is not one already
    // present before a comma in |mode|.
    FILE* result = nullptr;
#if BUILDFLAG(IS_MAC)
    // macOS does not provide a mode character to set O_CLOEXEC; see
    // https://developer.apple.com/legacy/library/documentation/Darwin/Reference/ManPages/man3/fopen.3.html.
    const char* the_mode = mode;
#else
    std::string mode_with_e(AppendModeCharacter(mode, 'e'));
    const char* the_mode = mode_with_e.c_str();
#endif
    do {
        result = fopen(filename.value().c_str(), the_mode);
    } while (!result && errno == EINTR);
#if BUILDFLAG(IS_MAC)
    // Mark the descriptor as close-on-exec.
    if (result) {
        SetCloseOnExec(fileno(result));
    }
#endif
    return result;
}

bool ReadFromFD(int fd, std::span<uint8_t> buffer) {
    while (!buffer.empty()) {
        ssize_t bytes_read = HANDLE_EINTR(read(fd, buffer.data(), buffer.size()));

        if (bytes_read <= 0) {
            return false;
        }
        buffer = buffer.subspan(bytes_read);
    }
    return true;
}

bool SetCloseOnExec(int fd) {
    const int flags = fcntl(fd, F_GETFD);
    if (flags == -1) {
        return false;
    }
    if (flags & FD_CLOEXEC) {
        return true;
    }
    if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1) {
        return false;
    }
    return true;
}

}  // namespace base
