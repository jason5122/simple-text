#include "build/buildflag.h"

#if IS_LINUX
extern "C" int SimpleTextMain();
#else
int SimpleTextMain();
#endif

int main() {
    return SimpleTextMain();
}
