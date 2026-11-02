#pragma once

#include <gtkmm.h>
#include <vector>
#include <mutex>
#include <memory>
#include "core/abpm_service.hpp"
#include "flatpak_permissions_modal.hpp"
#include "app_details_modal.hpp"

namespace bluehub::ui {

class InstalledPage : public Gtk::ScrolledWindow {
public:
    InstalledPage();

    void reload_apps();

private:
    void open_app_details(const core::InstalledApp& app);
    void on_modify_permissions(const core::InstalledApp& app);
    void on_change_icon(const core::InstalledApp& app);
    void on_uninstall_requested(const core::InstalledApp& app);
    void render_app_list();

    Gtk::Box* create_installed_row(const core::InstalledApp& app);

    Gtk::Box m_root_box{Gtk::Orientation::VERTICAL, 16};

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
};

} // namespace bluehub::ui