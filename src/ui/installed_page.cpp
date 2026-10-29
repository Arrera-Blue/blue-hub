#include "installed_page.hpp"

namespace bluehub::ui {

InstalledPage::InstalledPage() {
    set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);

    // Marges extérieures identiques à la page Découvrir
    m_content_box.set_margin_start(36);
    m_content_box.set_margin_end(36);
    m_content_box.set_margin_top(24);
    m_content_box.set_margin_bottom(30);

    // Reproduction exacte des 4 lignes de votre maquette
    m_content_box.append(*create_installed_row("FLATPAK APP", AppSourceType::FLATPAK));
    m_content_box.append(*create_installed_row("RPM APP", AppSourceType::RPM));
    m_content_box.append(*create_installed_row("DNF APP", AppSourceType::DNF));
    m_content_box.append(*create_installed_row("ARCHIVE APP", AppSourceType::ARCHIVE));

    set_child(m_content_box);
}

Gtk::Box* InstalledPage::create_installed_row(const std::string& name, AppSourceType type) {
    auto row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 16);
    row->add_css_class("installed-row");

    // 1. Placeholder de l'icône
    auto icon_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    icon_box->add_css_class("app-icon-placeholder");
    icon_box->set_valign(Gtk::Align::CENTER);
    icon_box->set_halign(Gtk::Align::CENTER);

    auto icon_label = Gtk::make_managed<Gtk::Label>("APP");
    icon_label->set_expand(true);
    icon_box->append(*icon_label);
    row->append(*icon_box);

    // 2. Nom de l'application
    auto title_label = Gtk::make_managed<Gtk::Label>(name);
    title_label->add_css_class("installed-app-title");
    title_label->set_halign(Gtk::Align::START);
    title_label->set_valign(Gtk::Align::CENTER);
    title_label->set_hexpand(true);
    row->append(*title_label);

    // 3. Boutons d'actions à droite
    auto actions_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    actions_box->set_valign(Gtk::Align::CENTER);

    // Cas Flatpak : Bouton permissions (clé)
    if (type == AppSourceType::FLATPAK) {
        auto perm_btn = Gtk::make_managed<Gtk::Button>();
        perm_btn->set_icon_name("dialog-password-symbolic");
        perm_btn->add_css_class("row-action-btn");
        perm_btn->set_tooltip_text("Gérer les permissions Flatpak");
        actions_box->append(*perm_btn);
    }

    // Cas Archive ou AppImage : Bouton changer l'icône
    if (type == AppSourceType::ARCHIVE || type == AppSourceType::APPIMAGE) {
        auto icon_btn = Gtk::make_managed<Gtk::Button>();
        icon_btn->set_icon_name("insert-image-symbolic");
        icon_btn->add_css_class("row-action-btn");
        icon_btn->set_tooltip_text("Changer l'icône de l'application");
        actions_box->append(*icon_btn);
    }

    // Bouton de désinstallation (poubelle)
    auto delete_btn = Gtk::make_managed<Gtk::Button>();
    delete_btn->set_icon_name("user-trash-symbolic");
    delete_btn->add_css_class("row-action-btn");
    delete_btn->set_tooltip_text("Désinstaller");
    actions_box->append(*delete_btn);

    row->append(*actions_box);

    return row;
}

} // namespace bluehub::ui