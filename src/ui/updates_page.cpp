#include "updates_page.hpp"

namespace bluehub::ui {

UpdatesPage::UpdatesPage() {
    set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);

    m_content_box.set_margin_start(36);
    m_content_box.set_margin_end(36);
    m_content_box.set_margin_top(28);
    m_content_box.set_margin_bottom(36);

    // 1. En-tête : Titre principal et gros bouton tout mettre à jour
    m_status_label.add_css_class("updates-title");
    m_status_label.set_halign(Gtk::Align::START);
    m_status_label.set_hexpand(true);

    m_btn_update_all.add_css_class("btn-update");
    m_btn_update_all.set_cursor("pointer");
    m_btn_update_all.set_valign(Gtk::Align::CENTER);

    m_header_box.append(m_status_label);
    m_header_box.append(m_btn_update_all);
    m_content_box.append(m_header_box);

    m_content_box.append(*create_separator());

    // 2. Section 1 : Applications à mettre à jour
    auto sec1_label = Gtk::make_managed<Gtk::Label>("Application a mettre a jour :");
    sec1_label->add_css_class("updates-section-title");
    sec1_label->set_halign(Gtk::Align::START);
    m_content_box.append(*sec1_label);

    m_content_box.append(*create_update_row("APP", true));
    m_content_box.append(*create_update_row("APP", true));

    m_content_box.append(*create_separator());

    // 3. Section 2 : Utilitaires à mettre à jour
    auto sec2_label = Gtk::make_managed<Gtk::Label>("Utilitaire a mettre a jour :");
    sec2_label->add_css_class("updates-section-title");
    sec2_label->set_halign(Gtk::Align::START);
    m_content_box.append(*sec2_label);

    m_content_box.append(*create_update_row("UTILITAIRE", false));
    m_content_box.append(*create_update_row("UTILITAIRE", false));

    m_content_box.append(*create_separator());

    // 4. Section 3 : Composants Système à mettre à jour
    auto sec3_label = Gtk::make_managed<Gtk::Label>("Composant Systeme a mettre a jour :");
    sec3_label->add_css_class("updates-section-title");
    sec3_label->set_halign(Gtk::Align::START);
    m_content_box.append(*sec3_label);

    m_content_box.append(*create_update_row("COMPOSANT", false));
    m_content_box.append(*create_update_row("COMPOSANT", false));

    set_child(m_content_box);
}

Gtk::Box* UpdatesPage::create_update_row(const std::string& name, bool with_icon) {
    auto row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 16);
    row->add_css_class("installed-row");

    // Icône APP facultative (présente sur les applications, absente sur utilitaires/système)
    if (with_icon) {
        auto icon_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
        icon_box->add_css_class("app-icon-placeholder");
        icon_box->set_valign(Gtk::Align::CENTER);
        icon_box->set_halign(Gtk::Align::CENTER);

        auto icon_label = Gtk::make_managed<Gtk::Label>("APP");
        icon_label->set_expand(true);
        icon_box->append(*icon_label);
        row->append(*icon_box);
    }

    // Nom
    auto title_label = Gtk::make_managed<Gtk::Label>(name);
    title_label->add_css_class("installed-app-title");
    title_label->set_halign(Gtk::Align::START);
    title_label->set_valign(Gtk::Align::CENTER);
    title_label->set_hexpand(true);
    row->append(*title_label);

    // Bouton individuel bleu 'Mettre a jour'
    auto update_btn = Gtk::make_managed<Gtk::Button>("Mettre a jour");
    update_btn->add_css_class("btn-update");
    update_btn->set_cursor("pointer");
    update_btn->set_valign(Gtk::Align::CENTER);
    row->append(*update_btn);

    return row;
}

Gtk::Separator* UpdatesPage::create_separator() {
    auto sep = Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::HORIZONTAL);
    sep->add_css_class("updates-separator");
    return sep;
}

} // namespace bluehub::ui