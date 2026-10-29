#pragma once

#include <gtkmm.h>
#include "discover_page.hpp"

namespace bluehub::ui {

class MainWindow : public Gtk::ApplicationWindow {
public:
    MainWindow();

private:
    void on_menu_clicked();

    // Barre de titre / HeaderBar
    Gtk::HeaderBar m_header_bar;
    Gtk::Button m_menu_btn;
    Gtk::Button m_search_btn;

    // Sélecteur d'onglets central
    Gtk::Box m_nav_switcher{Gtk::Orientation::HORIZONTAL, 4};
    Gtk::Button m_btn_discover{"Découvrir"};
    Gtk::Button m_btn_installed{"Installer"};
    Gtk::Button m_btn_updates{"Mise à jour"};

    // Contenu
    DiscoverPage m_discover_page;
};

} // namespace bluehub::ui