#include <curl/curl.h>

#include <cstdio>
#include <string_view>

namespace {

size_t WriteToStdout(char* contents, size_t size, size_t nmemb, void*) {
    const size_t byte_count = size * nmemb;
    std::fwrite(contents, 1, byte_count, stdout);
    return byte_count;
}

void PrintVersionInfo() {
    const curl_version_info_data* info = curl_version_info(CURLVERSION_NOW);

    std::printf("libcurl: %s\n", info->version);
    std::printf("host: %s\n", info->host);
    std::printf("ssl: %s\n", info->ssl_version ? info->ssl_version : "none");
    std::printf("zlib: %s\n", info->libz_version ? info->libz_version : "none");
}

int Fetch(std::string_view url) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::fprintf(stderr, "curl_easy_init failed\n");
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.data());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToStdout);

    const CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        std::fprintf(stderr, "curl_easy_perform failed: %s\n", curl_easy_strerror(result));
        curl_easy_cleanup(curl);
        return 1;
    }

    curl_easy_cleanup(curl);
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    const CURLcode global_init = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (global_init != CURLE_OK) {
        std::fprintf(stderr, "curl_global_init failed: %s\n", curl_easy_strerror(global_init));
        return 1;
    }

    PrintVersionInfo();

    int result = 0;
    if (argc > 1) {
        std::printf("\nFetching %s\n\n", argv[1]);
        result = Fetch(argv[1]);
    } else {
        std::printf("\nPass a URL to fetch it, for example: networking http://example.com\n");
    }

    curl_global_cleanup();
    return result;
}
