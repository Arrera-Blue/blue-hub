#pragma once

#include <gtkmm.h>
#include "discover_page.hpp"
#include "installed_page.hpp"

namespace bluehub::ui {

class MainWindow : public Gtk::ApplicationWindow {
public:
    MainWindow();

private:
    void on_about_clicked();
    void on_install_archive_clicked();
    void on_install_appimage_clicked();
    void on_install_rpm_clicked();
    void on_preferences_clicked();

    void switch_tab(const std::string& tab_name);

    // Barre de titre / HeaderBar
    Gtk::HeaderBar m_header_bar;

    // Menu déroulant Chevron (haut gauche)
    Gtk::MenuButton m_menu_btn;
    Gtk::Popover m_menu_popover;
    Gtk::Box m_menu_box{Gtk::Orientation::VERTICAL, 8};
    Gtk::Button m_btn_install_archive{"Installer depuis une archive"};
    Gtk::Button m_btn_install_appimage{"Installer depuis une APPIMAGE"};
    Gtk::Button m_btn_install_rpm{"Installer depuis un RPM"};
    Gtk::Button m_btn_preferences{"Préférences"};
    Gtk::Button m_btn_about{"A Propos"};

    // Sélecteur d'onglets central
    Gtk::Box m_nav_switcher{Gtk::Orientation::HORIZONTAL, 4};
    Gtk::Button m_btn_discover{"Découvrir"};
    Gtk::Button m_btn_installed{"Installer"};
    Gtk::Button m_btn_updates{"Mise à jour"};

    // Bouton de recherche (haut droite)
    Gtk::Button m_search_btn;

    // Conteneur de pages (Stack)
    Gtk::Stack m_stack;
    DiscoverPage m_discover_page;
    InstalledPage m_installed_page;
    Gtk::Box m_updates_placeholder{Gtk::Orientation::VERTICAL};
};

} // namespace bluehub::ui