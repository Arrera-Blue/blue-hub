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
    set_decorated(false);

    add_css_class("app-details-window");
    m_root_box.add_css_class("app-details-modal");

    // 1. Bouton fermer [✕] fixé en haut à gauche
    m_top_bar.set_margin_start(24);
    m_top_bar.set_margin_end(24);
    m_top_bar.set_margin_top(18);
    m_btn_close.set_halign(Gtk::Align::START);
    m_btn_close.set_icon_name("window-close-symbolic");
    m_btn_close.add_css_class("modal-close-btn");
    m_btn_close.set_cursor("pointer");
    m_btn_close.signal_clicked().connect([this]() { hide(); });
    m_top_bar.append(m_btn_close);
    m_root_box.append(m_top_bar);

    // 2. En-tête : Icône 80px, Textes et Bouton d'action
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

    m_btn_update.add_css_class("modal-update-btn");
    m_btn_update.set_valign(Gtk::Align::CENTER);
    m_btn_update.set_cursor("pointer");
    m_btn_update.set_visible(false);
    m_btn_update.signal_clicked().connect(sigc::mem_fun(*this, &AppDetailsModal::on_update_clicked));

    m_actions_box.set_valign(Gtk::Align::CENTER);
    m_actions_box.append(m_btn_install);
    m_actions_box.append(m_btn_update);
    m_header_box.append(m_actions_box);

    m_root_box.append(m_header_box);

    // 3. Conteneur défilable
    m_content_box.set_margin_start(24);
    m_content_box.set_margin_end(32);
    m_content_box.set_margin_bottom(24);

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

    m_dispatcher.connect(sigc::mem_fun(*this, &AppDetailsModal::on_details_ready));
    m_uninstall_dispatcher.connect([this]() {
        m_signal_app_uninstalled.emit();
        hide();
    });
}

void AppDetailsModal::update_action_button() {
    m_btn_install.remove_css_class("modal-uninstall-btn");

    if (m_is_installed) {
        if (m_has_update) {
            m_btn_update.set_visible(true);
            m_btn_update.set_sensitive(true);
            m_btn_update.set_label("Mettre à jour");
            if (!m_update_item.new_version.empty()) {
                m_btn_update.set_tooltip_text("Mettre à jour vers " + m_update_item.new_version);
            } else {
                m_btn_update.set_tooltip_text("Mettre à jour " + m_current_app.name);
            }
        } else {
            m_btn_update.set_visible(false);
        }

        if (m_is_system) {
            if (m_has_update) {
                m_btn_install.set_visible(false);
            } else {
                m_btn_install.set_visible(true);
                m_btn_install.set_label("Système");
                m_btn_install.set_sensitive(false);
                m_btn_install.set_tooltip_text("Application système protégée non désinstallable");
            }
        } else {
            m_btn_install.set_visible(true);
            m_btn_install.set_label("Désinstaller");
            m_btn_install.set_sensitive(true);
            m_btn_install.add_css_class("modal-uninstall-btn");
            m_btn_install.set_tooltip_text("Désinstaller " + m_current_app.name);
        }
    } else {
        m_btn_update.set_visible(false);
        m_btn_install.set_visible(true);
        m_btn_install.set_label("Installer");
        m_btn_install.set_sensitive(true);
        m_btn_install.set_tooltip_text("Installer " + m_current_app.name);
    }
}

void AppDetailsModal::show_app(const core::DiscoverApp& app, bool is_installed, bool is_system) {
    m_current_app = app;
    m_is_installed = is_installed;
    m_is_system = is_system;
    m_status_checked = is_installed;

    std::string detected_src = app.source;
    std::string detected_pkg = app.package;

    m_has_update = false;
    m_update_item = {};
    core::UpdateItem fast_up;
    if (core::AbpmService::is_update_available(app.id, app.package, fast_up)) {
        m_has_update = true;
        m_update_item = fast_up;
    }

    // Vérification immédiate via le cache (0ms)
    if (!m_status_checked) {
        bool sys = false;
        if (core::AbpmService::is_app_installed(app.id, app.package, sys, detected_src, detected_pkg)) {
            m_is_installed = true;
            m_is_system = sys;
            m_status_checked = true;
            m_current_app.source = detected_src;
            m_current_app.package = detected_pkg;
        }
    }

    m_fetched_is_installed = m_is_installed;
    m_fetched_is_system = m_is_system;
    m_fetched_source = detected_src;
    m_fetched_package = detected_pkg;
    m_fetched_has_update = m_has_update;
    m_fetched_update_item = m_update_item;

    m_name_label.set_label(app.name);
    std::string dev = extract_developer_name(app.id);
    if (is_valid_developer(dev)) {
        m_dev_label.set_label("par " + dev);
        m_dev_label.set_visible(true);
    } else {
        m_dev_label.set_visible(false);
    }
    m_verified_badge.set_visible(false);
    m_description_label.set_label(!app.summary.empty() ? app.summary : "Chargement des détails...");

    // Vider le carrousel en attente du rechargement
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
    std::string icon_path = app.local_icon_path;
    if (icon_path.empty() || !std::filesystem::exists(icon_path)) {
        icon_path = core::AbpmService::find_local_app_icon(app.icon, app.id, app.package);
    }

    if (!icon_path.empty() && std::filesystem::exists(icon_path)) {
        m_app_icon.set(icon_path);
    } else {
        auto display = Gdk::Display::get_default();
        std::string clean_icon = app.icon;
        if (clean_icon.size() > 4 && clean_icon.rfind(".png") == clean_icon.size() - 4) {
            clean_icon = clean_icon.substr(0, clean_icon.size() - 4);
        }
        bool has_icon = false;
        if (display) {
            auto theme = Gtk::IconTheme::get_for_display(display);
            if (theme) {
                if (theme->has_icon(clean_icon)) {
                    m_app_icon.set_from_icon_name(clean_icon);
                    has_icon = true;
                } else if (theme->has_icon(app.id)) {
                    m_app_icon.set_from_icon_name(app.id);
                    has_icon = true;
                }
            }
        }

        if (!has_icon) {
            m_app_icon.set_from_icon_name("application-x-executable");
        }
    }

    // Affiche le bouton UNIQUEMENT si le statut est confirmé, sinon masqué
    if (m_status_checked) {
        update_action_button();
        m_btn_install.set_visible(true);
    } else {
        m_btn_install.set_visible(false);
    }

    present();

    // Détection en arrière-plan et métadonnées
    std::thread([this, app, is_installed, is_system, detected_src, detected_pkg]() {
        bool installed = is_installed;
        bool sys = is_system;
        std::string src = detected_src;
        std::string pkg = detected_pkg;

        if (!installed) {
            installed = core::AbpmService::is_app_installed(app.id, app.package, sys, src, pkg);
        }

        core::UpdateItem async_up;
        bool has_up = core::AbpmService::is_update_available(app.id, pkg, async_up);

        auto details = core::AbpmService::get_app_details(app);
        {
            std::lock_guard<std::mutex> lock(m_details_mutex);
            m_fetched_details = std::move(details);
            m_fetched_is_installed = installed;
            m_fetched_is_system = sys;
            m_fetched_source = src;
            m_fetched_package = pkg;
            m_fetched_has_update = has_up;
            m_fetched_update_item = async_up;
        }
        m_dispatcher.emit();
    }).detach();
}

void AppDetailsModal::show_app(const core::InstalledApp& app) {
    core::DiscoverApp d_app;
    d_app.id = app.id;
    d_app.name = app.name;
    d_app.summary = "";
    d_app.icon = app.icon;
    d_app.source = app.source;
    d_app.package = app.package;
    d_app.local_icon_path = app.local_icon_path;
    show_app(d_app, true, app.is_system);
}

void AppDetailsModal::on_details_ready() {
    core::AppDetails details;
    {
        std::lock_guard<std::mutex> lock(m_details_mutex);
        if (m_fetched_details.id != m_current_app.id) {
            return;
        }
        details = m_fetched_details;
        m_is_installed = m_fetched_is_installed;
        m_is_system = m_fetched_is_system;
        m_has_update = m_fetched_has_update;
        m_update_item = m_fetched_update_item;
        if (m_is_installed) {
            if (!m_fetched_source.empty()) m_current_app.source = m_fetched_source;
            if (!m_fetched_package.empty()) m_current_app.package = m_fetched_package;
        }
    }

    if (is_valid_developer(details.developer)) {
        m_dev_label.set_label("par " + details.developer);
        m_dev_label.set_visible(true);
    } else {
        m_dev_label.set_visible(false);
    }

    m_verified_badge.set_visible(details.is_verified);

    if (!details.description.empty()) {
        m_description_label.set_label(details.description);
    } else if (m_description_label.get_text() == "Chargement des détails...") {
        m_description_label.set_label("Aucune description détaillée disponible pour cette application.");
    }

    update_carousel();
    update_action_button();
    m_btn_install.set_visible(true);
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
    if (m_is_installed) {
        if (m_is_system) return;

        auto dialog = Gtk::AlertDialog::create("Désinstaller " + m_current_app.name + " ?");
        dialog->set_detail("Cette action supprimera définitivement l'application via abpm.");
        dialog->set_buttons({"Annuler", "Désinstaller"});
        dialog->set_cancel_button(0);
        dialog->set_default_button(1);

        dialog->choose([this, dialog](const Glib::RefPtr<Gio::AsyncResult>& res) {
            try {
                int btn = dialog->choose_finish(res);
                if (btn == 1) {
                    m_btn_install.set_sensitive(false);
                    m_btn_install.set_label("Désinstallation...");
                    std::thread([this]() {
                        core::AbpmService::uninstall_app(m_current_app.package, m_current_app.source);
                        m_uninstall_dispatcher.emit();
                    }).detach();
                }
            } catch (...) {}
        });
    } else {
        m_signal_install_requested.emit(m_current_app);
        hide();
    }
}

void AppDetailsModal::on_update_clicked() {
    if (m_has_update) {
        m_signal_update_requested.emit(m_update_item);
        hide();
    }
}

} // namespace bluehub::ui