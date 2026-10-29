#include "discover_page.hpp"
#include <cmath>
#include <filesystem>

namespace bluehub::ui {

DiscoverPage::DiscoverPage() {
    set_transition_type(Gtk::StackTransitionType::CROSSFADE);
    set_transition_duration(250);

    // --- Configuration du contenu en ligne (ScrolledWindow) ---
    m_scrolled_window.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    m_content_box.set_margin_start(36);
    m_content_box.set_margin_end(36);
    m_content_box.set_margin_top(20);
    m_content_box.set_margin_bottom(30);

    // 1. Initialisation du Carrousel Adwaita
    m_carousel_widget = adw_carousel_new();
    adw_carousel_set_spacing(ADW_CAROUSEL(m_carousel_widget), 20);
    adw_carousel_set_allow_mouse_drag(ADW_CAROUSEL(m_carousel_widget), TRUE);
    adw_carousel_set_allow_scroll_wheel(ADW_CAROUSEL(m_carousel_widget), TRUE);

    auto carousel_cpp = Glib::wrap(m_carousel_widget);
    carousel_cpp->set_hexpand(true);
    carousel_cpp->set_halign(Gtk::Align::FILL);

    // Flèches de navigation gauche / droite
    m_btn_prev.set_icon_name("go-previous-symbolic");
    m_btn_prev.add_css_class("carousel-nav-btn");
    m_btn_prev.set_cursor("pointer");
    m_btn_prev.set_valign(Gtk::Align::CENTER);
    m_btn_prev.signal_clicked().connect(sigc::mem_fun(*this, &DiscoverPage::on_prev_clicked));

    m_btn_next.set_icon_name("go-next-symbolic");
    m_btn_next.add_css_class("carousel-nav-btn");
    m_btn_next.set_cursor("pointer");
    m_btn_next.set_valign(Gtk::Align::CENTER);
    m_btn_next.signal_clicked().connect(sigc::mem_fun(*this, &DiscoverPage::on_next_clicked));

    m_carousel_nav_row.append(m_btn_prev);
    m_carousel_nav_row.append(*carousel_cpp);
    m_carousel_nav_row.append(m_btn_next);

    // Indicateur à 5 points
    m_dots_widget = adw_carousel_indicator_dots_new();
    adw_carousel_indicator_dots_set_carousel(ADW_CAROUSEL_INDICATOR_DOTS(m_dots_widget), ADW_CAROUSEL(m_carousel_widget));

    m_carousel_container.add_css_class("hero-carousel-container");
    m_carousel_container.append(m_carousel_nav_row);
    m_carousel_container.append(*Glib::wrap(m_dots_widget));

    m_content_box.append(m_carousel_container);

    // 2. Grille des 6 catégories
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

    m_categories_grid.attach(m_btn_create, 0, 0);
    m_categories_grid.attach(m_btn_work, 1, 0);
    m_categories_grid.attach(m_btn_play, 2, 0);

    m_categories_grid.attach(m_btn_socialise, 0, 1);
    m_categories_grid.attach(m_btn_learn, 1, 1);
    m_categories_grid.attach(m_btn_develop, 2, 1);

    m_content_box.append(m_categories_grid);

    // 3. Rangée des cartes d'applications
    m_apps_row.set_homogeneous(true);
    m_apps_row.append(*create_app_card("APP"));
    m_apps_row.append(*create_app_card("APP"));
    m_apps_row.append(*create_app_card("APP"));
    m_apps_row.append(*create_app_card("APP"));

    m_content_box.append(m_apps_row);

    m_scrolled_window.set_child(m_content_box);
    add(m_scrolled_window, "content");

    // --- Configuration de la vue Hors-Ligne (AdwStatusPage) ---
    m_status_page = adw_status_page_new();
    adw_status_page_set_icon_name(ADW_STATUS_PAGE(m_status_page), "network-offline-symbolic");
    adw_status_page_set_title(ADW_STATUS_PAGE(m_status_page), "Aucune connexion Internet");
    adw_status_page_set_description(ADW_STATUS_PAGE(m_status_page), "Connectez-vous à Internet pour charger la logithèque et découvrir de nouvelles applications.");

    m_btn_retry.add_css_class("pill");
    m_btn_retry.add_css_class("suggested-action");
    m_btn_retry.set_halign(Gtk::Align::CENTER);
    m_btn_retry.set_cursor("pointer");
    m_btn_retry.signal_clicked().connect(sigc::mem_fun(*this, &DiscoverPage::on_retry_clicked));

    adw_status_page_set_child(ADW_STATUS_PAGE(m_status_page), GTK_WIDGET(m_btn_retry.gobj()));
    add(*Glib::wrap(m_status_page), "offline");

    // Affichage par défaut pendant la vérification
    set_visible_child("content");

    // 4. Lancement de la récupération API Flathub en arrière-plan
    m_dispatcher.connect(sigc::mem_fun(*this, &DiscoverPage::on_trending_loaded));
    load_flathub_trending_async();
}

DiscoverPage::~DiscoverPage() {
    if (m_fetch_thread.joinable()) {
        m_fetch_thread.join();
    }
}

void DiscoverPage::load_flathub_trending_async() {
    if (m_fetch_thread.joinable()) {
        m_fetch_thread.join();
    }

    m_fetch_thread = std::thread([this]() {
        auto live_apps = core::FlathubService::get_trending_apps(5);
        if (!live_apps.empty()) {
            m_fetched_apps = std::move(live_apps);
            m_has_connection = true;
        } else {
            m_has_connection = false;
        }
        m_dispatcher.emit();
    });
}

void DiscoverPage::on_retry_clicked() {
    load_flathub_trending_async();
}

void DiscoverPage::on_trending_loaded() {
    if (!m_has_connection) {
        // Basculer sur la page d'état hors-ligne
        set_visible_child("offline");
        return;
    }

    // Afficher la page de contenu normale
    set_visible_child("content");

    // Vider les anciennes diapositives
    guint n_pages = adw_carousel_get_n_pages(ADW_CAROUSEL(m_carousel_widget));
    for (guint i = 0; i < n_pages; ++i) {
        auto page = adw_carousel_get_nth_page(ADW_CAROUSEL(m_carousel_widget), 0);
        if (page) {
            adw_carousel_remove(ADW_CAROUSEL(m_carousel_widget), page);
        }
    }

    // Ajouter les vraies applications Flathub sans le badge
    for (const auto& app : m_fetched_apps) {
        auto slide = create_slide(app.name, app.summary, app.id, app.local_icon_path, app.color_class);
        adw_carousel_append(ADW_CAROUSEL(m_carousel_widget), GTK_WIDGET(slide->gobj()));
    }
}

void DiscoverPage::on_prev_clicked() {
    auto carousel = ADW_CAROUSEL(m_carousel_widget);
    guint total = adw_carousel_get_n_pages(carousel);
    if (total > 0) {
        int current = static_cast<int>(std::round(adw_carousel_get_position(carousel)));
        int prev = (current - 1 + total) % total;
        auto target = adw_carousel_get_nth_page(carousel, prev);
        if (target) {
            adw_carousel_scroll_to(carousel, target, TRUE);
        }
    }
}

void DiscoverPage::on_next_clicked() {
    auto carousel = ADW_CAROUSEL(m_carousel_widget);
    guint total = adw_carousel_get_n_pages(carousel);
    if (total > 0) {
        int current = static_cast<int>(std::round(adw_carousel_get_position(carousel)));
        int next = (current + 1) % total;
        auto target = adw_carousel_get_nth_page(carousel, next);
        if (target) {
            adw_carousel_scroll_to(carousel, target, TRUE);
        }
    }
}

Gtk::Box* DiscoverPage::create_slide(const std::string& name, const std::string& tagline, const std::string& icon_name, const std::string& local_icon_path, const std::string& color_class) {
    auto slide = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 24);
    slide->add_css_class("hero-slide");
    slide->add_css_class(color_class);
    slide->set_hexpand(true);
    slide->set_halign(Gtk::Align::FILL);

    // 1. Icône de l'application sans halo restrictif
    auto icon_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    icon_box->add_css_class("hero-slide-icon-box");
    icon_box->set_valign(Gtk::Align::CENTER);
    icon_box->set_halign(Gtk::Align::CENTER);

    // Priorité 1 : Image téléchargée depuis Flathub
    if (!local_icon_path.empty() && std::filesystem::exists(local_icon_path)) {
        auto icon_img = Gtk::make_managed<Gtk::Image>();
        icon_img->set(local_icon_path);
        icon_img->set_pixel_size(72);
        icon_img->set_expand(true);
        icon_box->append(*icon_img);
    } else {
        // Priorité 2 : Thème local du système
        auto display = Gdk::Display::get_default();
        bool icon_exists = false;
        if (display) {
            auto theme = Gtk::IconTheme::get_for_display(display);
            icon_exists = theme && theme->has_icon(icon_name);
        }

        if (icon_exists) {
            auto icon_img = Gtk::make_managed<Gtk::Image>();
            icon_img->set_from_icon_name(icon_name);
            icon_img->set_pixel_size(72);
            icon_img->set_expand(true);
            icon_box->append(*icon_img);
        } else {
            // Priorité 3 : Initiale de secours
            std::string initial = name.empty() ? "?" : name.substr(0, 1);
            auto letter_label = Gtk::make_managed<Gtk::Label>(initial);
            letter_label->add_css_class("hero-fallback-letter");
            letter_label->set_expand(true);
            letter_label->set_valign(Gtk::Align::CENTER);
            letter_label->set_halign(Gtk::Align::CENTER);
            icon_box->append(*letter_label);
        }
    }

    slide->append(*icon_box);

    // 2. Textes (Titre + Description)
    auto info_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 8);
    info_box->set_valign(Gtk::Align::CENTER);
    info_box->set_hexpand(true);

    auto title_label = Gtk::make_managed<Gtk::Label>(name);
    title_label->add_css_class("hero-app-title");
    title_label->set_halign(Gtk::Align::START);

    auto tagline_label = Gtk::make_managed<Gtk::Label>(tagline);
    tagline_label->add_css_class("hero-app-tagline");
    tagline_label->set_halign(Gtk::Align::START);
    tagline_label->set_wrap(true);

    info_box->append(*title_label);
    info_box->append(*tagline_label);
    slide->append(*info_box);

    return slide;
}

Gtk::Box* DiscoverPage::create_app_card(const std::string& name) {
    auto card = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 14);
    card->add_css_class("app-card");

    auto icon_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    icon_box->add_css_class("app-icon-placeholder");
    icon_box->set_valign(Gtk::Align::CENTER);
    icon_box->set_halign(Gtk::Align::CENTER);

    auto icon_label = Gtk::make_managed<Gtk::Label>("APP");
    icon_label->set_expand(true);
    icon_box->append(*icon_label);

    auto title_label = Gtk::make_managed<Gtk::Label>(name);
    title_label->add_css_class("app-title");
    title_label->set_halign(Gtk::Align::START);
    title_label->set_valign(Gtk::Align::CENTER);

    card->append(*icon_box);
    card->append(*title_label);

    return card;
}

} // namespace bluehub::ui
