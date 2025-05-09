/*ConnectRedPitayaHandler.cpp*/

#include "buttonsHandler/ConnectRedPitayaHandler.hpp"
#include "Utility/ConnectionManager.hpp"
#include "Utility/RSAKeyDialog.hpp"

namespace ConnectRedPitayaHandler
{
    static void showErrorDialog(Gtk::Window *parent, const std::string &title, const std::string &text)
    {
        auto dialog = new Gtk::MessageDialog(*parent, title, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, false);
        if (!text.empty())
            dialog->set_secondary_text(text);
        dialog->signal_response().connect([dialog](int)
                                          {
            dialog->hide();
            delete dialog; });
        dialog->show_all();
    }

    void handle(Gtk::Window *parentWindow,
                Gtk::Button &buttonConnectRedPitaya,
                Gtk::Button &buttonExportToRedPitaya,
                DetailsPanel &detailsPanel,
                std::string &redpitayaHost,
                std::string &redpitayaPassword,
                std::string &redpitayaPrivateKeyPath,
                bool &redpitayaConnected,
                bool modelLoaded)
    {
        buttonConnectRedPitaya.set_sensitive(false);

        auto dialog = new Gtk::Dialog("Connect to Red Pitaya", false);
        dialog->set_transient_for(*parentWindow);
        dialog->set_modal(false);
        dialog->set_resizable(true);
        dialog->set_position(Gtk::WIN_POS_CENTER);
        dialog->set_default_size(400, 400);
        Gtk::Box *contentArea = dialog->get_content_area();

        auto radioMac = Gtk::make_managed<Gtk::RadioButton>("Use last 6 characters of MAC address");
        auto radioIP = Gtk::make_managed<Gtk::RadioButton>("Use IP address of Red Pitaya");
        Gtk::RadioButton::Group group = radioMac->get_group();
        radioIP->set_group(group);

        auto entryMac = Gtk::make_managed<Gtk::Entry>();
        auto entryIP = Gtk::make_managed<Gtk::Entry>();
        auto entryPassword = Gtk::make_managed<Gtk::Entry>();
        entryPassword->set_visibility(false);

        auto checkUseRSA = Gtk::make_managed<Gtk::CheckButton>("Use RSA Private Key");
        auto entryPrivateKeyPath = Gtk::make_managed<Gtk::Entry>();
        entryPrivateKeyPath->set_placeholder_text("Path to private key (e.g. ~/.ssh/id_rsa_redpitaya)");
        entryPrivateKeyPath->set_sensitive(false);

        auto buttonCreateRSAKey = Gtk::make_managed<Gtk::Button>("Generate RSA Key");
        buttonCreateRSAKey->set_sensitive(false);

        auto buttonCopyRSAKey = Gtk::make_managed<Gtk::Button>("Copy RSA Key to RedPitaya");
        buttonCopyRSAKey->set_sensitive(false);

        auto labelPassword = Gtk::make_managed<Gtk::Label>("Enter SSH Password:");
        auto buttonConnect = Gtk::make_managed<Gtk::Button>("Connect");
        auto buttonHelp = Gtk::make_managed<Gtk::Button>("Help"); 

        radioMac->signal_toggled().connect([radioMac, entryMac, entryIP]()
                                           {
            entryMac->set_sensitive(radioMac->get_active());
            entryIP->set_sensitive(!radioMac->get_active()); });

        radioIP->signal_toggled().connect([radioIP, entryMac, entryIP]()
                                          {
            entryMac->set_sensitive(!radioIP->get_active());
            entryIP->set_sensitive(radioIP->get_active()); });

        auto updateCopyRSAButton = [=]()
        {
            bool useRSA = checkUseRSA->get_active();
            std::string keyPath = entryPrivateKeyPath->get_text();
            if (useRSA && !keyPath.empty())
            {
                if (keyPath[0] == '~')
                {
                    const char *home = std::getenv("HOME");
                    if (home)
                        keyPath.replace(0, 1, home);
                }
                buttonCopyRSAKey->set_sensitive(std::filesystem::exists(keyPath));
            }
            else
            {
                buttonCopyRSAKey->set_sensitive(false);
            }
        };

        checkUseRSA->signal_toggled().connect([=]()
                                              {
                                                  bool useRSA = checkUseRSA->get_active();
                                                  entryPrivateKeyPath->set_sensitive(useRSA);
                                                  buttonCreateRSAKey->set_sensitive(useRSA);
                                                  updateCopyRSAButton();
                                                  if (useRSA)
                                                      entryPassword->set_text(""); 
                                              });

        entryPrivateKeyPath->signal_changed().connect([=]()
                                                      { updateCopyRSAButton(); });

        buttonCreateRSAKey->signal_clicked().connect([=]()
                                                     {
            RSAKeyDialog keyDialog(*parentWindow);
            if (keyDialog.run() != Gtk::RESPONSE_OK)
                return;

            std::string path = keyDialog.get_key_path();
            std::string size = keyDialog.get_key_size();
            std::string pass = keyDialog.get_passphrase();

            if (!path.empty() && path[0] == '~') {
                const char *home = std::getenv("HOME");
                if (home)
                    path.replace(0, 1, home);
            }

            if (std::filesystem::exists(path)) {
                Gtk::MessageDialog overwriteDialog(*parentWindow, "Key already exists. Overwrite?", false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL);
                overwriteDialog.set_secondary_text("A key at this location already exists. Do you want to overwrite it?");
                if (overwriteDialog.run() != Gtk::RESPONSE_OK)
                    return;

                std::filesystem::remove(path);
                std::filesystem::remove(path + ".pub");
            }

            std::filesystem::path keyPath(path);
            std::filesystem::path parentDir = keyPath.parent_path();
            if (!parentDir.empty() && !std::filesystem::exists(parentDir)) {
                std::filesystem::create_directories(parentDir);
            }

            std::string cmd = "ssh-keygen -t rsa -b " + size + " -f \"" + path + "\"";
            cmd += pass.empty() ? " -N \"\"" : " -N \"" + pass + "\"";

            if (std::system(cmd.c_str()) == 0) {
                auto infoDialog = new Gtk::MessageDialog(*parentWindow, "RSA key generated successfully.", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, false);
                infoDialog->signal_response().connect([infoDialog](int) {
                    infoDialog->hide();
                    delete infoDialog;
                });
                infoDialog->show_all();
                entryPrivateKeyPath->set_text(path);
            } else {
                showErrorDialog(parentWindow, "Key generation failed", "Check the path or file permissions.");
            } });

        buttonCopyRSAKey->signal_clicked().connect([=]()
                                                   {
            std::string pubKeyPath = entryPrivateKeyPath->get_text() + ".pub";
            std::string hostname;

            if (radioMac->get_active()) {
                std::string mac = entryMac->get_text();
                if (mac.length() != 6) {
                    showErrorDialog(parentWindow, "Invalid MAC", "Enter 6-character MAC suffix.");
                    return;
                }
                hostname = "rp-" + mac + ".local";
            } else {
                hostname = entryIP->get_text();
            }

            if (hostname.empty()) {
                showErrorDialog(parentWindow, "Missing host", "Please enter IP or MAC.");
                return;
            }

            Gtk::Dialog pwdDialog("Enter RedPitaya password", *parentWindow);
            pwdDialog.set_default_size(400, 150);
            pwdDialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
            pwdDialog.add_button("OK", Gtk::RESPONSE_OK);
            Gtk::Entry *pwdEntry = Gtk::make_managed<Gtk::Entry>();
            pwdEntry->set_visibility(false);
            pwdDialog.get_content_area()->pack_start(*Gtk::make_managed<Gtk::Label>("Password to copy RSA key:"), Gtk::PACK_SHRINK);
            pwdDialog.get_content_area()->pack_start(*pwdEntry, Gtk::PACK_SHRINK);
            pwdDialog.show_all();
            if (pwdDialog.run() != Gtk::RESPONSE_OK)
                return;

            std::string password = pwdEntry->get_text();
            std::string cmd = "sshpass -p \"" + password + "\" ssh-copy-id -i \"" + pubKeyPath + "\" root@" + hostname;
            if (std::system(cmd.c_str()) == 0) {
                auto infoDialog = new Gtk::MessageDialog(*parentWindow, "RSA key copied successfully.", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, false);
                infoDialog->signal_response().connect([infoDialog](int) {
                    infoDialog->hide();
                    delete infoDialog;
                });
                infoDialog->show_all();
            } else {
                showErrorDialog(parentWindow, "Failed to copy RSA key", "Check password, host, or key path.");
            } });

        buttonConnect->signal_clicked().connect([=, &redpitayaHost, &redpitayaPassword, &redpitayaPrivateKeyPath, &redpitayaConnected, &detailsPanel, &buttonConnectRedPitaya, &buttonExportToRedPitaya]()
                                                {
                std::string hostname;
                std::string password = entryPassword->get_text();
                std::string privateKeyPath;
            
                if (radioMac->get_active()) {
                    std::string mac = entryMac->get_text();
                    if (mac.length() != 6) {
                        showErrorDialog(parentWindow, "Invalid MAC address", "Expected 6 characters (e.g. f0baf6).");
                        return;
                    }
                    hostname = "rp-" + mac + ".local";
                } else {
                    hostname = entryIP->get_text();
                    if (hostname.empty()) {
                        showErrorDialog(parentWindow, "Invalid IP address", "Please provide a valid RedPitaya IP.");
                        return;
                    }
                }
            
                if (checkUseRSA->get_active()) {
                    privateKeyPath = entryPrivateKeyPath->get_text();
                    if (!privateKeyPath.empty() && privateKeyPath[0] == '~') {
                        const char *home = std::getenv("HOME");
                        if (home)
                            privateKeyPath.replace(0, 1, home);
                    }
            
                    if (privateKeyPath.empty()) {
                        showErrorDialog(parentWindow, "Missing RSA path", "Please enter the private key path.");
                        return;
                    }
            
                    if (std::filesystem::is_directory(privateKeyPath)) {
                        showErrorDialog(parentWindow, "Invalid RSA path",
                                        "The path you entered is a directory.\nPlease specify the full path to your private key file (e.g. ~/.ssh/id_rsa_redpitaya).");
                        return;
                    }
            
                    if (!std::filesystem::exists(privateKeyPath)) {
                        showErrorDialog(parentWindow, "Invalid RSA path",
                                        "The private key file does not exist: " + privateKeyPath);
                        return;
                    }
                } else {
                    if (password.empty()) {
                        showErrorDialog(parentWindow, "Password required", "Enter RedPitaya password.");
                        return;
                    }
                }
            
                detailsPanel.append_log("Connecting to RedPitaya at " + hostname + "...");
                detailsPanel.set_status("Connecting...");
                detailsPanel.set_progress(0.2);
            
                if (ConnectionManager::isSSHConnectionAlive(hostname, password, privateKeyPath)) {
                    redpitayaHost = hostname;
                    redpitayaPassword = password;
                    redpitayaPrivateKeyPath = privateKeyPath;
                    redpitayaConnected = true;
            
                    buttonConnectRedPitaya.set_label("Connected!");
                    buttonConnectRedPitaya.set_sensitive(false);
                    if (modelLoaded)
                        buttonExportToRedPitaya.set_sensitive(true);
            
                    detailsPanel.append_log("Successfully connected to " + hostname);
                    detailsPanel.set_status("Connected");
                    detailsPanel.set_progress(1.0);
            
                    dialog->hide();
                    delete dialog;
                } else {
                    detailsPanel.append_log("Failed to connect to " + hostname);
                    detailsPanel.set_status("Connection failed");
                    detailsPanel.set_progress(0.0);
            
                    showErrorDialog(parentWindow, "Connection failed", "Could not establish SSH connection. Check credentials or reachability.");
                } });

        buttonHelp->signal_clicked().connect([=]()
                                             {
                auto helpDialog = new Gtk::Dialog("How to connect", false);
                helpDialog->set_resizable(false);
                helpDialog->set_position(Gtk::WIN_POS_CENTER);        
                helpDialog->set_transient_for(*parentWindow);
                helpDialog->set_modal(false);
                helpDialog->add_button("OK", Gtk::RESPONSE_OK);
            
                auto label = Gtk::make_managed<Gtk::Label>(
                    "You can connect to your RedPitaya using either of the following:\n\n"
                    "1- If you have the password:\n"
                    " - Use the last 6 characters of its MAC address (e.g., f0baf6)\n"
                    " - Or use its full IP address (e.g., 192.168.1.100)\n\n"
                    "2- If you don’t have the password but have an RSA key:\n"
                    " - Specify the RSA key path and click 'Connect'.\n\n"
                    "You can also generate an RSA key and copy it to the RedPitaya for password-less login.\n\n"
                    "Note: 'sshpass' must be installed to use password-based operations."
                );
                label->set_line_wrap(true);
                label->set_margin_top(10);
                label->set_margin_bottom(10);
                label->set_margin_start(10);
                label->set_margin_end(10);
        
                helpDialog->get_content_area()->pack_start(*label, Gtk::PACK_EXPAND_WIDGET);
                helpDialog->show_all();
            
                helpDialog->signal_response().connect([helpDialog](int) {
                    helpDialog->hide();
                    delete helpDialog;
                }); });

        contentArea->pack_start(*radioMac, Gtk::PACK_SHRINK);
        contentArea->pack_start(*entryMac, Gtk::PACK_SHRINK);
        contentArea->pack_start(*radioIP, Gtk::PACK_SHRINK);
        contentArea->pack_start(*entryIP, Gtk::PACK_SHRINK);
        contentArea->pack_start(*labelPassword, Gtk::PACK_SHRINK);
        contentArea->pack_start(*entryPassword, Gtk::PACK_SHRINK);
        contentArea->pack_start(*checkUseRSA, Gtk::PACK_SHRINK);
        contentArea->pack_start(*entryPrivateKeyPath, Gtk::PACK_SHRINK);
        contentArea->pack_start(*buttonCreateRSAKey, Gtk::PACK_SHRINK);
        contentArea->pack_start(*buttonCopyRSAKey, Gtk::PACK_SHRINK);
        contentArea->pack_start(*buttonConnect, Gtk::PACK_SHRINK);
        contentArea->pack_start(*buttonHelp, Gtk::PACK_SHRINK);

        dialog->signal_response().connect([=, &buttonConnectRedPitaya](int)
                                          {
            dialog->hide();
            delete dialog;
            buttonConnectRedPitaya.set_sensitive(true); });

        dialog->show_all();
    }
}
