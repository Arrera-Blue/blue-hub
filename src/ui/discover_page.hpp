#pragma once

#include <gtkmm.h>
#include <adwaita.h>
#include <string>
#include <vector>
#include <thread>
#include "core/flathub_service.hpp"

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
    Gtk::Button m_btn_create{"Create"};
    Gtk::Button m_btn_work{"WORK"};
    Gtk::Button m_btn_play{"PLAY"};
    Gtk::Button m_btn_socialise{"SOCIALISE"};
    Gtk::Button m_btn_learn{"LEARN"};
    Gtk::Button m_btn_develop{"DEVELOP"};

    // Rangée des cartes d'applications
    Gtk::Box m_apps_row{Gtk::Orientation::HORIZONTAL, 16};

    Gtk::Box* create_slide(const std::string& name, const std::string& tagline, const std::string& icon_name, const std::string& local_icon_path, const std::string& color_class);
    Gtk::Box* create_app_card(const std::string& name);

    // 2. Vue Hors-Ligne (AdwStatusPage)
    GtkWidget* m_status_page{nullptr};
    Gtk::Button m_btn_retry{"Réessayer"};

    // Gestion asynchrone
    Glib::Dispatcher m_dispatcher;
    std::thread m_fetch_thread;
    std::vector<core::TrendingApp> m_fetched_apps;
    bool m_has_connection{false};
};

} // namespace bluehub::ui
