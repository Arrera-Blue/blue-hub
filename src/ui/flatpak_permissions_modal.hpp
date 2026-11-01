#pragma once

#include <gtkmm.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "core/abpm_service.hpp"

namespace bluehub::ui {

struct FlatpakPermissionDef {
    std::string key;          // Identifiant
    std::string category;     // "shared", "sockets", "devices", "filesystems"
    std::string flatpak_flag; // Drapeau flatpak
    std::string title;
    std::string description;
    std::string icon_name;
};

class FlatpakPermissionsModal : public Gtk::Window {
public:
    explicit FlatpakPermissionsModal(Gtk::Window& parent);
    void show_permissions(const core::InstalledApp& app);

private:
    void reload_permissions();
    void apply_permission(const FlatpakPermissionDef& perm, bool enable);
    void reset_permissions();

    Gtk::Label* create_section_header(const std::string& title);
    Gtk::ListBoxRow* create_perm_row(const FlatpakPermissionDef& def, bool current_state);

    Gtk::Box m_root_box{Gtk::Orientation::VERTICAL, 12};
    Gtk::ScrolledWindow m_scrolled_window;
    Gtk::Box m_content_box{Gtk::Orientation::VERTICAL, 14};

    // 1. Bouton fermer [✕] placé EN HAUT À GAUCHE
    Gtk::Box m_top_bar{Gtk::Orientation::HORIZONTAL};
    Gtk::Button m_btn_close;

    // 2. En-tête : Icône, Nom, ID, Badge Flatpak et Bouton Réinitialiser
    Gtk::Box m_header_box{Gtk::Orientation::HORIZONTAL, 16};
    Gtk::Image m_app_icon;
    Gtk::Box m_title_box{Gtk::Orientation::VERTICAL, 4};
    Gtk::Label m_name_label;
    Gtk::Box m_meta_box{Gtk::Orientation::HORIZONTAL, 8};
    Gtk::Label m_id_label;
    Gtk::Label m_flatpak_badge{"Flatpak"};
    Gtk::Button m_btn_reset{"Restaurer"};

    // 3. Liste des permissions groupées
    Gtk::Box m_permissions_list_box{Gtk::Orientation::VERTICAL, 12};

    core::InstalledApp m_current_app;
    std::unordered_map<std::string, bool> m_current_states;
    bool m_updating_ui{false};
};

} // namespace bluehub::ui