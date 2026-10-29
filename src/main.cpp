#include <adwaita.h>
#include <gtkmm.h>
#include "ui/main_window.hpp"

int main(int argc, char* argv[]) {
    // 1. Initialisation de Libadwaita
    adw_init();

    // 2. Création de l'application GTK
    auto app = Gtk::Application::create("io.github.arrerasoftware.bluehub");

    app->signal_startup().connect([app]() {
        // Chargement du style CSS
        auto css_provider = Gtk::CssProvider::create();
        try {
            css_provider->load_from_path("data/style.css");
            Gtk::StyleProvider::add_provider_for_display(
                Gdk::Display::get_default(),
                css_provider,
                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
            );
        } catch (const Glib::Error& ex) {
            g_warning("Impossible de charger style.css : %s", ex.what());
        }
    });

    // 3. Lancement de la fenêtre principale
    return app->make_window_and_run<bluehub::ui::MainWindow>(argc, argv);
}