#include "discover_page.hpp"

namespace bluehub::ui {

DiscoverPage::DiscoverPage() {
    set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);

    // Marges extérieures
    m_content_box.set_margin_start(36);
    m_content_box.set_margin_end(36);
    m_content_box.set_margin_top(20);
    m_content_box.set_margin_bottom(30);

    // 1. Grande bannière supérieure
    m_hero_banner.set_size_request(-1, 240);
    m_hero_banner.add_css_class("hero-banner");
    m_content_box.append(m_hero_banner);

    // 2. Grille des 6 catégories (2x3)
    m_categories_grid.set_row_spacing(16);
    m_categories_grid.set_column_spacing(16);
    m_categories_grid.set_column_homogeneous(true);
    m_categories_grid.set_row_homogeneous(true);

    auto setup_cat_btn = [](Gtk::Button& btn) {
        btn.add_css_class("category-card");
        btn.set_cursor("pointer");
    };

    setup_cat_btn(m_btn_create);
    setup_cat_btn(m_btn_work);
    setup_cat_btn(m_btn_play);
    setup_cat_btn(m_btn_socialise);
    setup_cat_btn(m_btn_learn);
    setup_cat_btn(m_btn_develop);

    // Ligne 1
    m_categories_grid.attach(m_btn_create, 0, 0);
    m_categories_grid.attach(m_btn_work, 1, 0);
    m_categories_grid.attach(m_btn_play, 2, 0);

    // Ligne 2
    m_categories_grid.attach(m_btn_socialise, 0, 1);
    m_categories_grid.attach(m_btn_learn, 1, 1);
    m_categories_grid.attach(m_btn_develop, 2, 1);

    m_content_box.append(m_categories_grid);

    // 3. Rangée des 4 cartes d'applications
    m_apps_row.set_homogeneous(true);
    m_apps_row.append(*create_app_card("APP"));
    m_apps_row.append(*create_app_card("APP"));
    m_apps_row.append(*create_app_card("APP"));
    m_apps_row.append(*create_app_card("APP"));

    m_content_box.append(m_apps_row);

    set_child(m_content_box);
}

Gtk::Box* DiscoverPage::create_app_card(const std::string& name) {
    auto card = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 14);
    card->add_css_class("app-card");

    // Placeholder d'icône carrée avec texte "APP"
    auto icon_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    icon_box->add_css_class("app-icon-placeholder");
    icon_box->set_valign(Gtk::Align::CENTER);
    icon_box->set_halign(Gtk::Align::CENTER);

    auto icon_label = Gtk::make_managed<Gtk::Label>("APP");
    icon_label->set_expand(true);
    icon_box->append(*icon_label);

    // Titre de l'application
    auto title_label = Gtk::make_managed<Gtk::Label>(name);
    title_label->add_css_class("app-title");
    title_label->set_halign(Gtk::Align::START);
    title_label->set_valign(Gtk::Align::CENTER);

    card->append(*icon_box);
    card->append(*title_label);

    return card;
}

} // namespace bluehub::ui