#include "main_window.hpp"

namespace bluehub::ui {

MainWindow::MainWindow() {
    set_title("Blue Hub");
    set_default_size(1050, 700);

    // Définir l'icône de la fenêtre pour le système
    set_icon_name("blue-hub");

    // 1. Bouton Menu déroulant (haut gauche)
    // Par défaut fermé : icône hamburger
    m_menu_btn.set_icon_name("open-menu-symbolic");
    m_menu_btn.add_css_class("flat");

    // Changement dynamique de l'icône : flèche quand déployé, hamburger quand fermé
    m_menu_btn.property_active().signal_changed().connect([this]() {
        if (m_menu_btn.property_active().get_value()) {
            m_menu_btn.set_icon_name("pan-down-symbolic");
        } else {
            m_menu_btn.set_icon_name("open-menu-symbolic");
        }
    });

    // Style des 5 boutons du menu
    auto setup_menu_btn = [](Gtk::Button& btn) {
        btn.add_css_class("menu-item-btn");
        btn.set_cursor("pointer");
    };

    setup_menu_btn(m_btn_install_archive);
    setup_menu_btn(m_btn_install_appimage);
    setup_menu_btn(m_btn_install_rpm);
    setup_menu_btn(m_btn_preferences);
    setup_menu_btn(m_btn_about);

    // Connexion des signaux
    m_btn_install_archive.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_install_archive_clicked));
    m_btn_install_appimage.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_install_appimage_clicked));
    m_btn_install_rpm.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_install_rpm_clicked));
    m_btn_preferences.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_preferences_clicked));
    m_btn_about.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_about_clicked));

    // Construction de la boîte verticale du popover
    m_menu_box.add_css_class("app-menu-box");
    m_menu_box.append(m_btn_install_archive);
    m_menu_box.append(m_btn_install_appimage);
    m_menu_box.append(m_btn_install_rpm);
    m_menu_box.append(m_btn_preferences);
    m_menu_box.append(m_btn_about);

    m_menu_popover.set_child(m_menu_box);
    m_menu_btn.set_popover(m_menu_popover);

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

    // 3. Bouton Recherche Loupe (haut droite)
    m_search_btn.set_icon_name("system-search-symbolic");
    m_search_btn.add_css_class("flat");
    m_header_bar.pack_end(m_search_btn);

    // Associer la HeaderBar à la fenêtre
    set_titlebar(m_header_bar);

    // Définir la page Découvrir comme vue actuelle
    set_child(m_discover_page);
}

void MainWindow::on_about_clicked() {
    m_menu_popover.popdown();

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

void MainWindow::on_install_archive_clicked() {
    m_menu_popover.popdown();
    g_print("[Action] Installer depuis une archive cliqué\n");
}

void MainWindow::on_install_appimage_clicked() {
    m_menu_popover.popdown();
    g_print("[Action] Installer depuis une APPIMAGE cliqué\n");
}

void MainWindow::on_install_rpm_clicked() {
    m_menu_popover.popdown();
    g_print("[Action] Installer depuis un RPM cliqué\n");
}

void MainWindow::on_preferences_clicked() {
    m_menu_popover.popdown();
    g_print("[Action] Préférences cliqué\n");
}

} // namespace bluehub::ui