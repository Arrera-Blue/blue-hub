#include "flathub_service.hpp"
#include <curl/curl.h>
#include <iostream>
#include <regex>
#include <filesystem>

namespace bluehub::core {

namespace {
    size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t total_size = size * nmemb;
        std::string* str = static_cast<std::string*>(userp);
        str->append(static_cast<char*>(contents), total_size);
        return total_size;
    }

    std::string http_get(const std::string& url) {
        CURL* curl = curl_easy_init();
        std::string response;
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 4L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "BlueHub/1.0");
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }
        return response;
    }

    bool download_file(const std::string& url, const std::string& dest_path) {
        if (std::filesystem::exists(dest_path) && std::filesystem::file_size(dest_path) > 0) {
            return true; // Déjà en cache
        }

        CURL* curl = curl_easy_init();
        if (!curl) return false;

        FILE* fp = fopen(dest_path.c_str(), "wb");
        if (!fp) {
            curl_easy_cleanup(curl);
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 4L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "BlueHub/1.0");

        CURLcode res = curl_easy_perform(curl);
        fclose(fp);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK || std::filesystem::file_size(dest_path) == 0) {
            std::filesystem::remove(dest_path);
            return false;
        }
        return true;
    }

    std::string extract_field(const std::string& obj, const std::string& key) {
        std::string pattern = "\"" + key + "\"\\s*:\\s*\"([^\"]*)\"";
        std::regex reg(pattern);
        std::smatch m;
        if (std::regex_search(obj, m, reg) && m.size() > 1) {
            return m[1].str();
        }
        return "";
    }
}

std::vector<TrendingApp> FlathubService::get_trending_apps(int count) {
    std::vector<TrendingApp> apps;

    std::string url = "https://flathub.org/api/v2/collection/trending?page=1&per_page=" + std::to_string(count);
    std::string json = http_get(url);

    if (json.empty()) {
        return apps;
    }

    // Dossier cache pour stocker les logos Flathub téléchargés
    std::string cache_dir = "/tmp/blue-hub/cache/icons";
    try {
        std::filesystem::create_directories(cache_dir);
    } catch (...) {}

    const std::vector<std::string> palette = {
        "slide-vlc", "slide-blender", "slide-godot", "slide-vscode", "slide-gimp"
    };

    std::regex block_regex(R"raw(\{[^{}]+\})raw");
    auto it = std::sregex_iterator(json.begin(), json.end(), block_regex);
    auto end = std::sregex_iterator();

    size_t idx = 0;
    for (; it != end && static_cast<int>(apps.size()) < count; ++it) {
        std::string block = it->str();
        std::string id = extract_field(block, "id");
        std::string name = extract_field(block, "name");
        std::string summary = extract_field(block, "summary");
        std::string icon = extract_field(block, "icon");

        if (!id.empty() && !name.empty()) {
            TrendingApp app;
            app.id = id;
            app.name = name;
            app.summary = summary.empty() ? "Application disponible sur Flathub" : summary;
            app.icon_url = icon;
            app.color_class = palette[idx % palette.size()];

            // URL officielle Flathub si icon n'est pas une URL complète
            std::string download_url = icon;
            if (download_url.empty() || download_url.find("http") == std::string::npos) {
                download_url = "https://dl.flathub.org/repo/appstream/x86_64/icons/128x128/" + id + ".png";
            }

            std::string local_path = cache_dir + "/" + id + ".png";
            if (download_file(download_url, local_path)) {
                app.local_icon_path = local_path;
            }

            apps.push_back(app);
            idx++;
        }
    }

    return apps;
}

} // namespace bluehub::core