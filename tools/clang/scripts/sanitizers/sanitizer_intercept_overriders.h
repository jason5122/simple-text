// Workaround for https://crbug.com/1302636#c49, #c55
#ifdef SANITIZER_INTERCEPT_CRYPT_R
#undef SANITIZER_INTERCEPT_CRYPT_R
#define SANITIZER_INTERCEPT_CRYPT_R 0
#endif