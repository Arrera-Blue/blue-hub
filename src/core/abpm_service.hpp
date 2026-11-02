#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <unordered_set>

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
    bool is_verified{false};
    std::vector<std::string> screenshot_urls;
    std::vector<std::string> local_screenshot_paths;
};

struct InstalledApp {
    std::string id;
    std::string name;
    std::string version;
    std::string source;      
    std::string icon;
    std::string local_icon_path;
    std::string package;
    bool is_system{false};   
};

class AbpmService {
public:
    static std::vector<DiscoverApp> get_discover_apps(int page = 1, int per_page = 6);
    static AppDetails get_app_details(const DiscoverApp& app);

    // Gestion des applications installées
    static std::vector<InstalledApp> get_installed_apps(bool force_refresh = false);
    static bool is_app_installed(const std::string& id, const std::string& package, bool& out_is_system, std::string& out_source, std::string& out_package);
    static bool set_app_icon(const std::string& package, const std::string& icon_path);
    static bool uninstall_app(const std::string& package, const std::string& source);
    static void invalidate_installed_cache();
};

} // namespace bluehub::core