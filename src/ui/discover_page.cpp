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

    // 2. Grille des 6 catégories façon GNOME Software (2x3)
    m_categories_grid.set_row_spacing(16);
    m_categories_grid.set_column_spacing(16);
    m_categories_grid.set_column_homogeneous(true);
    m_categories_grid.set_row_homogeneous(true);

    auto btn_create = create_category_card("Créer", "Create", "applications-graphics-symbolic", "cat-create");
    auto btn_work = create_category_card("Travailler", "Work", "folder-documents-symbolic", "cat-work");
    auto btn_play = create_category_card("Jouer", "Play", "input-gaming-symbolic", "cat-play");
    auto btn_socialise = create_category_card("Échanger", "Socialise", "chat-message-new-symbolic", "cat-socialise");
    auto btn_learn = create_category_card("Apprendre", "Learn", "help-browser-symbolic", "cat-learn");
    auto btn_develop = create_category_card("Développer", "Develop", "utilities-terminal-symbolic", "cat-develop");

    // Ligne 1
    m_categories_grid.attach(*btn_create, 0, 0);
    m_categories_grid.attach(*btn_work, 1, 0);
    m_categories_grid.attach(*btn_play, 2, 0);

    // Ligne 2
    m_categories_grid.attach(*btn_socialise, 0, 1);
    m_categories_grid.attach(*btn_learn, 1, 1);
    m_categories_grid.attach(*btn_develop, 2, 1);

    m_content_box.append(m_categories_grid);

    // 3. Section des applications recommandées avec défilement de 6 en 6
    const char* const* langs = g_get_language_names();
    bool is_french = (langs && langs[0] && std::string(langs[0]).rfind("fr", 0) == 0);

    m_apps_section_title.set_label(is_french ? "Applications populaires" : "Popular Applications");
    m_apps_section_title.add_css_class("section-title");
    m_apps_section_title.set_halign(Gtk::Align::START);
    m_content_box.append(m_apps_section_title);

    m_apps_grid.set_column_spacing(12);
    m_apps_grid.set_row_spacing(12);
    m_apps_grid.set_column_homogeneous(true);
    m_content_box.append(m_apps_grid);

    // Bouton de chargement supplémentaire
    m_btn_load_more.set_label(is_french ? "Charger plus d'applications" : "Load more applications");
    m_btn_load_more.add_css_class("pill");
    m_btn_load_more.add_css_class("load-more-btn");
    m_btn_load_more.set_halign(Gtk::Align::CENTER);
    m_btn_load_more.set_cursor("pointer");
    m_btn_load_more.signal_clicked().connect([this]() {
        load_discover_apps_async();
    });
    m_content_box.append(m_btn_load_more);

    m_scrolled_window.set_child(m_content_box);

    // Contrôleur de molette de souris (détecte chaque impulsion vers le bas)
    auto scroll_ctrl = Gtk::EventControllerScroll::create();
    scroll_ctrl->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL);
    scroll_ctrl->signal_scroll().connect([this](double, double dy) -> bool {
        if (dy > 0 && !m_is_loading_apps && m_has_more_apps) {
            auto vadj = m_scrolled_window.get_vadjustment();
            double val = vadj->get_value();
            double max = vadj->get_upper() - vadj->get_page_size();
            if (max <= 0 || val >= max - 80.0) {
                load_discover_apps_async();
            }
        }
        return false;
    }, false);
    m_scrolled_window.add_controller(scroll_ctrl);

    // Détection par l'ascenseur
    auto vadj = m_scrolled_window.get_vadjustment();
    vadj->signal_value_changed().connect([this, vadj]() {
        double val = vadj->get_value();
        double max = vadj->get_upper() - vadj->get_page_size();
        if (max > 0 && val >= max - 80.0 && !m_is_loading_apps && m_has_more_apps) {
            load_discover_apps_async();
        }
    });

    add(m_scrolled_window, "content");

    // --- Configuration de la vue Hors-Ligne (AdwStatusPage) ---
    m_status_page = adw_status_page_new();
    adw_status_page_set_icon_name(ADW_STATUS_PAGE(m_status_page), "network-offline-symbolic");
    adw_status_page_set_title(ADW_STATUS_PAGE(m_status_page), 
        is_french ? "Aucune connexion Internet" : "No Internet Connection");
    adw_status_page_set_description(ADW_STATUS_PAGE(m_status_page), 
        is_french ? "Connectez-vous à Internet pour charger la logithèque et découvrir de nouvelles applications." 
                  : "Connect to the Internet to load the software hub and discover new apps.");

    m_btn_retry.set_label(is_french ? "Réessayer" : "Try Again");
    m_btn_retry.add_css_class("pill");
    m_btn_retry.add_css_class("suggested-action");
    m_btn_retry.set_halign(Gtk::Align::CENTER);
    m_btn_retry.set_cursor("pointer");
    m_btn_retry.signal_clicked().connect(sigc::mem_fun(*this, &DiscoverPage::on_retry_clicked));

    adw_status_page_set_child(ADW_STATUS_PAGE(m_status_page), GTK_WIDGET(m_btn_retry.gobj()));
    add(*Glib::wrap(m_status_page), "offline");

    set_visible_child("content");

    // 4. Lancement de la récupération asynchrone
    m_dispatcher.connect(sigc::mem_fun(*this, &DiscoverPage::on_trending_loaded));
    load_flathub_trending_async();

    m_apps_dispatcher.connect(sigc::mem_fun(*this, &DiscoverPage::on_discover_apps_loaded));
    load_discover_apps_async();
}

DiscoverPage::~DiscoverPage() {
    if (m_fetch_thread.joinable()) {
        m_fetch_thread.join();
    }
    if (m_apps_thread.joinable()) {
        m_apps_thread.join();
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

void DiscoverPage::load_discover_apps_async() {
    if (m_is_loading_apps || !m_has_more_apps) {
        return;
    }
    m_is_loading_apps = true;
    m_btn_load_more.set_sensitive(false);

    if (m_apps_thread.joinable()) {
        m_apps_thread.join();
    }

    int page_to_fetch = m_current_page;
    m_apps_thread = std::thread([this, page_to_fetch]() {
        auto apps = core::AbpmService::get_discover_apps(page_to_fetch, 5);
        m_incoming_apps = std::move(apps);
        m_apps_dispatcher.emit();
    });
}

void DiscoverPage::on_discover_apps_loaded() {
    if (m_apps_thread.joinable()) {
        m_apps_thread.join();
    }
    m_is_loading_apps = false;
    m_btn_load_more.set_sensitive(true);

    if (m_incoming_apps.empty()) {
        m_has_more_apps = false;
        m_btn_load_more.set_visible(false);
        return;
    }

    for (const auto& app : m_incoming_apps) {
        auto card = create_app_card(app);
        int col = m_loaded_apps_count % 5;
        int row = m_loaded_apps_count / 5;
        m_apps_grid.attach(*card, col, row);
        m_loaded_apps_count++;
    }

    m_current_page++;

    // Si après le premier lot l'écran n'est pas encore rempli, charger la suite automatiquement
    auto vadj = m_scrolled_window.get_vadjustment();
    if (vadj->get_upper() <= vadj->get_page_size() && m_has_more_apps && m_loaded_apps_count < 10) {
        load_discover_apps_async();
    }
}

void DiscoverPage::on_retry_clicked() {
    load_flathub_trending_async();
    if (m_loaded_apps_count == 0) {
        m_current_page = 1;
        m_has_more_apps = true;
        load_discover_apps_async();
    }
}

void DiscoverPage::on_trending_loaded() {
    if (!m_has_connection && m_loaded_apps_count == 0) {
        set_visible_child("offline");
        return;
    }

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

Gtk::Button* DiscoverPage::create_category_card(const std::string& fr_name, const std::string& en_name, const std::string& icon_name, const std::string& css_class) {
    auto btn = Gtk::make_managed<Gtk::Button>();
    btn->add_css_class("cat-card");
    btn->add_css_class(css_class);
    btn->set_cursor("pointer");

    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 12);
    box->set_halign(Gtk::Align::CENTER);
    box->set_valign(Gtk::Align::CENTER);

    auto icon = Gtk::make_managed<Gtk::Image>();
    icon->set_from_icon_name(icon_name);
    icon->set_pixel_size(24);
    box->append(*icon);

    const char* const* langs = g_get_language_names();
    bool is_french = (langs && langs[0] && std::string(langs[0]).rfind("fr", 0) == 0);
    std::string title = is_french ? fr_name : en_name;

    auto label = Gtk::make_managed<Gtk::Label>(title);
    label->add_css_class("cat-label");
    box->append(*label);

    btn->set_child(*box);
    return btn;
}

Gtk::Button* DiscoverPage::create_app_card(const core::DiscoverApp& app) {
    auto btn = Gtk::make_managed<Gtk::Button>();
    btn->add_css_class("app-card");
    btn->set_cursor("pointer");

    auto root_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 10);

    // 1. Icône de l'application
    auto icon_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    icon_box->add_css_class("app-card-icon-box");
    icon_box->set_valign(Gtk::Align::CENTER);

    auto img = Gtk::make_managed<Gtk::Image>();
    img->set_pixel_size(40);

    if (!app.local_icon_path.empty() && std::filesystem::exists(app.local_icon_path)) {
        img->set(app.local_icon_path);
    } else {
        auto display = Gdk::Display::get_default();
        bool has_icon = false;
        if (display) {
            auto theme = Gtk::IconTheme::get_for_display(display);
            has_icon = theme && theme->has_icon(app.icon);
        }

        if (has_icon) {
            img->set_from_icon_name(app.icon);
        } else {
            img->set_from_icon_name("application-x-executable");
        }
    }

    icon_box->append(*img);
    root_box->append(*icon_box);

    // 2. Nom et description
    auto text_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
    text_box->set_valign(Gtk::Align::CENTER);
    text_box->set_hexpand(true);

    auto name_label = Gtk::make_managed<Gtk::Label>(app.name);
    name_label->add_css_class("app-card-name");
    name_label->set_halign(Gtk::Align::START);
    name_label->set_ellipsize(Pango::EllipsizeMode::END);

    auto summary_label = Gtk::make_managed<Gtk::Label>(app.summary);
    summary_label->add_css_class("app-card-summary");
    summary_label->set_halign(Gtk::Align::START);
    summary_label->set_ellipsize(Pango::EllipsizeMode::END);

    text_box->append(*name_label);
    text_box->append(*summary_label);
    root_box->append(*text_box);

    btn->set_child(*root_box);

    btn->signal_clicked().connect([this, app]() {
        open_app_details(app);
    });
    return btn;
}

Gtk::Box* DiscoverPage::create_slide(const std::string& name, const std::string& tagline, const std::string& icon_name, const std::string& local_icon_path, const std::string& color_class) {
    auto slide = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 24);
    slide->add_css_class("hero-slide");
    slide->add_css_class(color_class);
    slide->set_hexpand(true);
    slide->set_halign(Gtk::Align::FILL);

    // Icône sans halo restrictif
    auto icon_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    icon_box->add_css_class("hero-slide-icon-box");
    icon_box->set_valign(Gtk::Align::CENTER);
    icon_box->set_halign(Gtk::Align::CENTER);

    if (!local_icon_path.empty() && std::filesystem::exists(local_icon_path)) {
        auto icon_img = Gtk::make_managed<Gtk::Image>();
        icon_img->set(local_icon_path);
        icon_img->set_pixel_size(72);
        icon_img->set_expand(true);
        icon_box->append(*icon_img);
    } else {
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

    // Titre + Description
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

void DiscoverPage::open_app_details(const core::DiscoverApp& app) {
    if (!m_details_modal) {
        auto root = dynamic_cast<Gtk::Window*>(get_root());
        if (root) {
            m_details_modal = std::make_unique<AppDetailsModal>(*root);
        }
    }
    if (m_details_modal) {
        m_details_modal->show_app(app);
    }
}

} // namespace bluehub::ui