#pragma once

#include <gtkmm.h>

namespace bluehub::ui {

class UpdatesPage : public Gtk::ScrolledWindow {
public:
    UpdatesPage();

private:
    Gtk::Box m_content_box{Gtk::Orientation::VERTICAL, 14};

    // En-tête global
    Gtk::Box m_header_box{Gtk::Orientation::HORIZONTAL, 16};
    Gtk::Label m_status_label{"Des mise a jour sont disponible"};
    Gtk::Button m_btn_update_all{"Mettre a jour"};

    // Helper pour générer les lignes
    Gtk::Box* create_update_row(const std::string& name, bool with_icon);
    Gtk::Separator* create_separator();
};

} // namespace bluehub::ui