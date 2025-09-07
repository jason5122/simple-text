#include "base/files/file_util.h"

namespace base {

bool CloseFile(FILE* file) {
    if (file == nullptr) {
        return true;
    }
    return fclose(file) == 0;
}

}  // namespace base
