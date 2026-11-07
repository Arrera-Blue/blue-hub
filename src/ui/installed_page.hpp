#pragma once

#include <gtkmm.h>
#include <vector>
#include <mutex>
#include <memory>
#include <thread>
#include "core/abpm_service.hpp"
#include "flatpak_permissions_modal.hpp"
#include "app_details_modal.hpp"

namespace bluehub::ui {

struct InstallProgress {
    std::string app_name;
    int percentage{0};
    std::string status_message;
    bool is_finished{false};
    bool is_error{false};
    std::string error_message;
};

class InstalledPage : public Gtk::ScrolledWindow {
public:
    InstalledPage();
    ~InstalledPage() override;

    void reload_apps();
    void start_installation(const core::DiscoverApp& app);
    sigc::signal<void(const core::UpdateItem&)>& signal_update_app() { return m_signal_update_app; }

private:
    void open_app_details(const core::InstalledApp& app);
    void on_modify_permissions(const core::InstalledApp& app);
    void on_change_icon(const core::InstalledApp& app);
    void on_uninstall_requested(const core::InstalledApp& app);
    void render_app_list();
    void on_install_progress_updated();
    void send_install_notification(const std::string& app_name);

    Gtk::Box* create_installed_row(const core::InstalledApp& app);

    Gtk::Box m_root_box{Gtk::Orientation::VERTICAL, 16};

    // 0. Bannière d'installation active en cours
    Gtk::Box m_install_card{Gtk::Orientation::HORIZONTAL, 16};
    Gtk::Image m_install_icon;
    Gtk::Box m_install_info_box{Gtk::Orientation::VERTICAL, 6};
    Gtk::Box m_install_header_row{Gtk::Orientation::HORIZONTAL, 12};
    Gtk::Label m_install_title_label;
    Gtk::Label m_install_pct_label;
    Gtk::Label m_install_status_label;
    Gtk::ProgressBar m_install_progress_bar;

    // Gestion asynchrone de l'installation
    InstallProgress m_current_install;
    std::mutex m_install_mutex;
    Glib::Dispatcher m_install_dispatcher;
    std::thread m_install_thread;
    bool m_is_installing{false};

    Gtk::Box m_user_header_box{Gtk::Orientation::HORIZONTAL};
    Gtk::Label m_user_count_label;
    Gtk::Box m_user_list_box{Gtk::Orientation::VERTICAL, 10};

    Gtk::Box m_system_header_box{Gtk::Orientation::HORIZONTAL};
    Gtk::Label m_system_count_label;
    Gtk::Box m_system_list_box{Gtk::Orientation::VERTICAL, 10};

    std::vector<core::InstalledApp> m_apps;
    std::mutex m_mutex;
    Glib::Dispatcher m_dispatcher;

    std::unique_ptr<FlatpakPermissionsModal> m_permissions_modal;
    std::unique_ptr<AppDetailsModal> m_details_modal;
    sigc::signal<void(const core::UpdateItem&)> m_signal_update_app;
};

} // namespace bluehub::ui