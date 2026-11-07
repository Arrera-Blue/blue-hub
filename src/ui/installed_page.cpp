#include "installed_page.hpp"
#include <filesystem>
#include <thread>
#include <algorithm>
#include <cstdio>

namespace bluehub::ui {

InstalledPage::InstalledPage() {
    set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);

    m_root_box.set_margin_start(36);
    m_root_box.set_margin_end(36);
    m_root_box.set_margin_top(24);
    m_root_box.set_margin_bottom(30);

    // 0. Bannière d'installation active en cours (en haut)
    m_install_card.add_css_class("install-progress-card");
    m_install_card.set_visible(false);

    m_install_icon.add_css_class("installed-app-icon");
    m_install_icon.set_valign(Gtk::Align::CENTER);
    m_install_icon.set_pixel_size(48);

    m_install_info_box.set_hexpand(true);
    m_install_info_box.set_valign(Gtk::Align::CENTER);

    m_install_title_label.add_css_class("install-progress-title");
    m_install_title_label.set_halign(Gtk::Align::START);
    m_install_title_label.set_hexpand(true);
    m_install_title_label.set_ellipsize(Pango::EllipsizeMode::END);

    m_install_pct_label.add_css_class("install-progress-pct");
    m_install_pct_label.set_halign(Gtk::Align::END);
    m_install_pct_label.set_width_chars(6);
    m_install_pct_label.set_xalign(1.0f);

    m_install_header_row.append(m_install_title_label);
    m_install_header_row.append(m_install_pct_label);

    m_install_status_label.add_css_class("install-progress-status");
    m_install_status_label.set_halign(Gtk::Align::START);

    m_install_progress_bar.add_css_class("install-progressbar");
    m_install_progress_bar.set_hexpand(true);

    m_install_info_box.append(m_install_header_row);
    m_install_info_box.append(m_install_status_label);
    m_install_info_box.append(m_install_progress_bar);

    m_install_card.append(m_install_icon);
    m_install_card.append(m_install_info_box);

    m_root_box.append(m_install_card);

    // 1. En-tête des applications utilisateur
    m_user_header_box.add_css_class("installed-header-row");
    m_user_count_label.add_css_class("installed-count-label");
    m_user_count_label.set_halign(Gtk::Align::START);
    m_user_count_label.set_hexpand(true);
    m_user_count_label.set_label("Chargement des applications...");
    m_user_header_box.append(m_user_count_label);

    m_root_box.append(m_user_header_box);
    m_root_box.append(m_user_list_box);

    // 2. En-tête de la catégorie Utilitaires système (en dessous)
    m_system_header_box.add_css_class("installed-section-separator");
    m_system_count_label.add_css_class("installed-count-label");
    m_system_count_label.set_halign(Gtk::Align::START);
    m_system_count_label.set_hexpand(true);
    m_system_count_label.set_label("Utilitaires système");
    m_system_header_box.append(m_system_count_label);

    m_root_box.append(m_system_header_box);
    m_root_box.append(m_system_list_box);

    set_child(m_root_box);

    m_dispatcher.connect([this]() { render_app_list(); });
    m_install_dispatcher.connect([this]() { on_install_progress_updated(); });
    reload_apps();
}

InstalledPage::~InstalledPage() {
    if (m_install_thread.joinable()) {
        m_install_thread.detach();
    }
}

void InstalledPage::reload_apps() {
    std::thread([this]() {
        auto apps = core::AbpmService::get_installed_apps(true);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_apps = std::move(apps);
        }
        m_dispatcher.emit();
    }).detach();
}

void InstalledPage::render_app_list() {
    // Vider les anciennes listes
    while (auto child = m_user_list_box.get_first_child()) {
        m_user_list_box.remove(*child);
    }
    while (auto child = m_system_list_box.get_first_child()) {
        m_system_list_box.remove(*child);
    }

    std::vector<core::InstalledApp> user_apps;
    std::vector<core::InstalledApp> system_apps;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& app : m_apps) {
            if (app.is_system) {
                system_apps.push_back(app);
            } else {
                user_apps.push_back(app);
            }
        }
    }

    // Tri alphabétique avec gestion propre des accents français (g_utf8_collate)
    auto alphabet_sort = [](const core::InstalledApp& a, const core::InstalledApp& b) {
        return g_utf8_collate(a.name.c_str(), b.name.c_str()) < 0;
    };
    std::sort(user_apps.begin(), user_apps.end(), alphabet_sort);
    std::sort(system_apps.begin(), system_apps.end(), alphabet_sort);

    m_user_count_label.set_label("Applications installées (" + std::to_string(user_apps.size()) + ")");
    for (const auto& app : user_apps) {
        m_user_list_box.append(*create_installed_row(app));
    }

    m_system_count_label.set_label("Utilitaires système (" + std::to_string(system_apps.size()) + ")");
    for (const auto& app : system_apps) {
        m_system_list_box.append(*create_installed_row(app));
    }
}

Gtk::Box* InstalledPage::create_installed_row(const core::InstalledApp& app) {
    auto row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 16);
    row->add_css_class("installed-row");

    // 1. Icône de l'application
    auto icon_img = Gtk::make_managed<Gtk::Image>();
    icon_img->set_pixel_size(44);
    icon_img->add_css_class("installed-app-icon");

    std::string icon_path = app.local_icon_path;
    if (icon_path.empty() || !std::filesystem::exists(icon_path)) {
        icon_path = core::AbpmService::find_local_app_icon(app.icon, app.id, app.package);
    }

    if (!icon_path.empty() && std::filesystem::exists(icon_path)) {
        icon_img->set(icon_path);
    } else {
        auto display = Gdk::Display::get_default();
        std::string clean_icon = app.icon;
        if (clean_icon.size() > 4 && clean_icon.rfind(".png") == clean_icon.size() - 4) {
            clean_icon = clean_icon.substr(0, clean_icon.size() - 4);
        }
        bool has_icon = false;
        if (display) {
            auto theme = Gtk::IconTheme::get_for_display(display);
            if (theme) {
                if (theme->has_icon(clean_icon)) {
                    icon_img->set_from_icon_name(clean_icon);
                    has_icon = true;
                } else if (theme->has_icon(app.id)) {
                    icon_img->set_from_icon_name(app.id);
                    has_icon = true;
                }
            }
        }
        if (!has_icon) {
            icon_img->set_from_icon_name("application-x-executable");
        }
    }

    // 2. Nom, badge éventuel et version
    auto text_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
    text_box->set_valign(Gtk::Align::CENTER);
    text_box->set_hexpand(true);

    auto title_label = Gtk::make_managed<Gtk::Label>(app.name);
    title_label->add_css_class("installed-app-title");
    title_label->set_halign(Gtk::Align::START);
    text_box->append(*title_label);

    auto sub_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);

    if (app.source == "flatpak") {
        auto badge = Gtk::make_managed<Gtk::Label>("Flatpak");
        badge->add_css_class("badge-source");
        badge->add_css_class("badge-flatpak");
        sub_box->append(*badge);
    } else if (app.source == "archive") {
        auto badge = Gtk::make_managed<Gtk::Label>("Archive");
        badge->add_css_class("badge-source");
        badge->add_css_class("badge-archive");
        sub_box->append(*badge);
    } else if (app.source == "appimage") {
        auto badge = Gtk::make_managed<Gtk::Label>("AppImage");
        badge->add_css_class("badge-source");
        badge->add_css_class("badge-appimage");
        sub_box->append(*badge);
    }

    if (!app.version.empty()) {
        auto ver_label = Gtk::make_managed<Gtk::Label>("v" + app.version);
        ver_label->add_css_class("installed-app-subtitle");
        sub_box->append(*ver_label);
    }
    text_box->append(*sub_box);

    // Conteneur cliquable : ouvrir la modale au clic sur l'application
    auto click_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 16);
    click_box->set_hexpand(true);
    click_box->set_cursor("pointer");
    click_box->append(*icon_img);
    click_box->append(*text_box);

    auto gesture = Gtk::GestureClick::create();
    gesture->signal_pressed().connect([this, app](int, double, double) {
        open_app_details(app);
    });
    click_box->add_controller(gesture);
    row->append(*click_box);

    // 3. Actions contextuelles rapides
    auto actions_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    actions_box->set_valign(Gtk::Align::CENTER);

    // Action A : Permissions Flatpak
    if (app.source == "flatpak") {
        auto perm_btn = Gtk::make_managed<Gtk::Button>();
        perm_btn->set_icon_name("preferences-system-symbolic");
        perm_btn->add_css_class("row-action-btn");
        perm_btn->add_css_class("perm-btn");
        perm_btn->set_cursor("pointer");
        perm_btn->set_tooltip_text("Modifier les autorisations Flatpak");
        perm_btn->signal_clicked().connect([this, app]() { on_modify_permissions(app); });
        actions_box->append(*perm_btn);
    }

    // Action B : Modifier l'icône (AppImage ou Archive)
    if (app.source == "appimage" || app.source == "archive") {
        auto icon_btn = Gtk::make_managed<Gtk::Button>();
        icon_btn->set_icon_name("insert-image-symbolic");
        icon_btn->add_css_class("row-action-btn");
        icon_btn->add_css_class("icon-btn");
        icon_btn->set_cursor("pointer");
        icon_btn->set_tooltip_text("Modifier l'icône de l'application");
        icon_btn->signal_clicked().connect([this, app]() { on_change_icon(app); });
        actions_box->append(*icon_btn);
    }

    // Action C : Désinstallation directe (non-système)
    if (!app.is_system) {
        auto delete_btn = Gtk::make_managed<Gtk::Button>();
        delete_btn->set_icon_name("user-trash-symbolic");
        delete_btn->add_css_class("row-action-btn");
        delete_btn->add_css_class("delete-btn");
        delete_btn->set_cursor("pointer");
        delete_btn->set_tooltip_text("Désinstaller " + app.name);
        delete_btn->signal_clicked().connect([this, app]() { on_uninstall_requested(app); });
        actions_box->append(*delete_btn);
    }

    row->append(*actions_box);
    return row;
}

void InstalledPage::open_app_details(const core::InstalledApp& app) {
    if (!m_details_modal) {
        auto root = dynamic_cast<Gtk::Window*>(get_root());
        if (root) {
            m_details_modal = std::make_unique<AppDetailsModal>(*root);
            m_details_modal->signal_app_uninstalled().connect([this]() {
                reload_apps();
            });
            m_details_modal->signal_install_requested().connect([this](const core::DiscoverApp& d_app) {
                start_installation(d_app);
            });
            m_details_modal->signal_update_requested().connect([this](const core::UpdateItem& item) {
                m_signal_update_app.emit(item);
            });
        }
    }
    if (m_details_modal) {
        m_details_modal->show_app(app);
    }
}

void InstalledPage::on_modify_permissions(const core::InstalledApp& app) {
    auto root = dynamic_cast<Gtk::Window*>(get_root());
    if (root) {
        if (!m_permissions_modal) {
            m_permissions_modal = std::make_unique<FlatpakPermissionsModal>(*root);
        }
        m_permissions_modal->show_permissions(app);
    }
}

void InstalledPage::on_change_icon(const core::InstalledApp& app) {
    auto file_dialog = Gtk::FileDialog::create();
    file_dialog->set_title("Sélectionner une nouvelle icône pour " + app.name);

    auto filter = Gtk::FileFilter::create();
    filter->set_name("Images (*.png, *.svg, *.jpg)");
    filter->add_mime_type("image/png");
    filter->add_mime_type("image/svg+xml");
    filter->add_mime_type("image/jpeg");

    auto filters = Gio::ListStore<Gtk::FileFilter>::create();
    filters->append(filter);
    file_dialog->set_filters(filters);

    file_dialog->open([this, app, file_dialog](const Glib::RefPtr<Gio::AsyncResult>& res) {
        try {
            auto file = file_dialog->open_finish(res);
            if (file) {
                std::string path = file->get_path();
                if (!path.empty()) {
                    core::AbpmService::set_app_icon(app.package, path);
                    reload_apps();
                }
            }
        } catch (...) {}
    });
}

void InstalledPage::on_uninstall_requested(const core::InstalledApp& app) {
    auto dialog = Gtk::AlertDialog::create("Désinstaller " + app.name + " ?");
    dialog->set_detail("Cette action supprimera définitivement l'application via abpm.");
    dialog->set_buttons({"Annuler", "Désinstaller"});
    dialog->set_cancel_button(0);
    dialog->set_default_button(1);

    dialog->choose([this, app, dialog](const Glib::RefPtr<Gio::AsyncResult>& res) {
        try {
            int btn = dialog->choose_finish(res);
            if (btn == 1) {
                std::thread([this, app]() {
                    core::AbpmService::uninstall_app(app.package, app.source);
                    reload_apps();
                }).detach();
            }
        } catch (...) {}
    });
}

void InstalledPage::start_installation(const core::DiscoverApp& app) {
    if (m_is_installing) {
        g_warning("[InstalledPage] Une installation est déjà en cours !");
        return;
    }
    m_is_installing = true;

    // 1. Mise à jour de la bannière visuelle
    m_install_title_label.set_text("Installation de " + app.name);
    m_install_status_label.set_text("Démarrage de l'installation...");
    m_install_pct_label.set_text("0 %");
    m_install_progress_bar.set_fraction(0.0);

    std::string icon_path = app.local_icon_path;
    if (icon_path.empty() || !std::filesystem::exists(icon_path)) {
        icon_path = core::AbpmService::find_local_app_icon(app.icon, app.id, app.package);
    }

    if (!icon_path.empty() && std::filesystem::exists(icon_path)) {
        m_install_icon.set(icon_path);
    } else {
        auto display = Gdk::Display::get_default();
        std::string clean_icon = app.icon;
        if (clean_icon.size() > 4 && clean_icon.rfind(".png") == clean_icon.size() - 4) {
            clean_icon = clean_icon.substr(0, clean_icon.size() - 4);
        }
        bool has_icon = false;
        if (display) {
            auto theme = Gtk::IconTheme::get_for_display(display);
            if (theme) {
                if (theme->has_icon(clean_icon)) {
                    m_install_icon.set_from_icon_name(clean_icon);
                    has_icon = true;
                } else if (theme->has_icon(app.id)) {
                    m_install_icon.set_from_icon_name(app.id);
                    has_icon = true;
                }
            }
        }
        if (!has_icon) {
            m_install_icon.set_from_icon_name("application-x-executable");
        }
    }
    m_install_icon.set_pixel_size(48);
    m_install_card.set_visible(true);

    {
        std::lock_guard<std::mutex> lock(m_install_mutex);
        m_current_install.app_name = app.name;
        m_current_install.percentage = 0;
        m_current_install.status_message = "Démarrage de l'installation...";
        m_current_install.is_finished = false;
        m_current_install.is_error = false;
        m_current_install.error_message = "";
    }

    if (m_install_thread.joinable()) {
        m_install_thread.detach();
    }

    m_install_thread = std::thread([this, app]() {
        std::string target = !app.package.empty() ? app.package : app.id;
        std::string cmd;

        // Choix de la commande :
        // Flatpak utilise libflatpak (avec SystemHelper de Flatpak) et s'exécute sans pkexec.
        // DNF/RPM modifie les fichiers système (/usr) et requiert les droits administrateur (pkexec).
        // AppImage et Archive s'installent dans l'espace utilisateur sans pkexec.
        if (app.source == "flatpak") {
            cmd = "abpm install -y --progress --flatpak \"" + target + "\" 2>&1";
        } else if (app.source == "dnf" || app.source == "rpm") {
            cmd = "pkexec abpm install -y --progress --dnf \"" + target + "\" 2>&1";
        } else if (app.source == "appimage" || app.source == "archive") {
            cmd = "abpm install -y --progress \"" + target + "\" 2>&1";
        } else {
            cmd = "abpm install -y --progress \"" + target + "\" 2>&1";
        }

        g_print("[InstalledPage] Commande d'installation : %s\n", cmd.c_str());

        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            {
                std::lock_guard<std::mutex> lock(m_install_mutex);
                m_current_install.is_error = true;
                m_current_install.error_message = "Impossible de lancer le processus d'installation.";
            }
            m_install_dispatcher.emit();
            m_is_installing = false;
            return;
        }

        char buffer[1024];
        bool has_finished = false;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string line(buffer);
            while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
                line.pop_back();
            }

            if (line.rfind("PROGRESS:ERROR:", 0) == 0) {
                std::string err = line.substr(15);
                {
                    std::lock_guard<std::mutex> lock(m_install_mutex);
                    m_current_install.is_error = true;
                    m_current_install.error_message = err;
                }
                m_install_dispatcher.emit();
                break;
            } else if (line.rfind("PROGRESS:", 0) == 0) {
                auto first_colon = line.find(':', 9);
                if (first_colon != std::string::npos) {
                    std::string pct_str = line.substr(9, first_colon - 9);
                    std::string msg = line.substr(first_colon + 1);
                    try {
                        int pct = std::stoi(pct_str);
                        {
                            std::lock_guard<std::mutex> lock(m_install_mutex);
                            m_current_install.percentage = pct;
                            m_current_install.status_message = msg;
                            if (pct >= 100) {
                                m_current_install.is_finished = true;
                                has_finished = true;
                            }
                        }
                        m_install_dispatcher.emit();
                    } catch (...) {}
                }
            } else {
                g_print("[abpm stdout] %s\n", line.c_str());
            }
        }

        int exit_code = pclose(pipe);
        int term_code = WEXITSTATUS(exit_code);
        g_print("[InstalledPage] Fin de commande, code: %d\n", term_code);

        {
            std::lock_guard<std::mutex> lock(m_install_mutex);
            if (!m_current_install.is_error) {
                if (term_code == 0 || has_finished) {
                    m_current_install.percentage = 100;
                    m_current_install.status_message = "Installation terminée avec succès !";
                    m_current_install.is_finished = true;
                } else if (term_code == 126 || term_code == 127) {
                    m_current_install.is_error = true;
                    m_current_install.error_message = "Autorisation administrateur refusée ou annulée.";
                } else {
                    m_current_install.is_error = true;
                    m_current_install.error_message = "L'installation a échoué (code " + std::to_string(term_code) + ").";
                }
            }
        }
        m_install_dispatcher.emit();
        m_is_installing = false;
    });
}

void InstalledPage::on_install_progress_updated() {
    InstallProgress progress;
    {
        std::lock_guard<std::mutex> lock(m_install_mutex);
        progress = m_current_install;
    }

    if (progress.is_error) {
        m_install_status_label.set_text("Erreur : " + progress.error_message);
        m_install_pct_label.set_text("Échec");
        m_is_installing = false;
        Glib::signal_timeout().connect_once([this]() {
            if (!m_is_installing) {
                m_install_card.set_visible(false);
            }
        }, 5000);
        return;
    }

    if (m_install_status_label.get_text() != progress.status_message) {
        m_install_status_label.set_text(progress.status_message);
    }
    std::string pct_text = std::to_string(progress.percentage) + " %";
    if (m_install_pct_label.get_text() != pct_text) {
        m_install_pct_label.set_text(pct_text);
    }
    m_install_progress_bar.set_fraction(std::clamp(progress.percentage / 100.0, 0.0, 1.0));

    if (progress.is_finished) {
        send_install_notification(progress.app_name);
        core::AbpmService::invalidate_installed_cache();
        reload_apps();

        Glib::signal_timeout().connect_once([this]() {
            if (!m_is_installing) {
                m_install_card.set_visible(false);
            }
        }, 2500);
    }
}

void InstalledPage::send_install_notification(const std::string& app_name) {
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
                auto notif = Gio::Notification::create("Installation terminée");
                notif->set_body(app_name + " est prête à être utilisée.");
                notif->set_icon(Gio::ThemedIcon::create("software-installed-symbolic"));
                app->send_notification("install-" + app_name, notif);
                return;
            }
        } catch (...) {}
    }

    std::string fallback = "notify-send -i software-installed-symbolic \"Installation terminée\" \"" + app_name + " est prête à être utilisée.\" >/dev/null 2>&1 &";
    system(fallback.c_str());
}

} // namespace bluehub::ui