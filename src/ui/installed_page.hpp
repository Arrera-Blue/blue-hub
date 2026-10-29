#pragma once

#include <gtkmm.h>

namespace bluehub::ui {

enum class AppSourceType {
    FLATPAK,
    RPM,
    DNF,
    ARCHIVE,
    APPIMAGE
};

class InstalledPage : public Gtk::ScrolledWindow {
public:
    InstalledPage();

private:
    Gtk::Box m_content_box{Gtk::Orientation::VERTICAL, 14};

    Gtk::Box* create_installed_row(const std::string& name, AppSourceType type);
};

} // namespace bluehub::ui