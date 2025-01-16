#pragma once

#include "base/files/file_path.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_POSIX)
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace base {

// Returns an absolute version of a relative path. Returns an empty path on
// error. This function can result in I/O so it can be slow.
//
// On POSIX, this function calls realpath(), so:
// 1) it fails if the path does not exist.
// 2) it expands all symlink components of the path.
// 3) it removes "." and ".." directory components.
FilePath MakeAbsoluteFilePath(const FilePath& input);

// Returns true if the given path exists on the local filesystem,
// false otherwise.
bool PathExists(const FilePath& path);

// Returns true if the given path is readable by the user, false otherwise.
bool PathIsReadable(const FilePath& path);

// Returns true if the given path is writable by the user, false otherwise.
bool PathIsWritable(const FilePath& path);

// Returns true if the given path exists and is a directory, false otherwise.
bool DirectoryExists(const FilePath& path);

// Reads the given |symlink| and returns the raw string in |target|.
// Returns false upon failure.
// IMPORTANT NOTE: if the string stored in the symlink is a relative file path,
// it should be interpreted relative to the symlink's directory, NOT the current
// working directory. ReadSymbolicLinkAbsolute() may be the better choice.
bool ReadSymbolicLink(const FilePath& symlink, FilePath* target);

// Get the temporary directory provided by the system.
//
// WARNING: In general, you should use CreateTemporaryFile variants below
// instead of this function. Those variants will ensure that the proper
// permissions are set so that other users on the system can't edit them while
// they're open (which can lead to security issues).
bool GetTempDir(FilePath* path);

// Get the home directory. This is more complicated than just getenv("HOME")
// as it knows to fall back on getpwent() etc.
//
// You should not generally call this directly. Instead use DIR_HOME with the
// path service which will use this function but cache the value.
// Path service may also override DIR_HOME.
FilePath GetHomeDir();

}  // namespace base
