#ifndef DOWNLOADER_HPP
#define DOWNLOADER_HPP

#include "thread_pool.hpp"
#include <curl/curl.h>
#include <fstream>
#include <string>
#include <queue>
#include <filesystem>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <functional>


class Downloader {

    public:

        static Downloader& instance(ThreadPool& pool,
                                    const std::string& download_dir = "downloads",
                                    const std::string& user_agent = "Downloader/1.0") {

            std::filesystem::path ca_path = std::filesystem::current_path() / "external" / "curl" / "cacert.pem";
            static Downloader downloader(pool, ca_path.string(), download_dir, user_agent);

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
                curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent_.c_str());

                res = curl_easy_perform(curl);

                if (res == CURLE_OK) {
                    savePage(website, response);
                }
                else{
                    //use logger?
                }

                curl_easy_cleanup(curl);
            }

        }

        static std::string toShortHex(std::size_t h) {
            std::stringstream ss;
            ss << std::hex << std::setw(8) << std::setfill('0') << (h & 0xffffffff);
            return ss.str();
        }

        static std::string sanitizeComponent(const std::string& in) {
            std::string out;
            out.reserve(in.size());
            for (unsigned char uc : in) {
                if (std::isalnum(uc) || uc == '.' || uc == '-' || uc == '_') {
                    out.push_back(static_cast<char>(uc));
                } 
                else {
                    if (out.empty() || out.back() != '_') out.push_back('_');
                }
            }
            while (!out.empty() && out.front() == '_') {
                out.erase(out.begin());
            }
            while (!out.empty() && out.back() == '_') {
                out.pop_back();
            }
            if (out.empty()) {
                out = "x";
            }
            return out;
        }

        static std::string urlToFilename(const std::string& url) {

            std::string s = url;

            auto pos = s.find("://");
            if (pos != std::string::npos) s = s.substr(pos + 3);

            pos = s.find('#');
            if (pos != std::string::npos) s = s.substr(0, pos);

            std::string query;
            pos = s.find('?');
            if (pos != std::string::npos) {
                query = s.substr(pos + 1);
                s = s.substr(0, pos);
            }

            std::string host;
            std::string path;
            pos = s.find('/');
            if (pos == std::string::npos) {
                host = s;
                path = "/";
            } 
            else {
                host = s.substr(0, pos);
                path = s.substr(pos);
            }

            std::string host_l;
            host_l.reserve(host.size());
            for (unsigned char c : host) host_l.push_back(static_cast<char>(std::tolower(c)));

            std::string host_s = sanitizeComponent(host_l);

            std::vector<std::string> segments;
            size_t i = 0;
            while (i < path.size()) {
                while (i < path.size() && path[i] == '/') {
                    ++i;
                }
                if (i >= path.size()) {
                    break;
                }
                size_t j = i;
                while (j < path.size() && path[j] != '/') {
                    ++j;
                }
                std::string seg = path.substr(i, j - i);
                segments.push_back(sanitizeComponent(seg));
                i = j;
            }

            std::string base = host_s;
            if (!segments.empty()) {
                base += '_';
                for (size_t k = 0; k < segments.size(); ++k) {
                    if (k) {
                        base += '_';
                    }
                    base += segments[k];
                }
            } 
            else {
                if (!query.empty()) {
                    base += "_index";
                }
            }

            std::size_t h = std::hash<std::string>{}(url);
            std::string short_hash = toShortHex(h);

            const bool needHash = (!query.empty() || !segments.empty());

            const size_t MAX_BASE_LEN = 200;

            if (base.size() > MAX_BASE_LEN) {
                
                base = base.substr(0, MAX_BASE_LEN);
                if (!base.empty() && base.back() == '_') {
                    base.pop_back();
                }
            }

            std::string filename = base;
            if (needHash) {
                filename += '_';
                filename += short_hash;
            }

            while (!filename.empty() && (filename.front() == '_' || filename.front() == '.')) {
                filename.erase(filename.begin());
            }
            while (!filename.empty() && (filename.back() == '_' || filename.back() == '.')) {
                filename.pop_back();
            }
            if (filename.empty()) {
                filename = "page";
            }

            filename += ".html";
            return filename;
        }
        

        void savePage(const std::string& website, const std::string& response){
            //To do: sqlite, json?

            const std::string filename = urlToFilename(website);

            std::filesystem::path dir(download_dir_);

            std::error_code ec;
            std::filesystem::create_directories(dir, ec);
            if (ec) {
                //use logger?
                return;
            }

            std::filesystem::path filePath = dir / filename;

            std::ofstream ofs(filePath, std::ios::binary);
            if (!ofs) {
                //use logger?
                return;
            }

            ofs.write(response.data(), static_cast<std::streamsize>(response.size()));

            ofs.close();
        }
        

        Downloader(ThreadPool& pool, const std::string& ca_path_str, 
                    const std::string& download_dir, const std::string& user_agent): 
                    pool_(pool), ca_path_str_(ca_path_str), 
                    download_dir_(download_dir), user_agent_(user_agent) {
            curl_global_init(CURL_GLOBAL_DEFAULT);
        }

        ThreadPool& pool_;
        std::string ca_path_str_;
        std::string download_dir_;
        std::string user_agent_;

};

#endif