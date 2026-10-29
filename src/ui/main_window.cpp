#include "main_window.hpp"

namespace bluehub::ui {

MainWindow::MainWindow() {
    set_title("Blue Hub");
    set_default_size(1050, 700);

    // Définir l'icône de la fenêtre pour le système
    set_icon_name("blue-hub");

    // 1. Bouton Menu Hamburger (gauche)
    m_menu_btn.set_icon_name("open-menu-symbolic");
    m_menu_btn.add_css_class("flat");
    m_menu_btn.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_menu_clicked));
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

void MainWindow::on_menu_clicked() {
    auto about = Gtk::make_managed<Gtk::AboutDialog>();
    about->set_transient_for(*this);
    about->set_modal(true);
    about->set_program_name("Blue Hub");
    about->set_version("2026.1.0");
    about->set_comments("Gestionnaire d'applications et logithèque pour Arrera Blue OS");
    about->set_logo_icon_name("blue-hub");
    about->set_copyright("© 2026 Arrera Software");
    about->present();
}

} // namespace bluehub::ui