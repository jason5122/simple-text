#include "path_service.h"

#include <unordered_map>

#include "base/files/file_util.h"

namespace base {

namespace {

struct PathData {
    // TODO: Implement locking.
    // Lock lock;
    std::unordered_map<PathKey, FilePath> cache;
};

PathData* GetPathData() {
    static auto* path_data = new PathData();
    return path_data;
}

}  // namespace

bool PathService::get(PathKey key, FilePath* result) {
    PathData* path_data = GetPathData();

    auto it = path_data->cache.find(key);
    if (it != path_data->cache.end()) {
        *result = it->second;
        return true;
    }

    FilePath path = get_special_path(key);
    if (path.empty()) {
        return false;
    }
    if (path.ReferencesParent()) {
        // Make sure path service never returns a path with ".." in it.
        path = MakeAbsoluteFilePath(path);
        if (path.empty()) return false;
    }
    *result = path;
    path_data->cache[key] = path;
    return true;
}

}  // namespace base
