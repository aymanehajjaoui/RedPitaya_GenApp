/*ConnectionManager.hpp*/

#pragma once

#include <gtkmm.h>
#include <string>
#include <cstdlib>

class ConnectionManager
{
public:
    static bool connectToRedPitaya(Gtk::Dialog &dialog, Gtk::Button &buttonConnect, Gtk::Entry &entryMac, Gtk::Entry &entryIP, Gtk::Entry &entryPassword, Gtk::Entry &entryDirectory);
    static bool pingHost(const std::string& hostname);
    static bool isSSHConnectionAlive(const std::string &hostname, const std::string &password, const std::string &privateKeyPath);
};