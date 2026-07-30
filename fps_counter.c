// https://bugs.documentfoundation.org/show_bug.cgi?id=137468#c12

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

typedef int CGSConnectionID;

extern CGSConnectionID _CGSDefaultConnection(void);
extern int32_t CGSGetPerformanceData(
    CGSConnectionID cid, float* outFPS, float* unk, float* unk2, float* unk3);

int main(int argc, char** argv) {
    CGSConnectionID cid = _CGSDefaultConnection();
    for (;;) {
        float fps, f1, f2, f3;
        CGSGetPerformanceData(cid, &fps, &f1, &f2, &f3);
        printf("fps=%f\n", fps);
        sleep(1);
    }
}
