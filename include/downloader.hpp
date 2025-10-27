#ifndef DOWNLOADER_HPP
#define DOWNLOADER_HPP

#include "thread_pool.hpp"
#include <curl/curl.h>
#include <fstream>
#include <string>
#include <queue>
#include <filesystem>
#include <mutex>


class Downloader {

    public:

        static Downloader& instance(ThreadPool& pool){

            std::filesystem::path ca_path = std::filesystem::current_path() / "external" / "curl" / "cacert.pem";
            static Downloader downloader(pool, ca_path.string());

            return downloader;
        }

        Downloader(const Downloader&) = delete;
        Downloader& operator=(const Downloader&) = delete;

        ~Downloader() {
            curl_global_cleanup();
        }

        void enqueue(const std::string& website) {

            pool_.enqueue(std::bind(&Downloader::download, this, website));

        }

    private:

        static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
            size_t totalSize = size * nmemb;
            std::string* body = static_cast<std::string*>(userp);
            body->append((char*)contents, totalSize);
            return totalSize;
        }

        void download(const std::string& website) {

            CURL* curl;
            CURLcode res;
            std::string response;

            curl = curl_easy_init();

            if (curl) {
                curl_easy_setopt(curl, CURLOPT_URL, website.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
                curl_easy_setopt(curl, CURLOPT_CAINFO, ca_path_str_.c_str());
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);

                res = curl_easy_perform(curl);

                if (res == CURLE_OK) {
                    savePage(website, response);
                }

                curl_easy_cleanup(curl);
            }

        }

        // "https://google.com" -> "google.com.html"
        static std::string urlToFilename(const std::string& url) {
            std::string s = url;

            // remove http:// or https://
            auto pos = s.find("://");
            if (pos != std::string::npos) s = s.substr(pos + 3);

            // cut off query and fragment
            pos = s.find_first_of("?#");
            if (pos != std::string::npos) s = s.substr(0, pos);

            // remove trailing slashes
            while (!s.empty() && s.back() == '/') s.pop_back();

            // replace any character that's not alnum, '.', '-', '_' with '_'
            std::string out;
            out.reserve(s.size());
            for (char c : s) {
                if (std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '-' || c == '_') {
                    out.push_back(c);
                } else {
                    out.push_back('_');
                }
            }

            if (out.empty()) out = "page";
            out += ".html";
            return out;
        }


        void savePage(const std::string& website, const std::string& response){
            //To do: sqlite, json?

            const std::string filename = urlToFilename(website);

            std::ofstream ofs(filename, std::ios::binary);
            if (!ofs) {
                return;
            }

            ofs.write(response.data(), static_cast<std::streamsize>(response.size()));

            ofs.close();
        }
        

        Downloader(ThreadPool& pool, const std::string& ca_path_str) 
                    : pool_(pool), ca_path_str_(ca_path_str) {
            curl_global_init(CURL_GLOBAL_DEFAULT);
        }

        ThreadPool& pool_;
        std::string ca_path_str_;

};

#endif