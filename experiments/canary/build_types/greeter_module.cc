#if defined(_WIN32)
#define DEMO_EXPORT __declspec(dllexport)
#else
#define DEMO_EXPORT __attribute__((visibility("default")))
#endif

extern "C" DEMO_EXPORT const char* ModuleGreeting() { return "hello from a loadable_module"; }
