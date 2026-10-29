#include "main_window.hpp"

namespace bluehub::ui {

MainWindow::MainWindow() {
    set_title("Blue Hub");
    set_default_size(1050, 700);

    // Définir l'icône de la fenêtre
    set_icon_name("blue-hub");

    // 1. Menu déroulant haut gauche (bascule dynamique hamburger / flèche)
    m_menu_btn.set_icon_name("open-menu-symbolic");
    m_menu_btn.add_css_class("flat");

    m_menu_btn.property_active().signal_changed().connect([this]() {
        if (m_menu_btn.property_active().get_value()) {
            m_menu_btn.set_icon_name("pan-down-symbolic");
        } else {
            m_menu_btn.set_icon_name("open-menu-symbolic");
        }
    });

    auto setup_menu_btn = [](Gtk::Button& btn) {
        btn.add_css_class("menu-item-btn");
        btn.set_cursor("pointer");
    };

    setup_menu_btn(m_btn_install_archive);
    setup_menu_btn(m_btn_install_appimage);
    setup_menu_btn(m_btn_install_rpm);
    setup_menu_btn(m_btn_preferences);
    setup_menu_btn(m_btn_about);

    m_btn_install_archive.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_install_archive_clicked));
    m_btn_install_appimage.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_install_appimage_clicked));
    m_btn_install_rpm.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_install_rpm_clicked));
    m_btn_preferences.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_preferences_clicked));
    m_btn_about.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_about_clicked));

    m_menu_box.add_css_class("app-menu-box");
    m_menu_box.append(m_btn_install_archive);
    m_menu_box.append(m_btn_install_appimage);
    m_menu_box.append(m_btn_install_rpm);
    m_menu_box.append(m_btn_preferences);
    m_menu_box.append(m_btn_about);

    m_menu_popover.set_child(m_menu_box);
    m_menu_btn.set_popover(m_menu_popover);
    m_header_bar.pack_start(m_menu_btn);

    // 2. Sélecteur central d'onglets
    m_nav_switcher.add_css_class("nav-switcher");

    m_btn_discover.add_css_class("nav-pill");
    m_btn_installed.add_css_class("nav-pill");
    m_btn_updates.add_css_class("nav-pill");

    m_btn_discover.signal_clicked().connect([this]() { switch_tab("discover"); });
    m_btn_installed.signal_clicked().connect([this]() { switch_tab("installed"); });
    m_btn_updates.signal_clicked().connect([this]() { switch_tab("updates"); });

    m_nav_switcher.append(m_btn_discover);
    m_nav_switcher.append(m_btn_installed);
    m_nav_switcher.append(m_btn_updates);
    m_header_bar.set_title_widget(m_nav_switcher);

    // 3. Bouton Recherche Loupe (haut droite)
    m_search_btn.set_icon_name("system-search-symbolic");
    m_search_btn.add_css_class("flat");
    m_header_bar.pack_end(m_search_btn);

    set_titlebar(m_header_bar);

    // 4. Configuration de la Stack (navigation)
    m_stack.set_transition_type(Gtk::StackTransitionType::CROSSFADE);
    m_stack.set_transition_duration(200);

    m_stack.add(m_discover_page, "discover");
    m_stack.add(m_installed_page, "installed");
    m_stack.add(m_updates_placeholder, "updates");

    // Onglet par défaut : Découvrir
    switch_tab("discover");

    set_child(m_stack);
}

void MainWindow::switch_tab(const std::string& tab_name) {
    m_stack.set_visible_child(tab_name);

    // Mise à jour de l'apparence des boutons piliers
    m_btn_discover.remove_css_class("active");
    m_btn_installed.remove_css_class("active");
    m_btn_updates.remove_css_class("active");

    if (tab_name == "discover") {
        m_btn_discover.add_css_class("active");
    } else if (tab_name == "installed") {
        m_btn_installed.add_css_class("active");
    } else if (tab_name == "updates") {
        m_btn_updates.add_css_class("active");
    }
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