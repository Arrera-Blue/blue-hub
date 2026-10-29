#pragma once

#include <string>
#include <vector>

namespace bluehub::core {

struct DiscoverApp {
    std::string id;
    std::string name;
    std::string summary;
    std::string icon;
    std::string source;  // "dnf" ou "flatpak"
    std::string package;
    std::string local_icon_path;
};

struct AppDetails {
    std::string id;
    std::string name;
    std::string summary;
    std::string description;
    std::string developer;
    bool is_verified{false}; // <--- Certification Flathub
    std::vector<std::string> screenshot_urls;
    std::vector<std::string> local_screenshot_paths;
};

class AbpmService {
public:
    static std::vector<DiscoverApp> get_discover_apps(int page = 1, int per_page = 6);
    static AppDetails get_app_details(const DiscoverApp& app);
};

} // namespace bluehub::core