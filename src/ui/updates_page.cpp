#include "updates_page.hpp"
#include <cstdio>
#include <filesystem>
#include <algorithm>

namespace bluehub::ui {

UpdatesPage::UpdatesPage() {
    set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);

    m_content_box.set_margin_start(36);
    m_content_box.set_margin_end(36);
    m_content_box.set_margin_top(28);
    m_content_box.set_margin_bottom(36);

    // 0. Bannière de progression temps réel (masquée par défaut)
    m_progress_card.add_css_class("install-progress-card");
    m_progress_card.set_visible(false);

    m_progress_icon.add_css_class("installed-app-icon");
    m_progress_icon.set_valign(Gtk::Align::CENTER);
    m_progress_icon.set_pixel_size(48);
    m_progress_icon.set_from_icon_name("software-update-available-symbolic");

    m_progress_info_box.set_hexpand(true);
    m_progress_info_box.set_valign(Gtk::Align::CENTER);

    m_progress_title_label.add_css_class("install-progress-title");
    m_progress_title_label.set_halign(Gtk::Align::START);
    m_progress_title_label.set_hexpand(true);
    m_progress_title_label.set_ellipsize(Pango::EllipsizeMode::END);

    m_progress_pct_label.add_css_class("install-progress-pct");
    m_progress_pct_label.set_halign(Gtk::Align::END);
    m_progress_pct_label.set_width_chars(6);
    m_progress_pct_label.set_xalign(1.0f);

    m_progress_header_row.append(m_progress_title_label);
    m_progress_header_row.append(m_progress_pct_label);

    m_progress_status_label.add_css_class("install-progress-status");
    m_progress_status_label.set_halign(Gtk::Align::START);

    m_progress_bar.add_css_class("install-progressbar");
    m_progress_bar.set_hexpand(true);

    m_progress_info_box.append(m_progress_header_row);
    m_progress_info_box.append(m_progress_status_label);
    m_progress_info_box.append(m_progress_bar);

    m_progress_card.append(m_progress_icon);
    m_progress_card.append(m_progress_info_box);
    m_content_box.append(m_progress_card);

    // 1. En-tête principal
    m_status_label.add_css_class("updates-title");
    m_status_label.set_halign(Gtk::Align::START);
    m_status_label.set_hexpand(true);

    m_btn_refresh.set_icon_name("view-refresh-symbolic");
    m_btn_refresh.add_css_class("btn-refresh-updates");
    m_btn_refresh.set_cursor("pointer");
    m_btn_refresh.set_valign(Gtk::Align::CENTER);
    m_btn_refresh.set_tooltip_text("Rechercher les mises à jour");
    m_btn_refresh.signal_clicked().connect([this]() { reload_updates(); });

    m_btn_update_all.add_css_class("btn-update");
    m_btn_update_all.set_cursor("pointer");
    m_btn_update_all.set_valign(Gtk::Align::CENTER);
    m_btn_update_all.signal_clicked().connect([this]() {
        std::vector<core::UpdateItem> all_items;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            all_items = m_updates;
        }
        start_update(all_items, "Mise à jour globale du système");
    });

    m_header_box.append(m_status_label);
    m_header_box.append(m_btn_refresh);
    m_header_box.append(m_btn_update_all);
    m_content_box.append(m_header_box);

    // 2. Section Applications
    m_apps_section_title.add_css_class("updates-section-title");
    m_apps_section_title.set_halign(Gtk::Align::START);
    m_btn_update_apps.add_css_class("btn-category-update");
    m_btn_update_apps.set_cursor("pointer");
    m_btn_update_apps.set_halign(Gtk::Align::START);
    m_btn_update_apps.signal_clicked().connect([this]() {
        std::vector<core::UpdateItem> category_items;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (const auto& it : m_updates) {
                if (it.category == "app") category_items.push_back(it);
            }
        }
        start_update(category_items, "Mise à jour des applications");
    });

    m_apps_section_box.append(m_apps_section_title);
    m_apps_section_box.append(m_apps_list_box);
    m_apps_section_box.append(m_btn_update_apps);
    m_apps_section_box.append(*create_separator());
    m_content_box.append(m_apps_section_box);

    // 3. Section Utilitaires
    m_utils_section_title.add_css_class("updates-section-title");
    m_utils_section_title.set_halign(Gtk::Align::START);
    m_btn_update_utils.add_css_class("btn-category-update");
    m_btn_update_utils.set_cursor("pointer");
    m_btn_update_utils.set_halign(Gtk::Align::START);
    m_btn_update_utils.signal_clicked().connect([this]() {
        std::vector<core::UpdateItem> category_items;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (const auto& it : m_updates) {
                if (it.category == "utility") category_items.push_back(it);
            }
        }
        start_update(category_items, "Mise à jour des utilitaires");
    });

    m_utils_section_box.append(m_utils_section_title);
    m_utils_section_box.append(m_utils_list_box);
    m_utils_section_box.append(m_btn_update_utils);
    m_utils_section_box.append(*create_separator());
    m_content_box.append(m_utils_section_box);

    // 4. Section Composants Système
    m_system_section_title.add_css_class("updates-section-title");
    m_system_section_title.set_halign(Gtk::Align::START);
    m_btn_update_system.add_css_class("btn-category-update");
    m_btn_update_system.set_cursor("pointer");
    m_btn_update_system.set_halign(Gtk::Align::START);
    m_btn_update_system.signal_clicked().connect([this]() {
        std::vector<core::UpdateItem> category_items;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (const auto& it : m_updates) {
                if (it.category == "system") category_items.push_back(it);
            }
        }
        start_update(category_items, "Mise à jour des composants système");
    });

    m_system_section_box.append(m_system_section_title);
    m_system_section_box.append(m_system_list_box);
    m_system_section_box.append(m_btn_update_system);
    m_content_box.append(m_system_section_box);

    set_child(m_content_box);

    m_dispatcher.connect([this]() { render_updates(); });
    m_progress_dispatcher.connect([this]() { on_update_progress_updated(); });

    reload_updates();
}

UpdatesPage::~UpdatesPage() {
    if (m_fetch_thread.joinable()) m_fetch_thread.detach();
    if (m_update_thread.joinable()) m_update_thread.detach();
}

void UpdatesPage::reload_updates() {
    m_status_label.set_label("Recherche des mises à jour...");
    m_btn_refresh.set_sensitive(false);

    if (m_fetch_thread.joinable()) m_fetch_thread.detach();

    m_fetch_thread = std::thread([this]() {
        auto list = core::AbpmService::get_available_updates(true);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_updates = std::move(list);
        }
        m_dispatcher.emit();
    });
}

void UpdatesPage::render_updates() {
    m_btn_refresh.set_sensitive(true);

    while (auto child = m_apps_list_box.get_first_child()) m_apps_list_box.remove(*child);
    while (auto child = m_utils_list_box.get_first_child()) m_utils_list_box.remove(*child);
    while (auto child = m_system_list_box.get_first_child()) m_system_list_box.remove(*child);

    std::vector<core::UpdateItem> apps;
    std::vector<core::UpdateItem> utils;
    std::vector<core::UpdateItem> sys;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& it : m_updates) {
            if (it.category == "app") apps.push_back(it);
            else if (it.category == "utility") utils.push_back(it);
            else sys.push_back(it);
        }
    }

    size_t total = apps.size() + utils.size() + sys.size();
    if (total == 0) {
        m_status_label.set_label("Votre système est à jour");
        m_btn_update_all.set_sensitive(false);
        m_apps_section_box.set_visible(false);
        m_utils_section_box.set_visible(false);
        m_system_section_box.set_visible(false);
        return;
    }

    m_btn_update_all.set_sensitive(true);
    m_status_label.set_label(std::to_string(total) + " mise(s) à jour disponible(s)");

    // Remplissage Section Applications
    if (!apps.empty()) {
        m_apps_section_title.set_label("Applications à mettre à jour (" + std::to_string(apps.size()) + ") :");
        m_btn_update_apps.set_label("Mettre à jour les applications (" + std::to_string(apps.size()) + ")");
        for (const auto& app : apps) {
            m_apps_list_box.append(*create_update_row(app));
        }
        m_apps_section_box.set_visible(true);
    } else {
        m_apps_section_box.set_visible(false);
    }

    // Remplissage Section Utilitaires
    if (!utils.empty()) {
        m_utils_section_title.set_label("Utilitaires à mettre à jour (" + std::to_string(utils.size()) + ") :");
        m_btn_update_utils.set_label("Mettre à jour les utilitaires (" + std::to_string(utils.size()) + ")");
        for (const auto& ut : utils) {
            m_utils_list_box.append(*create_update_row(ut));
        }
        m_utils_section_box.set_visible(true);
    } else {
        m_utils_section_box.set_visible(false);
    }

    // Remplissage Section Composants Système
    if (!sys.empty()) {
        m_system_section_title.set_label("Composants système à mettre à jour (" + std::to_string(sys.size()) + ") :");
        m_btn_update_system.set_label("Mettre à jour les composants système (" + std::to_string(sys.size()) + ")");
        for (const auto& s : sys) {
            m_system_list_box.append(*create_update_row(s));
        }
        m_system_section_box.set_visible(true);
    } else {
        m_system_section_box.set_visible(false);
    }
}

Gtk::Box* UpdatesPage::create_update_row(const core::UpdateItem& item) {
    auto row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 16);
    row->add_css_class("installed-row");

    // 1. Icône
    auto icon_img = Gtk::make_managed<Gtk::Image>();
    icon_img->set_pixel_size(44);
    icon_img->add_css_class("installed-app-icon");

    if (!item.local_icon_path.empty() && std::filesystem::exists(item.local_icon_path)) {
        icon_img->set(item.local_icon_path);
    } else {
        auto display = Gdk::Display::get_default();
        std::string clean_icon = item.icon;
        if (clean_icon.size() > 4 && clean_icon.rfind(".png") == clean_icon.size() - 4) {
            clean_icon = clean_icon.substr(0, clean_icon.size() - 4);
        }
        bool has_icon = false;
        if (display) {
            auto theme = Gtk::IconTheme::get_for_display(display);
            if (theme && (theme->has_icon(clean_icon) || theme->has_icon(item.id))) {
                icon_img->set_from_icon_name(theme->has_icon(clean_icon) ? clean_icon : item.id);
                has_icon = true;
            }
        }
        if (!has_icon) {
            if (item.category == "app") icon_img->set_from_icon_name("application-x-executable");
            else if (item.category == "utility") icon_img->set_from_icon_name("utilities-terminal-symbolic");
            else icon_img->set_from_icon_name("emblem-system-symbolic");
        }
    }
    row->append(*icon_img);

    // 2. Textes : Nom, Badge Source et Diff de Version
    auto text_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
    text_box->set_valign(Gtk::Align::CENTER);
    text_box->set_hexpand(true);

    auto title_label = Gtk::make_managed<Gtk::Label>(item.name);
    title_label->add_css_class("installed-app-title");
    title_label->set_halign(Gtk::Align::START);
    text_box->append(*title_label);

    auto sub_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    auto badge_source = Gtk::make_managed<Gtk::Label>(item.source == "flatpak" ? "FLATPAK" : "DNF");
    badge_source->add_css_class("badge-source");
    badge_source->add_css_class(item.source == "flatpak" ? "badge-flatpak" : "badge-dnf");
    sub_box->append(*badge_source);

    if (!item.current_version.empty() && !item.new_version.empty()) {
        auto ver_label = Gtk::make_managed<Gtk::Label>(item.current_version + "  ➜  " + item.new_version);
        ver_label->add_css_class("update-version-label");
        sub_box->append(*ver_label);
    }
    text_box->append(*sub_box);
    row->append(*text_box);

    // 3. Bouton individuel 'Mettre à jour'
    auto update_btn = Gtk::make_managed<Gtk::Button>("Mettre à jour");
    update_btn->add_css_class("btn-update");
    update_btn->set_cursor("pointer");
    update_btn->set_valign(Gtk::Align::CENTER);
    update_btn->signal_clicked().connect([this, item]() {
        start_update({item}, "Mise à jour de " + item.name);
    });
    row->append(*update_btn);

    return row;
}

void UpdatesPage::start_single_update(const core::UpdateItem& item) {
    start_update({item}, "Mise à jour de " + item.name);
}

void UpdatesPage::start_update(const std::vector<core::UpdateItem>& items, const std::string& title) {
    if (m_is_updating) return;
    if (items.empty()) return;
    m_is_updating = true;

    m_progress_title_label.set_text(title);
    m_progress_status_label.set_text("Initialisation de la mise à jour...");
    m_progress_pct_label.set_text("0 %");
    m_progress_bar.set_fraction(0.0);
    m_progress_card.set_visible(true);

    {
        std::lock_guard<std::mutex> lock(m_progress_mutex);
        m_current_progress.title = title;
        m_current_progress.percentage = 0;
        m_current_progress.status_message = "Initialisation...";
        m_current_progress.is_finished = false;
        m_current_progress.is_error = false;
        m_current_progress.error_message = "";
    }

    if (m_update_thread.joinable()) m_update_thread.detach();

    m_update_thread = std::thread([this, items, title]() {
        bool has_dnf = false;
        bool has_flatpak = false;
        std::vector<std::string> dnf_pkgs;
        std::vector<std::string> flatpak_pkgs;

        for (const auto& it : items) {
            if (it.source == "flatpak") {
                has_flatpak = true;
                flatpak_pkgs.push_back(it.package);
            } else {
                has_dnf = true;
                dnf_pkgs.push_back(it.package);
            }
        }

        std::vector<std::string> commands;
        if (has_dnf && has_flatpak) {
            commands.push_back("pkexec abpm update -y --progress 2>&1");
        } else if (has_dnf) {
            std::string cmd = "pkexec abpm update -y --progress --dnf ";
            for (const auto& p : dnf_pkgs) cmd += "\"" + p + "\" ";
            cmd += "2>&1";
            commands.push_back(cmd);
        } else if (has_flatpak) {
            std::string cmd = "abpm update -y --progress --flatpak ";
            for (const auto& p : flatpak_pkgs) cmd += "\"" + p + "\" ";
            cmd += "2>&1";
            commands.push_back(cmd);
        }

        bool success = true;
        for (const auto& cmd : commands) {
            g_print("[UpdatesPage] Commande : %s\n", cmd.c_str());
            FILE* pipe = popen(cmd.c_str(), "r");
            if (!pipe) {
                success = false;
                break;
            }

            char buffer[1024];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                std::string line(buffer);
                while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();

                if (line.rfind("PROGRESS:ERROR:", 0) == 0) {
                    std::string err = line.substr(15);
                    {
                        std::lock_guard<std::mutex> lock(m_progress_mutex);
                        m_current_progress.is_error = true;
                        m_current_progress.error_message = err;
                    }
                    m_progress_dispatcher.emit();
                    success = false;
                    break;
                } else if (line.rfind("PROGRESS:", 0) == 0) {
                    auto sep = line.find(':', 9);
                    if (sep != std::string::npos) {
                        try {
                            int pct = std::stoi(line.substr(9, sep - 9));
                            std::string msg = line.substr(sep + 1);
                            {
                                std::lock_guard<std::mutex> lock(m_progress_mutex);
                                m_current_progress.percentage = pct;
                                m_current_progress.status_message = msg;
                            }
                            m_progress_dispatcher.emit();
                        } catch (...) {}
                    }
                }
            }

            int exit_code = pclose(pipe);
            int term = WEXITSTATUS(exit_code);
            if (term != 0 && success) {
                if (term == 126 || term == 127) {
                    std::lock_guard<std::mutex> lock(m_progress_mutex);
                    m_current_progress.is_error = true;
                    m_current_progress.error_message = "Autorisation administrateur annulée.";
                } else {
                    std::lock_guard<std::mutex> lock(m_progress_mutex);
                    m_current_progress.is_error = true;
                    m_current_progress.error_message = "Échec de la mise à jour (code " + std::to_string(term) + ").";
                }
                success = false;
                m_progress_dispatcher.emit();
                break;
            }
        }

        if (success) {
            {
                std::lock_guard<std::mutex> lock(m_progress_mutex);
                m_current_progress.percentage = 100;
                m_current_progress.status_message = "Mise à jour terminée avec succès !";
                m_current_progress.is_finished = true;
            }
            m_progress_dispatcher.emit();
        }
        m_is_updating = false;
    });
}

void UpdatesPage::on_update_progress_updated() {
    UpdateProgress progress;
    {
        std::lock_guard<std::mutex> lock(m_progress_mutex);
        progress = m_current_progress;
    }

    if (progress.is_error) {
        m_progress_status_label.set_text("Erreur : " + progress.error_message);
        m_progress_pct_label.set_text("Échec");
        m_is_updating = false;
        Glib::signal_timeout().connect_once([this]() {
            if (!m_is_updating) m_progress_card.set_visible(false);
        }, 5000);
        return;
    }

    if (m_progress_status_label.get_text() != progress.status_message) {
        m_progress_status_label.set_text(progress.status_message);
    }
    std::string pct_text = std::to_string(progress.percentage) + " %";
    if (m_progress_pct_label.get_text() != pct_text) {
        m_progress_pct_label.set_text(pct_text);
    }
    m_progress_bar.set_fraction(std::clamp(progress.percentage / 100.0, 0.0, 1.0));

    if (progress.is_finished) {
        send_update_notification(progress.title + " terminée avec succès.");
        reload_updates();

        Glib::signal_timeout().connect_once([this]() {
            if (!m_is_updating) m_progress_card.set_visible(false);
        }, 2500);
    }
}

void UpdatesPage::send_update_notification(const std::string& text) {
    bool has_desktop = false;
    const char* home = std::getenv("HOME");
    if (std::filesystem::exists("/usr/share/applications/io.github.arrerasoftware.bluehub.desktop") ||
        (home && std::filesystem::exists(std::string(home) + "/.local/share/applications/io.github.arrerasoftware.bluehub.desktop"))) {
        has_desktop = true;
    }

    if (has_desktop) {
        try {
            auto app = Gio::Application::get_default();
            if (app) {
                auto notif = Gio::Notification::create("Mise à jour terminée");
                notif->set_body(text);
                notif->set_icon(Gio::ThemedIcon::create("software-update-available-symbolic"));
                app->send_notification("update-finished", notif);
                return;
            }
        } catch (...) {}
    }

    std::string fallback = "notify-send -i software-update-available-symbolic \"Mise à jour terminée\" \"" + text + "\" >/dev/null 2>&1 &";
    system(fallback.c_str());
}

Gtk::Separator* UpdatesPage::create_separator() {
    auto sep = Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::HORIZONTAL);
    sep->add_css_class("updates-separator");
    return sep;
}

} // namespace bluehub::ui
