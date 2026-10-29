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

class AbpmService {
public:
    static std::vector<DiscoverApp> get_discover_apps(int page = 1, int per_page = 6);
};

} // namespace bluehub::core