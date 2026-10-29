#include "main_window.hpp"

namespace bluehub::ui {

MainWindow::MainWindow() {
    set_title("Blue Hub");
    set_default_size(1050, 700);

    // 1. Bouton Menu Hamburger (gauche)
    m_menu_btn.set_icon_name("open-menu-symbolic");
    m_menu_btn.add_css_class("flat");
    m_header_bar.pack_start(m_menu_btn);

    // 2. Sélecteur central à 3 onglets
    m_nav_switcher.add_css_class("nav-switcher");

    auto setup_nav_btn = [](Gtk::Button& btn, bool active = false) {
        btn.add_css_class("nav-pill");
        if (active) {
            btn.add_css_class("active");
        }
    };

    setup_nav_btn(m_btn_discover, true);
    setup_nav_btn(m_btn_installed, false);
    setup_nav_btn(m_btn_updates, false);

    m_nav_switcher.append(m_btn_discover);
    m_nav_switcher.append(m_btn_installed);
    m_nav_switcher.append(m_btn_updates);

    m_header_bar.set_title_widget(m_nav_switcher);

    // 3. Bouton Recherche Loupe (droite)
    m_search_btn.set_icon_name("system-search-symbolic");
    m_search_btn.add_css_class("flat");
    m_header_bar.pack_end(m_search_btn);

    // Associer la HeaderBar à la fenêtre
    set_titlebar(m_header_bar);

    // Définir la page Découvrir comme vue actuelle
    set_child(m_discover_page);
}

} // namespace bluehub::ui