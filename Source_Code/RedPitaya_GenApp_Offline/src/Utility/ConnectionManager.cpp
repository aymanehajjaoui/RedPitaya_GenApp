/*ConnectionManager.cpp*/

#include "Utility/ConnectionManager.hpp"
#include "Utility/SSHManager.hpp"

bool ConnectionManager::connectToRedPitaya(Gtk::Dialog &dialog, Gtk::Button &buttonConnect, Gtk::Entry &entryMac, Gtk::Entry &entryIP, Gtk::Entry &entryPassword, Gtk::Entry &entryDirectory)
{
    std::string hostname;
    std::string password = entryPassword.get_text();

    if (entryMac.get_sensitive())
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
    else
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

bool ConnectionManager::pingHost(const std::string &hostname)
{
    std::string command = "ping -c 1 -W 2 " + hostname + " > /dev/null 2>&1";
    int result = std::system(command.c_str());
    return result == 0;
}

bool ConnectionManager::isSSHConnectionAlive(const std::string &hostname, const std::string &password, const std::string &privateKeyPath)
{
    std::string userHost = "root@" + hostname;
    std::string cmd;

    if (!privateKeyPath.empty())
        cmd = "ssh -i " + privateKeyPath + " -o ConnectTimeout=2 -o StrictHostKeyChecking=no " + userHost + " 'echo ok' > /dev/null 2>&1";
    else
        cmd = "sshpass -p \"" + password + "\" ssh -o ConnectTimeout=2 -o StrictHostKeyChecking=no " + userHost + " 'echo ok' > /dev/null 2>&1";

    return std::system(cmd.c_str()) == 0;
}
