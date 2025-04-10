/*ConnectionManager.hpp*/

#pragma once

#include <gtkmm.h>
#include <string>

class ConnectionManager
{
public:
    static bool connectToRedPitaya(Gtk::Dialog &dialog, Gtk::Button &buttonConnect, Gtk::Entry &entryMac, Gtk::Entry &entryIP, Gtk::Entry &entryPassword, Gtk::Entry &entryDirectory);
};