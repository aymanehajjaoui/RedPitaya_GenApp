/*Vue.cpp*/

#include "Vue.hpp"
#include "ConnectionManager.hpp"
#include "ExportManager.hpp"
#include "FileManager.hpp"
#include "SelectDialog.hpp"
#include <stdexcept>

Vue::Vue() : buttonBrowseModel("Browse model generated by QualIA"),
             buttonExportLocally("Export locally"),
             buttonConnectRedPitaya("Connect to RedPitaya"),
             buttonExportToRedPitaya("Export to RedPitaya"),
             buttonHelp("Help"),
             buttonQuit("Quit"),
             modelLoaded(false),
             redpitayaConnected(false)
{
    set_title("Model Template Generator");
    set_border_width(10);
    set_default_size(400, 120);

    // Signal connections
    buttonBrowseModel.signal_clicked().connect(sigc::mem_fun(*this, &Vue::onButtonBrowseModel));
    buttonExportLocally.signal_clicked().connect(sigc::mem_fun(*this, &Vue::onExportLocallyClicked));
    buttonConnectRedPitaya.signal_clicked().connect(sigc::mem_fun(*this, &Vue::onConnectRedPitayaClicked));
    buttonExportToRedPitaya.signal_clicked().connect(sigc::mem_fun(*this, &Vue::onExportToRedPitayaClicked));
    buttonHelp.signal_clicked().connect(sigc::mem_fun(*this, &Vue::onHelpClicked));
    buttonQuit.signal_clicked().connect(sigc::mem_fun(*this, &Vue::onQuitClicked));

    box.pack_start(buttonBrowseModel, Gtk::PACK_SHRINK);
    box.pack_start(buttonExportLocally, Gtk::PACK_SHRINK);
    box.pack_start(buttonConnectRedPitaya, Gtk::PACK_SHRINK);
    box.pack_start(buttonExportToRedPitaya, Gtk::PACK_SHRINK);
    box.pack_start(buttonHelp, Gtk::PACK_SHRINK);
    box.pack_start(buttonQuit, Gtk::PACK_SHRINK);

    add(box);
    show_all_children();

    buttonExportLocally.set_sensitive(false);
    buttonExportToRedPitaya.set_sensitive(false); // Enable only if both model & redpitaya connected
}

Vue::~Vue() {}

void Vue::onButtonBrowseModel()
{
    Gtk::FileChooserDialog dialog("Choose the model folder", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_OK", Gtk::RESPONSE_OK);

    if (dialog.run() == Gtk::RESPONSE_OK)
    {
        modelFolder = dialog.get_filename();
        std::cout << "Selected model folder: " << modelFolder << std::endl;

        if (FileManager::isValidQualiaModel(modelFolder))
        {
            modelLoaded = true;
            buttonExportLocally.set_sensitive(true);
            if (redpitayaConnected)
                buttonExportToRedPitaya.set_sensitive(true);
        }
        else
        {
            Gtk::MessageDialog errorDialog(*this, "Model selected seems to not be generated by Qualia.", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
            errorDialog.set_secondary_text("Please select a folder that contains: full_model.h, model.c, and include/model.h.");
            errorDialog.run();
        }
    }
}

void Vue::onExportLocallyClicked()
{
    if (!modelLoaded)
    {
        Gtk::MessageDialog dialog(*this, "Load the model first and then try again.", false, Gtk::MESSAGE_ERROR);
        dialog.run();
        return;
    }

    SelectDialog selectDialog(*this);
    if (selectDialog.run() == Gtk::RESPONSE_OK)
    {
        auto selectedVersions = selectDialog.get_selected_versions();
        if (selectedVersions.empty())
        {
            Gtk::MessageDialog warning(*this, "No versions selected.", false, Gtk::MESSAGE_WARNING);
            warning.set_secondary_text("Please select at least one version to export.");
            warning.run();
            return;
        }

        if (!ExportManager::exportLocally(modelFolder, genFilesDir, selectedVersions))
        {
            Gtk::MessageDialog errorDialog(*this, "Failed to export model locally.", false, Gtk::MESSAGE_ERROR);
            errorDialog.run();
        }
    }
}

void Vue::onConnectRedPitayaClicked()
{
    Gtk::Dialog dialog("Connect to Red Pitaya", *this);
    dialog.set_modal(true);
    dialog.set_default_size(400, 300);
    Gtk::Box *contentArea = dialog.get_content_area();

    Gtk::RadioButton radioMac("Use last 6 characters of MAC address");
    Gtk::RadioButton radioIP("Use IP address of Red Pitaya");
    Gtk::Entry entryMac, entryIP, entryPassword;
    Gtk::Button buttonConnect("Connect");
    Gtk::Label labelPassword("Enter SSH Password:");

    // Group and toggle setup
    Gtk::RadioButton::Group group = radioMac.get_group();
    radioIP.set_group(group);
    entryMac.set_sensitive(true);
    entryIP.set_sensitive(false);
    entryPassword.set_visibility(false); // Hide password characters

    radioMac.signal_toggled().connect([&]()
                                      {
        entryMac.set_sensitive(radioMac.get_active());
        entryIP.set_sensitive(!radioMac.get_active()); });

    radioIP.signal_toggled().connect([&]()
                                     {
        entryMac.set_sensitive(!radioIP.get_active());
        entryIP.set_sensitive(radioIP.get_active()); });

    // Pack UI elements
    contentArea->pack_start(radioMac, Gtk::PACK_SHRINK);
    contentArea->pack_start(entryMac, Gtk::PACK_SHRINK);
    contentArea->pack_start(radioIP, Gtk::PACK_SHRINK);
    contentArea->pack_start(entryIP, Gtk::PACK_SHRINK);
    contentArea->pack_start(labelPassword, Gtk::PACK_SHRINK);
    contentArea->pack_start(entryPassword, Gtk::PACK_SHRINK);
    contentArea->pack_start(buttonConnect, Gtk::PACK_SHRINK);

    buttonConnect.signal_clicked().connect([&, this]()
                                           {
        std::string hostname;
        std::string password = entryPassword.get_text();

        if (radioMac.get_active()) {
            std::string macSuffix = entryMac.get_text();
            if (macSuffix.length() != 6) {
                Gtk::MessageDialog err(dialog, "Invalid MAC address suffix.", false, Gtk::MESSAGE_ERROR);
                err.set_secondary_text("Please enter exactly 6 characters (e.g. F1E2D3).");
                err.run();
                return;
            }
            hostname = "rp-" + macSuffix + ".local";
        } else {
            hostname = entryIP.get_text();
            if (hostname.empty()) {
                Gtk::MessageDialog err(dialog, "Invalid IP address.", false, Gtk::MESSAGE_ERROR);
                err.set_secondary_text("Please enter a valid IP address (e.g. 192.168.1.100).");
                err.run();
                return;
            }
        }

        if (password.empty()) {
            Gtk::MessageDialog err(dialog, "Password required.", false, Gtk::MESSAGE_ERROR);
            err.set_secondary_text("Please enter your RedPitaya's SSH password.");
            err.run();
            return;
        }

        if (ConnectionManager::isSSHConnectionAlive(hostname, password))
        {
            redpitayaHost = hostname;
            redpitayaPassword = password;
            redpitayaConnected = true;

            buttonConnectRedPitaya.set_label("Connected!");
            buttonConnectRedPitaya.set_sensitive(false);

            if (modelLoaded)
                buttonExportToRedPitaya.set_sensitive(true);

            dialog.close();
        }
        else
        {
            Gtk::MessageDialog err(dialog, "SSH authentication failed.", false, Gtk::MESSAGE_ERROR);
            err.set_secondary_text("Check the IP/MAC and password. Ensure sshpass is installed and RedPitaya is reachable.");
            err.run();
        } });

    dialog.show_all();
    dialog.run();
}

void Vue::onExportToRedPitayaClicked()
{
    // Check SSH connection availability
    if (!redpitayaConnected ||
        !ConnectionManager::pingHost(redpitayaHost) ||
        !ConnectionManager::isSSHConnectionAlive(redpitayaHost, redpitayaPassword))
    {
        redpitayaConnected = false;
        buttonExportToRedPitaya.set_sensitive(false);
        buttonConnectRedPitaya.set_sensitive(true);
        buttonConnectRedPitaya.set_label("Connect to RedPitaya");

        Gtk::MessageDialog dialog(*this, "SSH connection to RedPitaya seems lost.", false, Gtk::MESSAGE_ERROR);
        dialog.set_secondary_text("Please reconnect using the 'Connect to RedPitaya' button.");
        dialog.run();
        return;
    }

    // Ask user to enter the target folder path (on RedPitaya)
    Gtk::Dialog dirDialog("Enter target folder on RedPitaya", *this);
    dirDialog.set_modal(true);
    dirDialog.set_default_size(400, 100);
    Gtk::Box *box = dirDialog.get_content_area();

    Gtk::Label label("Enter the full path to the target directory (e.g. /root/myfolder):");
    Gtk::Entry entry;
    entry.set_text("/root/");
    entry.set_activates_default(true);

    dirDialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dirDialog.add_button("_OK", Gtk::RESPONSE_OK);
    dirDialog.set_default_response(Gtk::RESPONSE_OK);

    box->pack_start(label, Gtk::PACK_SHRINK);
    box->pack_start(entry, Gtk::PACK_SHRINK);
    dirDialog.show_all();

    std::string targetDirectory;
    if (dirDialog.run() == Gtk::RESPONSE_OK)
    {
        targetDirectory = entry.get_text();
    }

    if (targetDirectory.empty())
    {
        Gtk::MessageDialog errorDialog(*this, "Please enter a directory path!", false, Gtk::MESSAGE_ERROR);
        errorDialog.run();
        return;
    }

    // Show SelectDialog to choose versions
    SelectDialog selectDialog(*this);
    if (selectDialog.run() != Gtk::RESPONSE_OK)
        return;

    auto selectedVersions = selectDialog.get_selected_versions();
    if (selectedVersions.empty())
    {
        Gtk::MessageDialog warning(*this, "No versions selected.", false, Gtk::MESSAGE_WARNING);
        warning.set_secondary_text("Please select at least one version to export.");
        warning.run();
        return;
    }

    // Attempt export
    if (!ExportManager::exportToRedPitaya(modelFolder, genFilesDir, selectedVersions, redpitayaHost, redpitayaPassword, targetDirectory))
    {
        Gtk::MessageDialog errorDialog(*this, "Failed to export model to Red Pitaya.", false, Gtk::MESSAGE_ERROR);
        errorDialog.run();
    }
    else
    {
        Gtk::MessageDialog successDialog(*this, "Files and directories exported successfully!", false, Gtk::MESSAGE_INFO);
        successDialog.run();
    }
}

void Vue::onHelpClicked()
{
    Gtk::MessageDialog helpDialog(*this, "Help & Dependencies", false, Gtk::MESSAGE_INFO);
    helpDialog.set_secondary_text(
        "To export to Red Pitaya, ensure you have the required dependencies installed:\n\n"
        "For Ubuntu/Debian:\n"
        "    sudo apt update\n"
        "    sudo apt install sshpass\n\n"
        "For Arch Linux:\n"
        "    sudo pacman -S sshpass\n\n"
        "For macOS (Homebrew):\n"
        "    brew install hudochenkov/sshpass/sshpass\n\n"
        "Once installed, restart the program and retry the export.");
    helpDialog.run();
}

void Vue::onQuitClicked()
{
    hide();
}
