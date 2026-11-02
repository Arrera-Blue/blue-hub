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

    void show_app(const core::DiscoverApp& app, bool is_installed = false, bool is_system = false);
    void show_app(const core::InstalledApp& app);

    sigc::signal<void()>& signal_app_uninstalled() { return m_signal_app_uninstalled; }
    sigc::signal<void(const core::DiscoverApp&)>& signal_install_requested() { return m_signal_install_requested; }

private:
    void on_install_clicked();
    void on_details_ready();
    void on_prev_screenshot();
    void on_next_screenshot();
    void update_carousel();
    void update_action_button();

    // Conteneur principal
    Gtk::Box m_root_box{Gtk::Orientation::VERTICAL, 14};
    Gtk::ScrolledWindow m_scrolled_window;
    Gtk::Box m_content_box{Gtk::Orientation::VERTICAL, 16};

    // 1. Bouton de fermeture [✕] fixé en haut à gauche
    Gtk::Box m_top_bar{Gtk::Orientation::HORIZONTAL};
    Gtk::Button m_btn_close;

    // 2. En-tête : Icône, Titre, Développeur, Badge Certifié et Bouton d'action
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

    // État d'installation et de désinstallation
    bool m_is_installed{false};
    bool m_is_system{false};
    bool m_status_checked{false};
    bool m_fetched_is_installed{false};
    bool m_fetched_is_system{false};
    std::string m_fetched_source;
    std::string m_fetched_package;

    sigc::signal<void()> m_signal_app_uninstalled;
    sigc::signal<void(const core::DiscoverApp&)> m_signal_install_requested;
    Glib::Dispatcher m_uninstall_dispatcher;
};

} // namespace bluehub::ui