#pragma once

#include <gtkmm.h>
#include <adwaita.h>
#include <string>
#include <vector>
#include <mutex>
#include "core/abpm_service.hpp"

namespace bluehub::ui {

class AppDetailsModal : public Gtk::Window {
public:
    explicit AppDetailsModal(Gtk::Window& parent);

    void show_app(const core::DiscoverApp& app);

private:
    void on_install_clicked();
    void on_details_ready();
    void on_prev_screenshot();
    void on_next_screenshot();
    void update_carousel();

    // Conteneur principal
    Gtk::Box m_root_box{Gtk::Orientation::VERTICAL, 14};
    Gtk::ScrolledWindow m_scrolled_window;
    Gtk::Box m_content_box{Gtk::Orientation::VERTICAL, 16};

    // 1. Bouton de fermeture [✕]
    Gtk::Box m_top_bar{Gtk::Orientation::HORIZONTAL};
    Gtk::Button m_btn_close;

    // 2. En-tête : Icône, Titre, Développeur, Badge Certifié et Bouton d'installation
    Gtk::Box m_header_box{Gtk::Orientation::HORIZONTAL, 18};
    Gtk::Image m_app_icon;
    Gtk::Box m_title_dev_box{Gtk::Orientation::VERTICAL, 4};
    Gtk::Label m_name_label;
    Gtk::Box m_dev_badge_box{Gtk::Orientation::HORIZONTAL, 8};
    Gtk::Label m_dev_label;
    Gtk::Label m_verified_badge{"Vérifié"};
    Gtk::Button m_btn_install{"Installer"};

    // 3. Carrousel Adwaita pour les captures d'écran
    Gtk::Box m_carousel_container{Gtk::Orientation::VERTICAL, 8};
    Gtk::Box m_carousel_nav_row{Gtk::Orientation::HORIZONTAL, 10};
    Gtk::Button m_btn_prev_screen;
    Gtk::Button m_btn_next_screen;
    GtkWidget* m_carousel_widget{nullptr};
    GtkWidget* m_dots_widget{nullptr};

    // 4. Description détaillée
    Gtk::Label m_description_label;

    core::DiscoverApp m_current_app;
    core::AppDetails m_fetched_details;
    std::mutex m_details_mutex;
    Glib::Dispatcher m_dispatcher;
};

} // namespace bluehub::ui
