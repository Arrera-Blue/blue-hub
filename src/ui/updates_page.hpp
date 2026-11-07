#pragma once

#include <gtkmm.h>
#include <vector>
#include <mutex>
#include <thread>
#include "core/abpm_service.hpp"

namespace bluehub::ui {

struct UpdateProgress {
    std::string title;
    int percentage{0};
    std::string status_message;
    bool is_finished{false};
    bool is_error{false};
    std::string error_message;
};

class UpdatesPage : public Gtk::ScrolledWindow {
public:
    UpdatesPage();
    ~UpdatesPage() override;

    void reload_updates();
    void start_single_update(const core::UpdateItem& item);

private:
    void render_updates();
    void start_update(const std::vector<core::UpdateItem>& items, const std::string& title);
    void on_update_progress_updated();
    void send_update_notification(const std::string& text);

    Gtk::Box* create_update_row(const core::UpdateItem& item);
    Gtk::Separator* create_separator();

    Gtk::Box m_content_box{Gtk::Orientation::VERTICAL, 16};

    // Bannière de progression temps réel (en haut)
    Gtk::Box m_progress_card{Gtk::Orientation::HORIZONTAL, 16};
    Gtk::Image m_progress_icon;
    Gtk::Box m_progress_info_box{Gtk::Orientation::VERTICAL, 6};
    Gtk::Box m_progress_header_row{Gtk::Orientation::HORIZONTAL, 12};
    Gtk::Label m_progress_title_label;
    Gtk::Label m_progress_pct_label;
    Gtk::Label m_progress_status_label;
    Gtk::ProgressBar m_progress_bar;

    // En-tête global
    Gtk::Box m_header_box{Gtk::Orientation::HORIZONTAL, 16};
    Gtk::Label m_status_label{"Recherche des mises à jour..."};
    Gtk::Button m_btn_refresh;
    Gtk::Button m_btn_update_all{"Tout mettre à jour"};

    // Section 1 : Applications
    Gtk::Box m_apps_section_box{Gtk::Orientation::VERTICAL, 10};
    Gtk::Label m_apps_section_title{"Applications à mettre à jour :"};
    Gtk::Box m_apps_list_box{Gtk::Orientation::VERTICAL, 8};
    Gtk::Button m_btn_update_apps{"Mettre à jour les applications"};

    // Section 2 : Utilitaires
    Gtk::Box m_utils_section_box{Gtk::Orientation::VERTICAL, 10};
    Gtk::Label m_utils_section_title{"Utilitaires à mettre à jour :"};
    Gtk::Box m_utils_list_box{Gtk::Orientation::VERTICAL, 8};
    Gtk::Button m_btn_update_utils{"Mettre à jour les utilitaires"};

    // Section 3 : Composants Système
    Gtk::Box m_system_section_box{Gtk::Orientation::VERTICAL, 10};
    Gtk::Label m_system_section_title{"Composants système à mettre à jour :"};
    Gtk::Box m_system_list_box{Gtk::Orientation::VERTICAL, 8};
    Gtk::Button m_btn_update_system{"Mettre à jour les composants système"};

    // Gestion asynchrone des données
    std::vector<core::UpdateItem> m_updates;
    std::mutex m_mutex;
    Glib::Dispatcher m_dispatcher;
    std::thread m_fetch_thread;

    // Gestion asynchrone de la progression d'exécution
    UpdateProgress m_current_progress;
    std::mutex m_progress_mutex;
    Glib::Dispatcher m_progress_dispatcher;
    std::thread m_update_thread;
    bool m_is_updating{false};
};

} // namespace bluehub::ui