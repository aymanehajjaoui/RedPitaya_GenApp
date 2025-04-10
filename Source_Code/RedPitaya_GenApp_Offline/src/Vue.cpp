/*Vue.cpp*/

#include "Vue.hpp"
#include "ConnectionManager.hpp"
#include "ExportManager.hpp"
#include "FileManager.hpp"
#include <stdexcept>

Vue::Vue() : buttonBrowseModel("Browse model generated by QualIA"),
             buttonExportLocally("Export locally"),
             buttonExportToRedPitaya("Export to Red Pitaya"),
             buttonHelp("Help"),
             buttonQuit("Quit"),
             modelLoaded(false) // Initialize modelLoaded flag
{
    set_title("Model Template Generator");
    set_border_width(10);
    set_default_size(400, 100);

    buttonBrowseModel.signal_clicked().connect(sigc::mem_fun(*this, &Vue::onButtonBrowseModel));
    buttonExportLocally.signal_clicked().connect(sigc::mem_fun(*this, &Vue::onExportLocallyClicked));
    buttonExportToRedPitaya.signal_clicked().connect(sigc::mem_fun(*this, &Vue::onExportToRedPitayaClicked));
    buttonHelp.signal_clicked().connect(sigc::mem_fun(*this, &Vue::onHelpClicked));
    buttonQuit.signal_clicked().connect(sigc::mem_fun(*this, &Vue::onQuitClicked));

    box.pack_start(buttonBrowseModel, Gtk::PACK_SHRINK);
    box.pack_start(buttonExportLocally, Gtk::PACK_SHRINK);
    box.pack_start(buttonExportToRedPitaya, Gtk::PACK_SHRINK);
    box.pack_start(buttonHelp, Gtk::PACK_SHRINK);
    box.pack_start(buttonQuit, Gtk::PACK_SHRINK);

    add(box);
    show_all_children();

    // Initially disable export buttons
    buttonExportLocally.set_sensitive(false);
    buttonExportToRedPitaya.set_sensitive(false);
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

    if (!ExportManager::exportLocally(modelFolder, genFilesDir))
    {
        Gtk::MessageDialog errorDialog(*this, "Failed to export model locally.", false, Gtk::MESSAGE_ERROR);
        errorDialog.run();
    }
}

void Vue::onExportToRedPitayaClicked()
{
    if (!modelLoaded)
    {
        Gtk::MessageDialog dialog(*this, "Load the model first and then try again.", false, Gtk::MESSAGE_ERROR);
        dialog.run();
        return;
    }

    Gtk::Dialog dialog("Connect to Red Pitaya", *this);
    dialog.set_modal(true);
    dialog.set_default_size(400, 350);
    Gtk::Box *contentArea = dialog.get_content_area();

    Gtk::RadioButton radioMac("Use last 6 characters of MAC address");
    Gtk::RadioButton radioIP("Use IP address of Red Pitaya");
    Gtk::Entry entryMac, entryIP, entryPassword, entryDirectory;
    Gtk::Button buttonConnect("Connect"), buttonExport("Export"), buttonExit("Exit");
    Gtk::Label labelPassword("Enter SSH Password:");
    Gtk::Label labelDirectory("Enter Target Directory on Red Pitaya:");

    // Initial states
    entryPassword.set_visibility(false); // Hide password text
    entryDirectory.set_sensitive(false); // Disable directory input initially
    buttonExport.set_sensitive(false);   // Disable export initially

    // Group radio buttons
    Gtk::RadioButton::Group group = radioMac.get_group();
    radioIP.set_group(group);
    entryMac.set_sensitive(true);
    entryIP.set_sensitive(false);

    // Toggle behavior
    radioMac.signal_toggled().connect([&]() {
        entryMac.set_sensitive(radioMac.get_active());
        entryIP.set_sensitive(!radioMac.get_active());
    });

    radioIP.signal_toggled().connect([&]() {
        entryMac.set_sensitive(!radioIP.get_active());
        entryIP.set_sensitive(radioIP.get_active());
    });

    // Pack widgets
    contentArea->pack_start(radioMac, Gtk::PACK_SHRINK);
    contentArea->pack_start(entryMac, Gtk::PACK_SHRINK);
    contentArea->pack_start(radioIP, Gtk::PACK_SHRINK);
    contentArea->pack_start(entryIP, Gtk::PACK_SHRINK);
    contentArea->pack_start(labelPassword, Gtk::PACK_SHRINK);
    contentArea->pack_start(entryPassword, Gtk::PACK_SHRINK);
    contentArea->pack_start(buttonConnect, Gtk::PACK_SHRINK);
    contentArea->pack_start(labelDirectory, Gtk::PACK_SHRINK);
    contentArea->pack_start(entryDirectory, Gtk::PACK_SHRINK);
    contentArea->pack_start(buttonExport, Gtk::PACK_SHRINK);
    contentArea->pack_start(buttonExit, Gtk::PACK_SHRINK);

    // Exit button action
    buttonExit.signal_clicked().connect([&]() { dialog.close(); });

    // Connect button action
    buttonConnect.signal_clicked().connect([&]() {
        if (ConnectionManager::connectToRedPitaya(dialog, buttonConnect, entryMac, entryIP, entryPassword, entryDirectory))
        {
            buttonConnect.set_label("Connected");
            buttonConnect.set_sensitive(false);
            entryDirectory.set_sensitive(true);
            buttonExport.set_sensitive(true);
        }
    });

    // Export button action
    buttonExport.signal_clicked().connect([&]() {
        std::string hostname = radioMac.get_active() ? "rp-" + entryMac.get_text() + ".local" : entryIP.get_text();
        std::string password = entryPassword.get_text();
        std::string targetDirectory = entryDirectory.get_text();

        if (targetDirectory.empty())
        {
            Gtk::MessageDialog errorDialog(dialog, "Please enter a directory path!", false, Gtk::MESSAGE_ERROR);
            errorDialog.run();
            return;
        }

        if (!ExportManager::exportToRedPitaya(modelFolder, genFilesDir, hostname, password, targetDirectory))
        {
            Gtk::MessageDialog errorDialog(dialog, "Failed to export model to Red Pitaya.", false, Gtk::MESSAGE_ERROR);
            errorDialog.run();
        }
        else
        {
            Gtk::MessageDialog successDialog(dialog, "Files and directories exported successfully!", false, Gtk::MESSAGE_INFO);
            successDialog.run();
        }
    });

    dialog.show_all();
    dialog.run();
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
