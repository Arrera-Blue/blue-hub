#include "flatpak_permissions_modal.hpp"
#include <sstream>
#include <unordered_set>
#include <cstdlib>

namespace bluehub::ui {

FlatpakPermissionsModal::FlatpakPermissionsModal(Gtk::Window& parent) {
    set_transient_for(parent);
    set_modal(true);
    set_default_size(780, 640);
    set_resizable(false);
    set_decorated(false); // Modale sans bordure système

    add_css_class("app-details-window");
    m_root_box.add_css_class("app-details-modal");

    // 1. Bouton fermer [✕] placé EN HAUT À GAUCHE
    m_top_bar.set_margin_start(20);
    m_top_bar.set_margin_end(20);
    m_top_bar.set_margin_top(16);
    m_btn_close.set_icon_name("window-close-symbolic");
    m_btn_close.add_css_class("modal-close-btn");
    m_btn_close.set_cursor("pointer");
    m_btn_close.set_halign(Gtk::Align::START); // <-- HAUT À GAUCHE
    m_btn_close.signal_clicked().connect([this]() { hide(); });
    m_top_bar.append(m_btn_close);
    m_root_box.append(m_top_bar);

    // 2. En-tête de l'application
    m_header_box.set_margin_start(24);
    m_header_box.set_margin_end(24);
    m_header_box.set_margin_bottom(8);

    m_app_icon.set_pixel_size(56);
    m_app_icon.add_css_class("installed-app-icon");
    m_header_box.append(m_app_icon);

    m_title_box.set_valign(Gtk::Align::CENTER);
    m_title_box.set_hexpand(true);

    m_name_label.add_css_class("modal-app-name");
    m_name_label.set_halign(Gtk::Align::START);
    m_title_box.append(m_name_label);

    m_id_label.add_css_class("dim-label");
    m_id_label.set_halign(Gtk::Align::START);
    m_meta_box.append(m_id_label);

    m_flatpak_badge.add_css_class("badge-source");
    m_flatpak_badge.add_css_class("badge-flatpak");
    m_meta_box.append(m_flatpak_badge);
    m_title_box.append(m_meta_box);

    m_header_box.append(m_title_box);

    m_btn_reset.set_icon_name("edit-undo-symbolic");
    m_btn_reset.set_tooltip_text("Restaurer les permissions par défaut du développeur");
    m_btn_reset.set_valign(Gtk::Align::CENTER);
    m_btn_reset.set_cursor("pointer");
    m_btn_reset.signal_clicked().connect(sigc::mem_fun(*this, &FlatpakPermissionsModal::reset_permissions));
    m_header_box.append(m_btn_reset);

    m_root_box.append(m_header_box);

    // 3. Zone de défilement pour les permissions
    m_scrolled_window.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    m_scrolled_window.set_vexpand(true);

    m_content_box.set_margin_start(24);
    m_content_box.set_margin_end(24);
    m_content_box.set_margin_bottom(24);

    m_content_box.append(m_permissions_list_box);
    m_scrolled_window.set_child(m_content_box);
    m_root_box.append(m_scrolled_window);

    set_child(m_root_box);
}

void FlatpakPermissionsModal::show_permissions(const core::InstalledApp& app) {
    m_current_app = app;
    m_name_label.set_label(app.name);
    m_id_label.set_label(app.id);

    auto display = Gdk::Display::get_default();
    auto theme = display ? Gtk::IconTheme::get_for_display(display) : nullptr;
    if (theme && theme->has_icon(app.icon)) {
        m_app_icon.set_from_icon_name(app.icon);
    } else {
        m_app_icon.set_from_icon_name("application-x-executable");
    }

    reload_permissions();
    present();
}

static const std::vector<std::pair<std::string, std::vector<FlatpakPermissionDef>>> PERMISSION_SECTIONS = {
    {"Réseau et communication", {
        {"network", "shared", "network", "Accès à Internet", "Permet à l'application de se connecter au réseau et à Internet", "network-wireless-symbolic"},
        {"ipc", "shared", "ipc", "Communication inter-processus (IPC)", "Permet d'échanger des données avec d'autres processus", "system-run-symbolic"}
    }},
    {"Affichage et audio", {
        {"wayland", "sockets", "wayland", "Serveur d'affichage Wayland", "Affichage graphique natif sous Wayland", "video-display-symbolic"},
        {"x11", "sockets", "fallback-x11", "Serveur d'affichage X11 / XWayland", "Compatibilité d'affichage pour les fenêtres X11", "window-new-symbolic"},
        {"pulseaudio", "sockets", "pulseaudio", "Serveur audio (PulseAudio / PipeWire)", "Lecture et enregistrement du son", "audio-volume-high-symbolic"}
    }},
    {"Périphériques matériels", {
        {"dri", "devices", "dri", "Accélération graphique 3D (GPU / DRI)", "Rendu 3D accéléré matériellement via la carte graphique", "video-joined-displays-symbolic"},
        {"all_devices", "devices", "all", "Tous les périphériques (/dev)", "Accès complet aux webcams, manettes et matériels", "drive-harddisk-symbolic"}
    }},
    {"Système de fichiers", {
        {"home", "filesystems", "home", "Dossier personnel (~)", "Accès en lecture et écriture à tout votre dossier personnel", "user-home-symbolic"},
        {"host", "filesystems", "host", "Système de fichiers racine (/)", "Accès complet à l'ensemble du système de fichiers", "drive-harddisk-system-symbolic"},
        {"xdg-download", "filesystems", "xdg-download", "Dossier Téléchargements", "Accès aux fichiers du dossier Téléchargements", "folder-download-symbolic"},
        {"xdg-documents", "filesystems", "xdg-documents", "Dossier Documents", "Accès aux documents personnels", "folder-documents-symbolic"},
        {"xdg-pictures", "filesystems", "xdg-pictures", "Dossier Images", "Accès aux photos et images", "folder-pictures-symbolic"},
        {"xdg-music", "filesystems", "xdg-music", "Dossier Musique", "Accès aux fichiers musicaux", "folder-music-symbolic"},
        {"xdg-videos", "filesystems", "xdg-videos", "Dossier Vidéos", "Accès aux fichiers vidéo", "folder-videos-symbolic"}
    }}
};

void FlatpakPermissionsModal::reload_permissions() {
    m_updating_ui = true;
    m_current_states.clear();

    while (auto child = m_permissions_list_box.get_first_child()) {
        m_permissions_list_box.remove(*child);
    }

    // 1. Lire les métadonnées de base déclarées par le Flatpak
    std::unordered_map<std::string, std::unordered_set<std::string>> base_context;
    try {
        std::string stdout_data;
        int exit_status = 0;
        Glib::spawn_command_line_sync("flatpak info -m " + m_current_app.id, &stdout_data, nullptr, &exit_status);
        if (exit_status == 0 && !stdout_data.empty()) {
            std::istringstream stream(stdout_data);
            std::string line;
            bool in_context = false;
            while (std::getline(stream, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (line.rfind("[", 0) == 0) {
                    in_context = (line == "[Context]");
                    continue;
                }
                if (in_context) {
                    auto eq = line.find('=');
                    if (eq != std::string::npos) {
                        std::string key = line.substr(0, eq);
                        std::string val = line.substr(eq + 1);
                        std::istringstream tok_stream(val);
                        std::string tok;
                        while (std::getline(tok_stream, tok, ';')) {
                            if (!tok.empty()) {
                                base_context[key].insert(tok);
                            }
                        }
                    }
                }
            }
        }
    } catch (...) {}

    // 2. Lire les surcharges utilisateur existantes
    std::unordered_map<std::string, std::unordered_set<std::string>> overrides;
    try {
        std::string stdout_data;
        int exit_status = 0;
        Glib::spawn_command_line_sync("flatpak override --user --show " + m_current_app.id, &stdout_data, nullptr, &exit_status);
        if (exit_status == 0 && !stdout_data.empty()) {
            std::istringstream stream(stdout_data);
            std::string line;
            bool in_context = false;
            while (std::getline(stream, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (line.rfind("[", 0) == 0) {
                    in_context = (line == "[Context]");
                    continue;
                }
                if (in_context) {
                    auto eq = line.find('=');
                    if (eq != std::string::npos) {
                        std::string key = line.substr(0, eq);
                        std::string val = line.substr(eq + 1);
                        std::istringstream tok_stream(val);
                        std::string tok;
                        while (std::getline(tok_stream, tok, ';')) {
                            if (!tok.empty()) {
                                overrides[key].insert(tok);
                            }
                        }
                    }
                }
            }
        }
    } catch (...) {}

    // 3. Affichage des groupes de permissions (Style GTK boxed-list)
    for (const auto& sec : PERMISSION_SECTIONS) {
        m_permissions_list_box.append(*create_section_header(sec.first));

        auto list_box = Gtk::make_managed<Gtk::ListBox>();
        list_box->add_css_class("boxed-list");
        list_box->set_selection_mode(Gtk::SelectionMode::NONE);

        for (const auto& def : sec.second) {
            bool is_active = false;
            const auto& cat_overrides = overrides[def.category];
            const auto& cat_base = base_context[def.category];

            if (def.key == "x11") {
                if (cat_overrides.count("!fallback-x11") || cat_overrides.count("!x11")) {
                    is_active = false;
                } else if (cat_overrides.count("fallback-x11") || cat_overrides.count("x11")) {
                    is_active = true;
                } else {
                    is_active = cat_base.count("fallback-x11") || cat_base.count("x11");
                }
            } else if (def.key == "dri") {
                if (cat_overrides.count("!dri")) {
                    is_active = false;
                } else if (cat_overrides.count("dri") || cat_overrides.count("all")) {
                    is_active = true;
                } else {
                    is_active = cat_base.count("dri") || cat_base.count("all");
                }
            } else {
                if (cat_overrides.count("!" + def.flatpak_flag)) {
                    is_active = false;
                } else if (cat_overrides.count(def.flatpak_flag)) {
                    is_active = true;
                } else {
                    is_active = cat_base.count(def.flatpak_flag) > 0;
                }
            }

            m_current_states[def.key] = is_active;
            list_box->append(*create_perm_row(def, is_active));
        }

        m_permissions_list_box.append(*list_box);
    }

    m_updating_ui = false;
}

Gtk::Label* FlatpakPermissionsModal::create_section_header(const std::string& title) {
    auto lbl = Gtk::make_managed<Gtk::Label>(title);
    lbl->add_css_class("heading");
    lbl->set_halign(Gtk::Align::START);
    lbl->set_margin_start(4);
    lbl->set_margin_top(8);
    lbl->set_margin_bottom(2);
    return lbl;
}

Gtk::ListBoxRow* FlatpakPermissionsModal::create_perm_row(const FlatpakPermissionDef& def, bool current_state) {
    auto row = Gtk::make_managed<Gtk::ListBoxRow>();
    row->set_activatable(false);
    row->set_selectable(false);

    auto row_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 14);
    row_box->set_margin_start(14);
    row_box->set_margin_end(14);
    row_box->set_margin_top(10);
    row_box->set_margin_bottom(10);

    auto icon = Gtk::make_managed<Gtk::Image>();
    icon->set_from_icon_name(def.icon_name);
    icon->set_pixel_size(22);
    icon->set_valign(Gtk::Align::CENTER);
    icon->add_css_class("dim-label");
    row_box->append(*icon);

    auto text_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 2);
    text_box->set_valign(Gtk::Align::CENTER);
    text_box->set_hexpand(true);

    auto title_lbl = Gtk::make_managed<Gtk::Label>(def.title);
    title_lbl->set_halign(Gtk::Align::START);
    text_box->append(*title_lbl);

    auto desc_lbl = Gtk::make_managed<Gtk::Label>(def.description);
    desc_lbl->add_css_class("dim-label");
    desc_lbl->set_halign(Gtk::Align::START);
    desc_lbl->set_wrap(true);
    text_box->append(*desc_lbl);

    row_box->append(*text_box);

    auto sw = Gtk::make_managed<Gtk::Switch>();
    sw->set_valign(Gtk::Align::CENTER);
    sw->set_active(current_state);

    sw->property_active().signal_changed().connect([this, def, sw]() {
        if (m_updating_ui) return;
        bool new_val = sw->get_active();
        apply_permission(def, new_val);
    });

    row_box->append(*sw);
    row->set_child(*row_box);
    return row;
}

void FlatpakPermissionsModal::apply_permission(const FlatpakPermissionDef& perm, bool enable) {
    std::string arg;
    if (perm.category == "shared") {
        arg = enable ? "--share=" + perm.flatpak_flag : "--unshare=" + perm.flatpak_flag;
    } else if (perm.category == "sockets") {
        if (perm.key == "x11") {
            arg = enable ? "--socket=fallback-x11" : "--nosocket=fallback-x11 --nosocket=x11";
        } else {
            arg = enable ? "--socket=" + perm.flatpak_flag : "--nosocket=" + perm.flatpak_flag;
        }
    } else if (perm.category == "devices") {
        arg = enable ? "--device=" + perm.flatpak_flag : "--nodevice=" + perm.flatpak_flag;
    } else if (perm.category == "filesystems") {
        arg = enable ? "--filesystem=" + perm.flatpak_flag : "--nofilesystem=" + perm.flatpak_flag;
    }

    std::string cmd = "flatpak override --user " + arg + " " + m_current_app.id;
    std::system(cmd.c_str());
    m_current_states[perm.key] = enable;
}

void FlatpakPermissionsModal::reset_permissions() {
    std::string cmd = "flatpak override --user --reset " + m_current_app.id;
    std::system(cmd.c_str());
    reload_permissions();
}

} // namespace bluehub::ui