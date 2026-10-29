#pragma once

#include <string>
#include <vector>

namespace bluehub::core {

struct TrendingApp {
    std::string id;
    std::string name;
    std::string summary;
    std::string icon_url;
    std::string local_icon_path; // Chemin de l'image téléchargée en local
    std::string color_class;
};

class FlathubService {
public:
    static std::vector<TrendingApp> get_trending_apps(int count = 5);
};

} // namespace bluehub::core