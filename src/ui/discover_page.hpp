#pragma once

#include <gtkmm.h>

namespace bluehub::ui {

class DiscoverPage : public Gtk::ScrolledWindow {
public:
    DiscoverPage();

private:
    Gtk::Box m_content_box{Gtk::Orientation::VERTICAL, 24};

    // 1. Grande bannière supérieure
    Gtk::Box m_hero_banner;

    // 2. Grille des 6 catégories (2 lignes x 3 colonnes)
    Gtk::Grid m_categories_grid;
    Gtk::Button m_btn_create{"Create"};
    Gtk::Button m_btn_work{"WORK"};
    Gtk::Button m_btn_play{"PLAY"};
    Gtk::Button m_btn_socialise{"SOCIALISE"};
    Gtk::Button m_btn_learn{"LEARN"};
    Gtk::Button m_btn_develop{"DEVELOP"};

    // 3. Rangée des cartes d'applications
    Gtk::Box m_apps_row{Gtk::Orientation::HORIZONTAL, 16};

    // Retourne un pointeur géré par GTK
    Gtk::Box* create_app_card(const std::string& name);
};

} // namespace bluehub::ui