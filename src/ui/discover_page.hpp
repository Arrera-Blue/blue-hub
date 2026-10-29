#pragma once

#include <gtkmm.h>
#include <adwaita.h>
#include <string>
#include <vector>
#include <thread>
#include <memory>
#include "core/flathub_service.hpp"
#include "core/abpm_service.hpp"
#include "app_details_modal.hpp"

namespace bluehub::ui {

class DiscoverPage : public Gtk::Stack {
public:
    DiscoverPage();
    ~DiscoverPage() override;

private:
    void on_prev_clicked();
    void on_next_clicked();
    void load_flathub_trending_async();
    void on_trending_loaded();
    void on_retry_clicked();

    void load_discover_apps_async();
    void on_discover_apps_loaded();

    // 1. Vue principale avec défilement
    Gtk::ScrolledWindow m_scrolled_window;
    Gtk::Box m_content_box{Gtk::Orientation::VERTICAL, 24};

    // Carrousel avec flèches latérales & indicateur à points
    Gtk::Box m_carousel_container{Gtk::Orientation::VERTICAL, 8};
    Gtk::Box m_carousel_nav_row{Gtk::Orientation::HORIZONTAL, 12};
    Gtk::Button m_btn_prev;
    Gtk::Button m_btn_next;
    GtkWidget* m_carousel_widget{nullptr};
    GtkWidget* m_dots_widget{nullptr};

    // Grille des 6 catégories (2 lignes x 3 colonnes)
    Gtk::Grid m_categories_grid;

    // Titre de section et grille d'applications (5 colonnes)
    Gtk::Label m_apps_section_title;
    Gtk::Grid m_apps_grid;
    Gtk::Button m_btn_load_more;

    Gtk::Box* create_slide(const std::string& name, const std::string& tagline, const std::string& icon_name, const std::string& local_icon_path, const std::string& color_class);
    Gtk::Button* create_category_card(const std::string& fr_name, const std::string& en_name, const std::string& icon_name, const std::string& css_class);
    Gtk::Button* create_app_card(const core::DiscoverApp& app);

    // 2. Vue Hors-Ligne (AdwStatusPage)
    GtkWidget* m_status_page{nullptr};
    Gtk::Button m_btn_retry{"Réessayer"};

    // Gestion asynchrone Flathub Carrousel
    Glib::Dispatcher m_dispatcher;
    std::thread m_fetch_thread;
    std::vector<core::TrendingApp> m_fetched_apps;
    bool m_has_connection{false};

    // Gestion asynchrone ABPM Discover (6 en 6 au défilement)
    Glib::Dispatcher m_apps_dispatcher;
    std::thread m_apps_thread;
    std::vector<core::DiscoverApp> m_incoming_apps;
    int m_current_page{1};
    int m_loaded_apps_count{0};
    bool m_is_loading_apps{false};
    bool m_has_more_apps{true};
    void open_app_details(const core::DiscoverApp& app);
    std::unique_ptr<AppDetailsModal> m_details_modal;
};

} // namespace bluehub::ui