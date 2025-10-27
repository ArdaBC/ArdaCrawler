#include <curl/curl.h>
#include <iostream>

int main() {
    std::cout << "libcurl version: " << curl_version() << std::endl;
    return 0;
}
