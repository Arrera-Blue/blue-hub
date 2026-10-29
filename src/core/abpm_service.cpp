#include "abpm_service.hpp"
#include <glibmm.h>
#include <regex>
#include <sstream>
#include <filesystem>
#include <curl/curl.h>
#include <iostream>
#include <unordered_map>

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

    size_t write_cb(void* ptr, size_t size, size_t nmemb, void* data) {
        std::string* s = static_cast<std::string*>(data);
        s->append(static_cast<char*>(ptr), size * nmemb);
        return size * nmemb;
    }

    bool check_flathub_verified(const std::string& app_id) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        std::string url = "https://flathub.org/api/v2/verification/" + app_id + "/status";
        std::string response;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "BlueHub/1.0");

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) return false;
        return (response.find("\"verified\":true") != std::string::npos ||
                response.find("\"verified\": true") != std::string::npos);
    }

    bool is_likely_english(const std::string& text) {
        std::string lower = text;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        static const std::vector<std::string> en_markers = {
            " is a ", " is an ", " allows you to ", " with a ", " featuring ", " brings ", " software for "
        };
        for (const auto& marker : en_markers) {
            if (lower.find(marker) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    bool download_file(const std::string& url, const std::string& dest_path) {
        if (std::filesystem::exists(dest_path) && std::filesystem::file_size(dest_path) > 1024) {
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
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "BlueHub/1.0");

        CURLcode res = curl_easy_perform(curl);
        fclose(fp);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK || !std::filesystem::exists(dest_path) || std::filesystem::file_size(dest_path) == 0) {
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

            if (icon.rfind("http", 0) == 0) {
                std::string local_path = cache_dir + "/" + id + ".png";
                if (download_file(icon, local_path)) {
                    app.local_icon_path = local_path;
                }
            }

            apps.push_back(app);
        }
    }

    return apps;
}

AppDetails AbpmService::get_app_details(const DiscoverApp& app) {
    AppDetails details;
    details.id = app.id;
    details.name = app.name;
    details.summary = app.summary;
    details.description = app.summary;
    details.developer = "Éditeur officiel";

    // 1. Certification officielle Flathub
    details.is_verified = check_flathub_verified(app.id);

    // Déduction par défaut du développeur
    auto first_dot = app.id.find('.');
    if (first_dot != std::string::npos) {
        auto second_dot = app.id.find('.', first_dot + 1);
        if (second_dot != std::string::npos) {
            std::string dev = app.id.substr(first_dot + 1, second_dot - first_dot - 1);
            if (!dev.empty()) {
                dev[0] = static_cast<char>(std::toupper(dev[0]));
                details.developer = dev;
            }
        }
    }

    std::string stdout_data;
    std::string stderr_data;
    int exit_status = 0;

    // 2. Interroger appstreamcli localement
    std::string cmd = "appstreamcli dump " + app.id;
    try {
        Glib::spawn_command_line_sync(cmd, &stdout_data, &stderr_data, &exit_status);
    } catch (...) {}

    if ((stdout_data.empty() || exit_status != 0) && app.package != app.id && !app.package.empty()) {
        cmd = "appstreamcli dump " + app.package;
        try {
            Glib::spawn_command_line_sync(cmd, &stdout_data, &stderr_data, &exit_status);
        } catch (...) {}
    }

    if (!stdout_data.empty()) {
        std::regex desc_regex(R"raw(<description>([\s\S]*?)</description>)raw");
        std::smatch match;
        if (std::regex_search(stdout_data, match, desc_regex)) {
            std::string raw_desc = match[1].str();
            raw_desc = std::regex_replace(raw_desc, std::regex(R"raw(<li[^>]*>)raw"), "• ");
            raw_desc = std::regex_replace(raw_desc, std::regex(R"raw(</li>)raw"), "\n");
            raw_desc = std::regex_replace(raw_desc, std::regex(R"raw(</p>)raw"), "\n\n");
            raw_desc = std::regex_replace(raw_desc, std::regex(R"raw(<[^>]+>)raw"), "");

            std::istringstream stream(raw_desc);
            std::string line;
            std::string clean_desc;
            bool last_empty = false;
            while (std::getline(stream, line)) {
                size_t start = line.find_first_not_of(" \t\r\n");
                if (start == std::string::npos) {
                    if (!last_empty && !clean_desc.empty()) {
                        clean_desc += "\n\n";
                        last_empty = true;
                    }
                    continue;
                }
                size_t end = line.find_last_not_of(" \t\r\n");
                std::string trimmed = line.substr(start, end - start + 1);
                clean_desc += trimmed + "\n";
                last_empty = false;
            }
            if (!clean_desc.empty()) {
                details.description = clean_desc;
            }
        }

        std::regex dev_regex(R"raw(<developer[^>]*>[\s\S]*?<name>([^<]+)</name>)raw");
        if (std::regex_search(stdout_data, match, dev_regex)) {
            details.developer = match[1].str();
        } else {
            std::regex proj_regex(R"raw(<project_group>([^<]+)</project_group>)raw");
            if (std::regex_search(stdout_data, match, proj_regex)) {
                details.developer = match[1].str();
            }
        }

        std::string lower_dev = details.developer;
        std::transform(lower_dev.begin(), lower_dev.end(), lower_dev.begin(), ::tolower);
        if (lower_dev == "github" || lower_dev == "gitlab" || lower_dev == "codeberg") {
            details.developer = "";
        }

        std::regex ss_block_regex(R"raw(<screenshots>([\s\S]*?)</screenshots>)raw");
        if (std::regex_search(stdout_data, match, ss_block_regex)) {
            std::string ss_block = match[1].str();
            std::regex img_regex(R"raw(<image[^>]*>([^<]+)</image>)raw");
            auto it = std::sregex_iterator(ss_block.begin(), ss_block.end(), img_regex);
            auto end = std::sregex_iterator();
            for (; it != end && details.screenshot_urls.size() < 6; ++it) {
                std::string url = (*it)[1].str();
                url.erase(0, url.find_first_not_of(" \t\r\n"));
                url.erase(url.find_last_not_of(" \t\r\n") + 1);
                if (!url.empty()) {
                    details.screenshot_urls.push_back(url);
                }
            }
        }
    }

    // 3. Traduction et localisation française des descriptions
    const char* const* langs = g_get_language_names();
    bool is_french = (langs && langs[0] && std::string(langs[0]).rfind("fr", 0) == 0);

    if (is_french) {
        static const std::unordered_map<std::string, std::string> fr_descriptions = {
            {"org.mozilla.firefox",
             "Firefox est un navigateur web libre, moderne et performant conçu par Mozilla pour préserver votre vie privée.\n\n"
             "Il intègre une protection renforcée contre le pistage en ligne, une synchronisation sécurisée de vos onglets et mots de passe, ainsi qu'un immense catalogue d'extensions pour adapter votre navigation à vos besoins."},

            {"org.gimp.GIMP",
             "GIMP (GNU Image Manipulation Program) est la suite libre de référence pour la création graphique et la retouche photo avancée.\n\n"
             "Idéal pour les photographes et les designers, il fournit des outils sophistiqués de peinture, de composition par calques, de détourage et de traitement d'images haute résolution."},

            {"org.videolan.vlc",
             "VLC est un lecteur multimédia libre et universel capable de lire tous les formats de vidéos et de musiques sans installer de codecs supplémentaires.\n\n"
             "Il prend en charge les flux réseau en direct, les sous-titres, la conversion de médias et offre une lecture fluide et optimisée sur tous vos écrans."},

            {"org.videolan.VLC",
             "VLC est un lecteur multimédia libre et universel capable de lire tous les formats de vidéos et de musiques sans installer de codecs supplémentaires.\n\n"
             "Il prend en charge les flux réseau en direct, les sous-titres, la conversion de médias et offre une lecture fluide et optimisée sur tous vos écrans."},

            {"org.blender.Blender",
             "Blender est la suite libre tout-en-un de création 3D professionnelle.\n\n"
             "Elle couvre l'intégralité du pipeline créatif : modélisation polygonale, sculpture, texturation, animation de personnages, simulation physique, rendu photoréaliste et montage vidéo."},

            {"org.godotengine.Godot",
             "Godot Engine est un moteur de jeu 2D et 3D libre, moderne et léger conçu pour donner vie à vos projets de jeux vidéo.\n\n"
             "Son architecture par scènes et nœuds, combinée à son langage GDScript intuitif et son support de C#, offre un confort de développement exceptionnel."},

            {"com.visualstudio.code",
             "Visual Studio Code est un éditeur de code source moderne et puissant développé par Microsoft.\n\n"
             "Il inclut la complétion intelligente IntelliSense, le débogage interactif, le contrôle de version Git intégré et des milliers d'extensions pour tous vos langages préférés."},

            {"net.armatik.Kitsune",
             "Kitsune est une application élégante et fluide pour rechercher, regarder et suivre vos animes favoris en streaming via le catalogue AniLiberty."},

            {"com.obsproject.Studio",
             "OBS Studio est le logiciel libre de référence pour la capture vidéo haute performance et la diffusion en direct sur toutes les plateformes de streaming."},

            {"org.libreoffice.LibreOffice",
             "LibreOffice est la suite bureautique libre la plus complète du marché, comprenant traitement de texte, tableur, logiciel de présentation et dessin vectoriel."},

            {"org.inkscape.Inkscape",
             "Inkscape est un éditeur de graphismes vectoriels professionnel et libre conforme aux standards SVG, idéal pour l'illustration, les logos et la typographie."},

            {"org.kde.kdenlive",
             "Kdenlive est un éditeur de montage vidéo multipiste libre et puissant, doté de nombreux effets et transitions pour réaliser des montages vidéo de haute qualité."},

            {"io.github.shifteight.mousam",
             "Mousam est une application météo élégante qui affiche les conditions climatiques locales en temps réel, avec prévisions horaires et alertes météo."}
        };

        auto it = fr_descriptions.find(app.id);
        if (it != fr_descriptions.end()) {
            details.description = it->second;
        } else if (is_likely_english(details.description) && !app.summary.empty()) {
            details.description = app.summary + ".\n\n" +
                "Profitez d'une expérience complète et intégrée avec cette application conçue pour s'adapter à votre flux de travail sur Arrera Blue OS.";
        }
    }

    // 4. Téléchargement et mise en cache des captures d'écran
    std::string ss_cache_dir = "/tmp/blue-hub/cache/screenshots";
    try {
        std::filesystem::create_directories(ss_cache_dir);
    } catch (...) {}

    for (size_t i = 0; i < details.screenshot_urls.size() && i < 5; ++i) {
        std::string dest = ss_cache_dir + "/" + details.id + "_" + std::to_string(i) + ".png";
        if (download_file(details.screenshot_urls[i], dest)) {
            details.local_screenshot_paths.push_back(dest);
        }
    }

    return details;
}

} // namespace bluehub::core
