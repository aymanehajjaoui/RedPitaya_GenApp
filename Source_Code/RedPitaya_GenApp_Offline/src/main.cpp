/*main.cpp*/

#include <gtkmm/application.h>
#include "Vue.hpp"

int main(int argc, char *argv[])
{
    auto app = Gtk::Application::create(argc, argv, "org.gtkmm.toleconfig");
    Vue vue;
    return app->run(vue);
}
