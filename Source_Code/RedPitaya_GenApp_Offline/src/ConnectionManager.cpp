/*ConnectionManager.cpp*/

#include "ConnectionManager.hpp"
#include "SSHManager.hpp"

bool ConnectionManager::connectToRedPitaya(Gtk::Dialog &dialog, Gtk::Button &buttonConnect, Gtk::Entry &entryMac, Gtk::Entry &entryIP, Gtk::Entry &entryPassword, Gtk::Entry &entryDirectory)
{
    std::string hostname;
    std::string password = entryPassword.get_text();

    if (entryMac.get_sensitive()) // If MAC is selected
    {
        std::string macLast6 = entryMac.get_text();
        if (macLast6.length() != 6)
        {
            Gtk::MessageDialog errorDialog(dialog, "Invalid MAC address! Please enter exactly 6 characters.", false, Gtk::MESSAGE_ERROR);
            errorDialog.run();
            return false;
        }
        hostname = "rp-" + macLast6 + ".local";
    }
    else // If IP is selected
    {
        hostname = entryIP.get_text();
        if (hostname.empty())
        {
            Gtk::MessageDialog errorDialog(dialog, "Invalid IP address! Please enter a valid IP.", false, Gtk::MESSAGE_ERROR);
            errorDialog.run();
            return false;
        }
    }

    if (password.empty())
    {
        Gtk::MessageDialog errorDialog(dialog, "Please enter a password!", false, Gtk::MESSAGE_ERROR);
        errorDialog.run();
        return false;
    }

    bool success = SSHManager::connect_to_ssh(hostname, password);
    if (success)
    {
        Gtk::MessageDialog successDialog(dialog, "SSH Connection Successful!", false, Gtk::MESSAGE_INFO);
        successDialog.run();

        buttonConnect.set_label("Connected");
        buttonConnect.set_sensitive(false);
        entryDirectory.set_sensitive(true);

        return true;
    }
    else
    {
        Gtk::MessageDialog errorDialog(dialog, "SSH Connection Failed! Incorrect IP/MAC or password.", false, Gtk::MESSAGE_ERROR);
        errorDialog.run();
        return false;
    }
}
