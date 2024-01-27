#import "build/buildflag.h"

#if IS_MAC
#import "main_mac.h"
#endif

int main() {
    return SimpleTextMain();
}
