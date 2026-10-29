#include "app_details_modal.hpp"
#include <filesystem>
#include <algorithm>
#include <thread>
#include <cmath>

namespace bluehub::ui {

namespace {
    bool is_valid_developer(const std::string& dev) {
        if (dev.empty()) return false;
        std::string lower = dev;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower != "github" && lower != "gitlab" && lower != "codeberg" && lower != "éditeur officiel";
    }

    std::string extract_developer_name(const std::string& app_id) {
        if (app_id.find("gnome") != std::string::npos) return "The GNOME Project";
        if (app_id.find("mozilla") != std::string::npos) return "Mozilla Corporation";
        if (app_id.find("videolan") != std::string::npos) return "VideoLAN";
        if (app_id.find("armatik") != std::string::npos) return "Armatik";
        if (app_id.find("gimp") != std::string::npos) return "The GIMP Team";
        if (app_id.find("blender") != std::string::npos) return "Blender Foundation";
        if (app_id.find("microsoft") != std::string::npos || app_id.find("vscode") != std::string::npos) return "Microsoft";
        
        auto first_dot = app_id.find('.');
        if (first_dot != std::string::npos) {
            auto second_dot = app_id.find('.', first_dot + 1);
            if (second_dot != std::string::npos) {
                std::string dev = app_id.substr(first_dot + 1, second_dot - first_dot - 1);
                // Si c'est io.github.auteur.app, on extrait le nom de l'auteur
                if (dev == "github" || dev == "gitlab" || dev == "codeberg") {
                    auto third_dot = app_id.find('.', second_dot + 1);
                    if (third_dot != std::string::npos) {
                        std::string author = app_id.substr(second_dot + 1, third_dot - second_dot - 1);
                        if (!author.empty()) {
                            author[0] = static_cast<char>(std::toupper(author[0]));
                            return author;
                        }
                    }
                    return "";
                }
                if (!dev.empty()) {
                    dev[0] = static_cast<char>(std::toupper(dev[0]));
                    return dev;
                }
            }
        }
        return "";
    }
}

AppDetailsModal::AppDetailsModal(Gtk::Window& parent) {
    set_transient_for(parent);
    set_modal(true);
    set_default_size(800, 640);
    set_resizable(false);
    set_decorated(false); // Fenêtre modale sans bordure système

    // Classe CSS sur la fenêtre pour transparence intégrale sans halo
    add_css_class("app-details-window");

    m_root_box.add_css_class("app-details-modal");

    // 1. Bouton fermer [✕] fixé en haut
    m_top_bar.set_margin_start(24);
    m_top_bar.set_margin_end(24);
    m_top_bar.set_margin_top(18);
    m_btn_close.set_icon_name("window-close-symbolic");
    m_btn_close.add_css_class("modal-close-btn");
    m_btn_close.set_cursor("pointer");
    m_btn_close.signal_clicked().connect([this]() { hide(); });
    m_top_bar.append(m_btn_close);
    m_root_box.append(m_top_bar);

    // 2. En-tête : Icône 80px, Textes et Bouton bleu Installer (FIXÉ en haut, ne défile pas)
    m_header_box.set_margin_start(24);
    m_header_box.set_margin_end(24);
    m_header_box.set_margin_bottom(12);

    m_app_icon.set_pixel_size(80);
    m_app_icon.add_css_class("modal-app-icon");
    m_header_box.append(m_app_icon);

    m_title_dev_box.set_valign(Gtk::Align::CENTER);
    m_title_dev_box.set_hexpand(true);

    m_name_label.add_css_class("modal-app-name");
    m_name_label.set_halign(Gtk::Align::START);

    m_dev_label.add_css_class("modal-app-dev");
    m_dev_label.set_halign(Gtk::Align::START);

    // Ligne Développeur + Badge vérifié Flathub
    m_dev_badge_box.set_orientation(Gtk::Orientation::HORIZONTAL);
    m_dev_badge_box.set_spacing(8);
    m_dev_badge_box.set_valign(Gtk::Align::CENTER);

    m_verified_badge.set_label("Vérifié");
    m_verified_badge.add_css_class("verified-badge");
    m_verified_badge.set_valign(Gtk::Align::CENTER);
    m_verified_badge.set_visible(false);

    m_dev_badge_box.append(m_dev_label);
    m_dev_badge_box.append(m_verified_badge);

    m_title_dev_box.append(m_name_label);
    m_title_dev_box.append(m_dev_badge_box);
    m_header_box.append(m_title_dev_box);

    m_btn_install.add_css_class("modal-install-btn");
    m_btn_install.set_valign(Gtk::Align::CENTER);
    m_btn_install.set_cursor("pointer");
    m_btn_install.signal_clicked().connect(sigc::mem_fun(*this, &AppDetailsModal::on_install_clicked));
    m_header_box.append(m_btn_install);

    m_root_box.append(m_header_box);

    // 3. Conteneur défilable : marge de 32px à droite pour isoler totalement la scrollbar
    m_content_box.set_margin_start(24);
    m_content_box.set_margin_end(32);
    m_content_box.set_margin_bottom(24);

    // Carrousel Adwaita pour les captures d'écran
    m_carousel_widget = adw_carousel_new();
    adw_carousel_set_spacing(ADW_CAROUSEL(m_carousel_widget), 14);
    adw_carousel_set_allow_mouse_drag(ADW_CAROUSEL(m_carousel_widget), TRUE);
    adw_carousel_set_allow_scroll_wheel(ADW_CAROUSEL(m_carousel_widget), TRUE);

    auto carousel_cpp = Glib::wrap(m_carousel_widget);
    carousel_cpp->set_hexpand(true);
    carousel_cpp->set_halign(Gtk::Align::FILL);

    m_btn_prev_screen.set_icon_name("go-previous-symbolic");
    m_btn_prev_screen.add_css_class("modal-carousel-btn");
    m_btn_prev_screen.set_cursor("pointer");
    m_btn_prev_screen.set_valign(Gtk::Align::CENTER);
    m_btn_prev_screen.signal_clicked().connect(sigc::mem_fun(*this, &AppDetailsModal::on_prev_screenshot));

    m_btn_next_screen.set_icon_name("go-next-symbolic");
    m_btn_next_screen.add_css_class("modal-carousel-btn");
    m_btn_next_screen.set_cursor("pointer");
    m_btn_next_screen.set_valign(Gtk::Align::CENTER);
    m_btn_next_screen.signal_clicked().connect(sigc::mem_fun(*this, &AppDetailsModal::on_next_screenshot));

    m_carousel_nav_row.add_css_class("modal-carousel-nav-row");
    m_carousel_nav_row.append(m_btn_prev_screen);
    m_carousel_nav_row.append(*carousel_cpp);
    m_carousel_nav_row.append(m_btn_next_screen);

    m_dots_widget = adw_carousel_indicator_dots_new();
    adw_carousel_indicator_dots_set_carousel(ADW_CAROUSEL_INDICATOR_DOTS(m_dots_widget), ADW_CAROUSEL(m_carousel_widget));
    auto dots_cpp = Glib::wrap(m_dots_widget);
    dots_cpp->set_halign(Gtk::Align::CENTER);

    m_carousel_container.add_css_class("modal-carousel-container");
    m_carousel_container.append(m_carousel_nav_row);
    m_carousel_container.append(*dots_cpp);
    m_content_box.append(m_carousel_container);

    // 4. Description détaillée
    m_description_label.add_css_class("modal-app-description");
    m_description_label.set_wrap(true);
    m_description_label.set_halign(Gtk::Align::START);
    m_content_box.append(m_description_label);

    m_scrolled_window.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    m_scrolled_window.set_vexpand(true);
    m_scrolled_window.set_child(m_content_box);

    m_root_box.append(m_scrolled_window);
    set_child(m_root_box);

    // Connexion du dispatcher thread-safe pour la mise à jour asynchrone
    m_dispatcher.connect(sigc::mem_fun(*this, &AppDetailsModal::on_details_ready));
}

void AppDetailsModal::show_app(const core::DiscoverApp& app) {
    m_current_app = app;

    // Textes de base
    m_name_label.set_label(app.name);
    std::string dev = extract_developer_name(app.id);
    if (is_valid_developer(dev)) {
        m_dev_label.set_label("par " + dev);
        m_dev_label.set_visible(true);
    } else {
        m_dev_label.set_visible(false);
    }
    m_verified_badge.set_visible(false);
    m_description_label.set_label(app.summary);

    // Vider le carrousel en attente du chargement
    auto carousel = ADW_CAROUSEL(m_carousel_widget);
    guint n_pages = adw_carousel_get_n_pages(carousel);
    for (guint i = 0; i < n_pages; ++i) {
        auto page = adw_carousel_get_nth_page(carousel, 0);
        if (page) {
            adw_carousel_remove(carousel, page);
        }
    }
    m_carousel_container.set_visible(false);

    // Chargement de l'icône
    if (!app.local_icon_path.empty() && std::filesystem::exists(app.local_icon_path)) {
        m_app_icon.set(app.local_icon_path);
    } else {
        auto display = Gdk::Display::get_default();
        bool has_icon = false;
        if (display) {
            auto theme = Gtk::IconTheme::get_for_display(display);
            has_icon = theme && theme->has_icon(app.icon);
        }

        if (has_icon) {
            m_app_icon.set_from_icon_name(app.icon);
        } else {
            m_app_icon.set_from_icon_name("application-x-executable");
        }
    }

    m_btn_install.set_sensitive(true);
    m_btn_install.set_label("Installer");

    // Afficher immédiatement la fenêtre modale
    present();

    // Lancer la récupération asynchrone des métadonnées et captures
    std::thread([this, app]() {
        auto details = core::AbpmService::get_app_details(app);
        {
            std::lock_guard<std::mutex> lock(m_details_mutex);
            m_fetched_details = std::move(details);
        }
        m_dispatcher.emit();
    }).detach();
}

void AppDetailsModal::on_details_ready() {
    core::AppDetails details;
    {
        std::lock_guard<std::mutex> lock(m_details_mutex);
        if (m_fetched_details.id != m_current_app.id) {
            return;
        }
        details = m_fetched_details;
    }

    if (is_valid_developer(details.developer)) {
        m_dev_label.set_label("par " + details.developer);
        m_dev_label.set_visible(true);
    } else {
        m_dev_label.set_visible(false);
    }

    // Badge certifié Flathub
    m_verified_badge.set_visible(details.is_verified);

    if (!details.description.empty()) {
        m_description_label.set_label(details.description);
    }

    update_carousel();
}

void AppDetailsModal::on_prev_screenshot() {
    auto carousel = ADW_CAROUSEL(m_carousel_widget);
    guint total = adw_carousel_get_n_pages(carousel);
    if (total > 1) {
        int current = static_cast<int>(std::round(adw_carousel_get_position(carousel)));
        int prev = (current - 1 + total) % total;
        auto target = adw_carousel_get_nth_page(carousel, prev);
        if (target) {
            adw_carousel_scroll_to(carousel, target, TRUE);
        }
    }
}

void AppDetailsModal::on_next_screenshot() {
    auto carousel = ADW_CAROUSEL(m_carousel_widget);
    guint total = adw_carousel_get_n_pages(carousel);
    if (total > 1) {
        int current = static_cast<int>(std::round(adw_carousel_get_position(carousel)));
        int next = (current + 1) % total;
        auto target = adw_carousel_get_nth_page(carousel, next);
        if (target) {
            adw_carousel_scroll_to(carousel, target, TRUE);
        }
    }
}

void AppDetailsModal::update_carousel() {
    auto carousel = ADW_CAROUSEL(m_carousel_widget);

    // Vider les anciennes diapositives
    guint n_pages = adw_carousel_get_n_pages(carousel);
    for (guint i = 0; i < n_pages; ++i) {
        auto page = adw_carousel_get_nth_page(carousel, 0);
        if (page) {
            adw_carousel_remove(carousel, page);
        }
    }

    const auto& paths = m_fetched_details.local_screenshot_paths;
    if (paths.empty()) {
        m_carousel_container.set_visible(false);
        return;
    }

    m_carousel_container.set_visible(true);

    for (const auto& path : paths) {
        auto slide = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
        slide->add_css_class("modal-screenshot-slide");
        slide->set_hexpand(true);
        slide->set_halign(Gtk::Align::FILL);

        auto pic = Gtk::make_managed<Gtk::Picture>();
        pic->set_filename(path);
        pic->set_can_shrink(true);
        pic->set_content_fit(Gtk::ContentFit::CONTAIN);
        pic->set_hexpand(true);
        pic->set_vexpand(true);

        slide->append(*pic);
        adw_carousel_append(carousel, GTK_WIDGET(slide->gobj()));
    }

    bool multiple = paths.size() > 1;
    m_btn_prev_screen.set_visible(multiple);
    m_btn_next_screen.set_visible(multiple);
    if (m_dots_widget) {
        Glib::wrap(m_dots_widget)->set_visible(multiple);
    }

    if (adw_carousel_get_n_pages(carousel) > 0) {
        auto first_page = adw_carousel_get_nth_page(carousel, 0);
        if (first_page) {
            adw_carousel_scroll_to(carousel, first_page, FALSE);
        }
    }
}

void AppDetailsModal::on_install_clicked() {
    m_btn_install.set_sensitive(false);
    m_btn_install.set_label("Installation...");
}

} // namespace bluehub::ui