#include "installed_page.hpp"
#include <filesystem>
#include <thread>
#include <algorithm>

namespace bluehub::ui {

InstalledPage::InstalledPage() {
    set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);

    m_root_box.set_margin_start(36);
    m_root_box.set_margin_end(36);
    m_root_box.set_margin_top(24);
    m_root_box.set_margin_bottom(30);

    // 1. En-tête des applications utilisateur
    m_user_header_box.add_css_class("installed-header-row");
    m_user_count_label.add_css_class("installed-count-label");
    m_user_count_label.set_halign(Gtk::Align::START);
    m_user_count_label.set_hexpand(true);
    m_user_count_label.set_label("Chargement des applications...");
    m_user_header_box.append(m_user_count_label);

    m_root_box.append(m_user_header_box);
    m_root_box.append(m_user_list_box);

    // 2. En-tête de la catégorie Utilitaires système (en dessous)
    m_system_header_box.add_css_class("installed-section-separator");
    m_system_count_label.add_css_class("installed-count-label");
    m_system_count_label.set_halign(Gtk::Align::START);
    m_system_count_label.set_hexpand(true);
    m_system_count_label.set_label("Utilitaires système");
    m_system_header_box.append(m_system_count_label);

    m_root_box.append(m_system_header_box);
    m_root_box.append(m_system_list_box);

    set_child(m_root_box);

    m_dispatcher.connect([this]() { render_app_list(); });
    reload_apps();
}

void InstalledPage::reload_apps() {
    std::thread([this]() {
        auto apps = core::AbpmService::get_installed_apps(true);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_apps = std::move(apps);
        }
        m_dispatcher.emit();
    }).detach();
}

void InstalledPage::render_app_list() {
    // Vider les anciennes listes
    while (auto child = m_user_list_box.get_first_child()) {
        m_user_list_box.remove(*child);
    }
    while (auto child = m_system_list_box.get_first_child()) {
        m_system_list_box.remove(*child);
    }

    std::vector<core::InstalledApp> user_apps;
    std::vector<core::InstalledApp> system_apps;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& app : m_apps) {
            if (app.is_system) {
                system_apps.push_back(app);
            } else {
                user_apps.push_back(app);
            }
        }
    }

    // Tri alphabétique avec gestion propre des accents français (g_utf8_collate)
    auto alphabet_sort = [](const core::InstalledApp& a, const core::InstalledApp& b) {
        return g_utf8_collate(a.name.c_str(), b.name.c_str()) < 0;
    };
    std::sort(user_apps.begin(), user_apps.end(), alphabet_sort);
    std::sort(system_apps.begin(), system_apps.end(), alphabet_sort);

    m_user_count_label.set_label("Applications installées (" + std::to_string(user_apps.size()) + ")");
    for (const auto& app : user_apps) {
        m_user_list_box.append(*create_installed_row(app));
    }

    m_system_count_label.set_label("Utilitaires système (" + std::to_string(system_apps.size()) + ")");
    for (const auto& app : system_apps) {
        m_system_list_box.append(*create_installed_row(app));
    }
}

Gtk::Box* InstalledPage::create_installed_row(const core::InstalledApp& app) {
    auto row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 16);
    row->add_css_class("installed-row");

    // 1. Icône de l'application
    auto icon_img = Gtk::make_managed<Gtk::Image>();
    icon_img->set_pixel_size(44);
    icon_img->add_css_class("installed-app-icon");

    if (!app.local_icon_path.empty() && std::filesystem::exists(app.local_icon_path)) {
        icon_img->set(app.local_icon_path);
    } else {
        auto display = Gdk::Display::get_default();
        bool has_icon = false;
        if (display) {
            auto theme = Gtk::IconTheme::get_for_display(display);
            has_icon = theme && theme->has_icon(app.icon);
        }
        if (has_icon) {
            icon_img->set_from_icon_name(app.icon);
        } else {
            icon_img->set_from_icon_name("application-x-executable");
        }
    }

    // 2. Nom, badge éventuel et version
    auto text_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
    text_box->set_valign(Gtk::Align::CENTER);
    text_box->set_hexpand(true);

    auto title_label = Gtk::make_managed<Gtk::Label>(app.name);
    title_label->add_css_class("installed-app-title");
    title_label->set_halign(Gtk::Align::START);
    text_box->append(*title_label);

    auto sub_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);

    if (app.source == "flatpak") {
        auto badge = Gtk::make_managed<Gtk::Label>("Flatpak");
        badge->add_css_class("badge-source");
        badge->add_css_class("badge-flatpak");
        sub_box->append(*badge);
    } else if (app.source == "archive") {
        auto badge = Gtk::make_managed<Gtk::Label>("Archive");
        badge->add_css_class("badge-source");
        badge->add_css_class("badge-archive");
        sub_box->append(*badge);
    } else if (app.source == "appimage") {
        auto badge = Gtk::make_managed<Gtk::Label>("AppImage");
        badge->add_css_class("badge-source");
        badge->add_css_class("badge-appimage");
        sub_box->append(*badge);
    }

    if (!app.version.empty()) {
        auto ver_label = Gtk::make_managed<Gtk::Label>("v" + app.version);
        ver_label->add_css_class("installed-app-subtitle");
        sub_box->append(*ver_label);
    }
    text_box->append(*sub_box);

    // Conteneur cliquable : ouvrir la modale au clic sur l'application
    auto click_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 16);
    click_box->set_hexpand(true);
    click_box->set_cursor("pointer");
    click_box->append(*icon_img);
    click_box->append(*text_box);

    auto gesture = Gtk::GestureClick::create();
    gesture->signal_pressed().connect([this, app](int, double, double) {
        open_app_details(app);
    });
    click_box->add_controller(gesture);
    row->append(*click_box);

    // 3. Actions contextuelles rapides
    auto actions_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    actions_box->set_valign(Gtk::Align::CENTER);

    // Action A : Permissions Flatpak
    if (app.source == "flatpak") {
        auto perm_btn = Gtk::make_managed<Gtk::Button>();
        perm_btn->set_icon_name("preferences-system-symbolic");
        perm_btn->add_css_class("row-action-btn");
        perm_btn->add_css_class("perm-btn");
        perm_btn->set_cursor("pointer");
        perm_btn->set_tooltip_text("Modifier les autorisations Flatpak");
        perm_btn->signal_clicked().connect([this, app]() { on_modify_permissions(app); });
        actions_box->append(*perm_btn);
    }

    // Action B : Modifier l'icône (AppImage ou Archive)
    if (app.source == "appimage" || app.source == "archive") {
        auto icon_btn = Gtk::make_managed<Gtk::Button>();
        icon_btn->set_icon_name("insert-image-symbolic");
        icon_btn->add_css_class("row-action-btn");
        icon_btn->add_css_class("icon-btn");
        icon_btn->set_cursor("pointer");
        icon_btn->set_tooltip_text("Modifier l'icône de l'application");
        icon_btn->signal_clicked().connect([this, app]() { on_change_icon(app); });
        actions_box->append(*icon_btn);
    }

    // Action C : Désinstallation directe (non-système)
    if (!app.is_system) {
        auto delete_btn = Gtk::make_managed<Gtk::Button>();
        delete_btn->set_icon_name("user-trash-symbolic");
        delete_btn->add_css_class("row-action-btn");
        delete_btn->add_css_class("delete-btn");
        delete_btn->set_cursor("pointer");
        delete_btn->set_tooltip_text("Désinstaller " + app.name);
        delete_btn->signal_clicked().connect([this, app]() { on_uninstall_requested(app); });
        actions_box->append(*delete_btn);
    }

    row->append(*actions_box);
    return row;
}

void InstalledPage::open_app_details(const core::InstalledApp& app) {
    if (!m_details_modal) {
        auto root = dynamic_cast<Gtk::Window*>(get_root());
        if (root) {
            m_details_modal = std::make_unique<AppDetailsModal>(*root);
            m_details_modal->signal_app_uninstalled().connect([this]() {
                reload_apps();
            });
        }
    }
    if (m_details_modal) {
        m_details_modal->show_app(app);
    }
}

void InstalledPage::on_modify_permissions(const core::InstalledApp& app) {
    auto root = dynamic_cast<Gtk::Window*>(get_root());
    if (root) {
        if (!m_permissions_modal) {
            m_permissions_modal = std::make_unique<FlatpakPermissionsModal>(*root);
        }
        m_permissions_modal->show_permissions(app);
    }
}

void InstalledPage::on_change_icon(const core::InstalledApp& app) {
    auto file_dialog = Gtk::FileDialog::create();
    file_dialog->set_title("Sélectionner une nouvelle icône pour " + app.name);

    auto filter = Gtk::FileFilter::create();
    filter->set_name("Images (*.png, *.svg, *.jpg)");
    filter->add_mime_type("image/png");
    filter->add_mime_type("image/svg+xml");
    filter->add_mime_type("image/jpeg");

    auto filters = Gio::ListStore<Gtk::FileFilter>::create();
    filters->append(filter);
    file_dialog->set_filters(filters);

    file_dialog->open([this, app, file_dialog](const Glib::RefPtr<Gio::AsyncResult>& res) {
        try {
            auto file = file_dialog->open_finish(res);
            if (file) {
                std::string path = file->get_path();
                if (!path.empty()) {
                    core::AbpmService::set_app_icon(app.package, path);
                    reload_apps();
                }
            }
        } catch (...) {}
    });
}

void InstalledPage::on_uninstall_requested(const core::InstalledApp& app) {
    auto dialog = Gtk::AlertDialog::create("Désinstaller " + app.name + " ?");
    dialog->set_detail("Cette action supprimera définitivement l'application via abpm.");
    dialog->set_buttons({"Annuler", "Désinstaller"});
    dialog->set_cancel_button(0);
    dialog->set_default_button(1);

    dialog->choose([this, app, dialog](const Glib::RefPtr<Gio::AsyncResult>& res) {
        try {
            int btn = dialog->choose_finish(res);
            if (btn == 1) {
                std::thread([this, app]() {
                    core::AbpmService::uninstall_app(app.package, app.source);
                    reload_apps();
                }).detach();
            }
        } catch (...) {}
    });
}

} // namespace bluehub::ui