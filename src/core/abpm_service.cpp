#include "abpm_service.hpp"
#include <glibmm.h>
#include <regex>
#include <filesystem>
#include <curl/curl.h>
#include <iostream>

namespace bluehub::core {

namespace {
    std::string extract_json_field(const std::string& obj, const std::string& key) {
        std::string pattern = "\"" + key + "\"\\s*:\\s*\"([^\"]*)\"";
        std::regex reg(pattern);
        std::smatch m;
        if (std::regex_search(obj, m, reg) && m.size() > 1) {
            return m[1].str();
        }
        return "";
    }

    bool download_icon(const std::string& url, const std::string& dest_path) {
        if (std::filesystem::exists(dest_path) && std::filesystem::file_size(dest_path) > 0) {
            return true;
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
}

std::vector<DiscoverApp> AbpmService::get_discover_apps(int page, int per_page) {
    std::vector<DiscoverApp> apps;

    std::string stdout_data;
    std::string stderr_data;
    int exit_status = 0;

    std::string cmd = "abpm discover --page " + std::to_string(page) +
                      " --per-page " + std::to_string(per_page) + " --json";

    try {
        Glib::spawn_command_line_sync(cmd, &stdout_data, &stderr_data, &exit_status);
    } catch (const Glib::Error& ex) {
        std::cerr << "Erreur lors de l'exécution de abpm discover: " << ex.what() << std::endl;
        return apps;
    }

    if (stdout_data.empty() || exit_status != 0) {
        return apps;
    }

    std::string cache_dir = "/tmp/blue-hub/cache/icons";
    try {
        std::filesystem::create_directories(cache_dir);
    } catch (...) {}

    std::regex block_regex(R"raw(\{[^{}]+\})raw");
    auto it = std::sregex_iterator(stdout_data.begin(), stdout_data.end(), block_regex);
    auto end = std::sregex_iterator();

    for (; it != end; ++it) {
        std::string block = it->str();
        std::string id = extract_json_field(block, "id");
        std::string name = extract_json_field(block, "name");
        std::string summary = extract_json_field(block, "summary");
        std::string icon = extract_json_field(block, "icon");
        std::string source = extract_json_field(block, "source");
        std::string pkg = extract_json_field(block, "package");

        if (!id.empty() && !name.empty()) {
            DiscoverApp app;
            app.id = id;
            app.name = name;
            app.summary = summary;
            app.icon = icon;
            app.source = source.empty() ? "dnf" : source;
            app.package = pkg.empty() ? id : pkg;

            // Si l'icône est une URL HTTP(S) (Flatpak), on la télécharge en cache
            if (icon.rfind("http", 0) == 0) {
                std::string local_path = cache_dir + "/" + id + ".png";
                if (download_icon(icon, local_path)) {
                    app.local_icon_path = local_path;
                }
            }

            apps.push_back(app);
        }
    }

    return apps;
}

} // namespace bluehub::core