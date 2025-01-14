#include "file_path.h"

namespace base {

namespace {
const FilePath::CharType kStringTerminator = FILE_PATH_LITERAL('\0');
}

FilePath::FilePath(StringPieceType path) : path_(path) {
    auto pos = path_.find(kStringTerminator);
    if (pos != StringType::npos) {
        path_.erase(pos);
    }
}

const FilePath::StringType& FilePath::value() const {
    return path_;
}

bool FilePath::empty() const {
    return path_.empty();
}

void FilePath::clear() {
    path_.clear();
}

}  // namespace base
