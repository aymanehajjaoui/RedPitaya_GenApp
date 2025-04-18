/*Vue.cpp*/

#include "Vue.hpp"
#include "ConnectionManager.hpp"
#include "ExportManager.hpp"
#include "FileManager.hpp"
#include "SelectDialog.hpp"
#include <stdexcept>
#include <thread>

Vue::Vue()
    : buttonBrowseModel("Browse model generated by Qualia"),
      buttonExportLocally("Export locally"),
      buttonConnectRedPitaya("Connect to RedPitaya"),
      buttonExportToRedPitaya("Export to RedPitaya"),
      cancelExportButton("Cancel Export"),
      buttonHelp("Help"),
      buttonQuit("Quit"),
      mainBox(Gtk::ORIENTATION_VERTICAL),
      buttonRowBox(Gtk::ORIENTATION_HORIZONTAL),
      checkShowDetails("Show details"),
      modelLoaded(false),
      redpitayaConnected(false)
{
    set_title("Model Template Generator");
    set_border_width(10);
    set_default_size(1, 1);

    
    buttonBrowseModel.signal_clicked().connect(sigc::mem_fun(*this, &Vue::onButtonBrowseModel));
    buttonExportLocally.signal_clicked().connect(sigc::mem_fun(*this, &Vue::onExportLocallyClicked));
    buttonConnectRedPitaya.signal_clicked().connect(sigc::mem_fun(*this, &Vue::onConnectRedPitayaClicked));
    buttonExportToRedPitaya.signal_clicked().connect(sigc::mem_fun(*this, &Vue::onExportToRedPitayaClicked));
    buttonHelp.signal_clicked().connect(sigc::mem_fun(*this, &Vue::onHelpClicked));
    buttonQuit.signal_clicked().connect(sigc::mem_fun(*this, &Vue::onQuitClicked));
    checkShowDetails.signal_toggled().connect(sigc::mem_fun(*this, &Vue::onCheckShowDetailsClicked));

    cancelExportButton.signal_clicked().connect([this]()
                                                {
        cancelExportFlag = true;
        Glib::signal_idle().connect_once([this]() {
            detailsPanel.append_log("Export cancellation requested.");
            detailsPanel.set_status("Cancelling...");
        }); });

    
    buttonRowBox.pack_start(buttonBrowseModel, Gtk::PACK_SHRINK);
    buttonRowBox.pack_start(buttonExportLocally, Gtk::PACK_SHRINK);
    buttonRowBox.pack_start(buttonConnectRedPitaya, Gtk::PACK_SHRINK);
    buttonRowBox.pack_start(buttonExportToRedPitaya, Gtk::PACK_SHRINK);
    buttonRowBox.pack_start(cancelExportButton, Gtk::PACK_SHRINK);
    buttonRowBox.pack_start(buttonHelp, Gtk::PACK_SHRINK);
    buttonRowBox.pack_start(buttonQuit, Gtk::PACK_SHRINK);

    mainBox.pack_start(buttonRowBox, Gtk::PACK_SHRINK);
    mainBox.pack_start(checkShowDetails, Gtk::PACK_SHRINK);
    mainBox.pack_start(detailsPanel, Gtk::PACK_EXPAND_WIDGET);

    add(mainBox);
    show_all_children();

    
    detailsPanel.hide();
    checkShowDetails.set_active(false);

    
    buttonExportLocally.set_sensitive(false);
    buttonExportToRedPitaya.set_sensitive(false);
    cancelExportButton.set_sensitive(false);
}

Vue::~Vue() {}

void Vue::onButtonBrowseModel()
{
    buttonBrowseModel.set_sensitive(false);

    auto dialog = new Gtk::FileChooserDialog("Choose the model folder", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
    dialog->set_transient_for(*this);
    dialog->set_modal(false);
    dialog->set_resizable(true);
    dialog->set_position(Gtk::WIN_POS_CENTER);
    dialog->set_default_size(600, 400);
    dialog->add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog->add_button("_OK", Gtk::RESPONSE_OK);

    dialog->signal_response().connect([this, dialog](int response)
                                      {
        buttonBrowseModel.set_sensitive(true);
        dialog->hide();
        std::string folder = dialog->get_filename();
        delete dialog;

        if (response != Gtk::RESPONSE_OK || folder.empty()) return;

        modelFolder = folder;
        std::cout << "Selected model folder: " << modelFolder << std::endl;

        detailsPanel.append_log("Selected model folder: " + modelFolder);
        detailsPanel.set_status("Checking model validity...");
        detailsPanel.set_progress(0.2);

        if (FileManager::isValidQualiaModel(modelFolder))
        {
            detailsPanel.append_log("Model folder is valid.");
            detailsPanel.set_status("Model loaded");
            detailsPanel.set_progress(1.0);

            modelLoaded = true;
            buttonExportLocally.set_sensitive(true);
            if (redpitayaConnected)
                buttonExportToRedPitaya.set_sensitive(true);
        }
        else
        {
            detailsPanel.append_log("Invalid model folder structure.");
            detailsPanel.set_status("Invalid model folder");
            detailsPanel.set_progress(0.0);

            auto errorDialog = new Gtk::MessageDialog(*this, "Model selected seems to not be generated by Qualia.", false, Gtk::MESSAGE_ERROR);
            errorDialog->set_secondary_text("Please select a folder that contains: full_model.h, model.c, and include/model.h.");
            errorDialog->set_modal(false);
            errorDialog->set_position(Gtk::WIN_POS_CENTER);
            errorDialog->add_button("OK", Gtk::RESPONSE_OK);
            errorDialog->signal_response().connect([errorDialog](int) {
                errorDialog->hide();
                delete errorDialog;
            });
            errorDialog->show_all();
        } });

    dialog->show_all();
}

void Vue::onExportLocallyClicked()
{
    buttonExportLocally.set_sensitive(false);

    auto selectDialog = new SelectDialog();
    selectDialog->set_modal(false);
    selectDialog->set_position(Gtk::WIN_POS_CENTER);

    selectDialog->signal_response().connect([this, selectDialog](int response)
                                            {
        auto selectedVersions = selectDialog->get_selected_versions();
        selectDialog->hide();
        delete selectDialog;

        if (response != Gtk::RESPONSE_OK || selectedVersions.empty())
        {
            buttonExportLocally.set_sensitive(true);
            return;
        }

        auto folderDialog = new Gtk::FileChooserDialog("Choose the target folder", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
        folderDialog->set_transient_for(*this);
        folderDialog->set_modal(false);
        folderDialog->set_resizable(true);
        folderDialog->set_position(Gtk::WIN_POS_CENTER);
        folderDialog->set_default_size(600, 400);
        folderDialog->add_button("_Cancel", Gtk::RESPONSE_CANCEL);
        folderDialog->add_button("_OK", Gtk::RESPONSE_OK);

        folderDialog->signal_response().connect([this, folderDialog, selectedVersions](int response) {
            std::string targetFolder = folderDialog->get_filename();
            folderDialog->hide();
            delete folderDialog;

            if (response != Gtk::RESPONSE_OK || targetFolder.empty())
            {
                buttonExportLocally.set_sensitive(true);
                return;
            }

            int total = selectedVersions.size();
            int done = 0;
            bool allSucceeded = true;

            detailsPanel.append_log("Exporting " + std::to_string(total) + " version(s) to: " + targetFolder);
            detailsPanel.set_status("Exporting...");
            detailsPanel.set_progress(0.0);

            for (const auto &version : selectedVersions)
            {
                detailsPanel.append_log("Exporting version: " + version + "...");
                bool success = ExportManager::exportLocally(
                    modelFolder, version,
                    targetFolder, cancelExportFlag);
                
                if (success)
                    detailsPanel.append_log("Exported: " + version);
                else
                {
                    detailsPanel.append_log("Failed to export: " + version);
                    allSucceeded = false;
                }

                done++;
                detailsPanel.set_progress(static_cast<double>(done) / total);
            }

            detailsPanel.set_status(allSucceeded ? "Export completed" : "Export completed with errors");

            Gtk::MessageDialog resultDialog(*this,
                                            allSucceeded ? "Export completed successfully." : "Some versions failed to export.",
                                            false,
                                            allSucceeded ? Gtk::MESSAGE_INFO : Gtk::MESSAGE_WARNING);
            resultDialog.run();

            buttonExportLocally.set_sensitive(true);
        });

        folderDialog->show_all(); });

    selectDialog->show_all();
}

void Vue::onConnectRedPitayaClicked()
{
    buttonConnectRedPitaya.set_sensitive(false);

    auto dialog = new Gtk::Dialog("Connect to Red Pitaya", false);
    dialog->set_transient_for(*this);
    dialog->set_modal(false);
    dialog->set_resizable(true);
    dialog->set_position(Gtk::WIN_POS_CENTER);
    dialog->set_default_size(400, 300);

    Gtk::Box *contentArea = dialog->get_content_area();

    auto radioMac = Gtk::make_managed<Gtk::RadioButton>("Use last 6 characters of MAC address");
    auto radioIP = Gtk::make_managed<Gtk::RadioButton>("Use IP address of Red Pitaya");
    Gtk::RadioButton::Group group = radioMac->get_group();
    radioIP->set_group(group);

    auto entryMac = Gtk::make_managed<Gtk::Entry>();
    auto entryIP = Gtk::make_managed<Gtk::Entry>();
    auto entryPassword = Gtk::make_managed<Gtk::Entry>();
    entryPassword->set_visibility(false);

    auto labelPassword = Gtk::make_managed<Gtk::Label>("Enter SSH Password:");
    auto buttonConnect = Gtk::make_managed<Gtk::Button>("Connect");

    entryMac->set_sensitive(true);
    entryIP->set_sensitive(false);

    radioMac->signal_toggled().connect([radioMac, entryMac, entryIP]()
                                       {
        entryMac->set_sensitive(radioMac->get_active());
        entryIP->set_sensitive(!radioMac->get_active()); });

    radioIP->signal_toggled().connect([radioIP, entryMac, entryIP]()
                                      {
        entryMac->set_sensitive(!radioIP->get_active());
        entryIP->set_sensitive(radioIP->get_active()); });

    contentArea->pack_start(*radioMac, Gtk::PACK_SHRINK);
    contentArea->pack_start(*entryMac, Gtk::PACK_SHRINK);
    contentArea->pack_start(*radioIP, Gtk::PACK_SHRINK);
    contentArea->pack_start(*entryIP, Gtk::PACK_SHRINK);
    contentArea->pack_start(*labelPassword, Gtk::PACK_SHRINK);
    contentArea->pack_start(*entryPassword, Gtk::PACK_SHRINK);
    contentArea->pack_start(*buttonConnect, Gtk::PACK_SHRINK);

    buttonConnect->signal_clicked().connect([this, dialog, radioMac, entryMac, entryIP, entryPassword]()
                                            { handleConnectButton(dialog, radioMac, entryMac, entryIP, entryPassword); });

    dialog->signal_response().connect([this, dialog](int)
                                      {
        dialog->hide();
        delete dialog;
        buttonConnectRedPitaya.set_sensitive(true); });

    dialog->show_all();
}

void Vue::handleConnectButton(Gtk::Dialog *dialog,
                              Gtk::RadioButton *radioMac,
                              Gtk::Entry *entryMac,
                              Gtk::Entry *entryIP,
                              Gtk::Entry *entryPassword)
{
    std::string hostname;
    std::string password = entryPassword->get_text();

    if (radioMac->get_active())
    {
        std::string macSuffix = entryMac->get_text();
        if (macSuffix.length() != 6)
        {
            showErrorDialog(dialog, "Invalid MAC address suffix.",
                            "Please enter exactly 6 characters (e.g. F1E2D3).");
            return;
        }
        hostname = "rp-" + macSuffix + ".local";
    }
    else
    {
        hostname = entryIP->get_text();
        if (hostname.empty())
        {
            showErrorDialog(dialog, "Invalid IP address.", "Please enter a valid IP address.");
            return;
        }
    }

    if (password.empty())
    {
        showErrorDialog(dialog, "Password required.", "Please enter your RedPitaya's SSH password.");
        return;
    }

    detailsPanel.append_log("Connecting to RedPitaya at " + hostname + "...");
    detailsPanel.set_status("Connecting...");
    detailsPanel.set_progress(0.2);

    if (ConnectionManager::isSSHConnectionAlive(hostname, password))
    {
        redpitayaHost = hostname;
        redpitayaPassword = password;
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
    }
    else
    {
        detailsPanel.append_log("Failed to connect to " + hostname);
        detailsPanel.set_status("Connection failed");
        detailsPanel.set_progress(0.0);

        showErrorDialog(dialog, "SSH authentication failed.",
                        "Check the IP/MAC and password. Ensure sshpass is installed and the device is reachable.");
    }
}

void Vue::onExportToRedPitayaClicked()
{
    buttonExportToRedPitaya.set_sensitive(false);

    if (!redpitayaConnected ||
        !ConnectionManager::pingHost(redpitayaHost) ||
        !ConnectionManager::isSSHConnectionAlive(redpitayaHost, redpitayaPassword))
    {
        redpitayaConnected = false;
        buttonConnectRedPitaya.set_sensitive(true);
        buttonConnectRedPitaya.set_label("Connect to RedPitaya");

        showErrorDialog(this, "SSH connection to RedPitaya seems lost.",
                        "Please reconnect using the 'Connect to RedPitaya' button.");
        buttonExportToRedPitaya.set_sensitive(true);
        return;
    }

    
    auto dirDialog = new Gtk::Dialog("Enter target folder on RedPitaya", false);
    dirDialog->set_transient_for(*this);
    dirDialog->set_modal(false);
    dirDialog->set_default_size(500, 100);
    dirDialog->set_position(Gtk::WIN_POS_CENTER);

    Gtk::Box *box = dirDialog->get_content_area();
    auto label = Gtk::make_managed<Gtk::Label>("Enter the full path to the target directory (e.g. /root/myfolder):");
    auto entry = Gtk::make_managed<Gtk::Entry>();
    entry->set_text("/root/");
    entry->set_activates_default(true);

    dirDialog->add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dirDialog->add_button("_OK", Gtk::RESPONSE_OK);
    dirDialog->set_default_response(Gtk::RESPONSE_OK);
    box->pack_start(*label, Gtk::PACK_SHRINK);
    box->pack_start(*entry, Gtk::PACK_SHRINK);

    dirDialog->signal_response().connect([this, dirDialog, entry](int response)
                                         {
        std::string targetDirectory;
        if (response == Gtk::RESPONSE_OK)
            targetDirectory = entry->get_text();

        dirDialog->hide();
        delete dirDialog;

        if (targetDirectory.empty())
        {
            showErrorDialog(this, "Please enter a directory path!", "");
            buttonExportToRedPitaya.set_sensitive(true);
            return;
        }

        
        auto selectDialog = new SelectDialog();
        selectDialog->set_modal(false);
        selectDialog->signal_response().connect([this, selectDialog, targetDirectory](int selResp)
        {
            std::vector<std::string> selectedVersions = selectDialog->get_selected_versions();
            selectDialog->hide();
            Glib::signal_idle().connect_once([selectDialog]() {
                delete selectDialog;
            });

            if (selResp != Gtk::RESPONSE_OK || selectedVersions.empty())
            {
                if (selectedVersions.empty())
                {
                    showErrorDialog(this, "No versions selected.",
                                    "Please select at least one version to export.");
                }
                buttonExportToRedPitaya.set_sensitive(true);
                return;
            }

            
            cancelExportButton.set_sensitive(true);
            cancelExportFlag = false;

            std::thread([this, selectedVersions, targetDirectory]() {
                Glib::signal_idle().connect_once([this]() {
                    detailsPanel.append_log("Starting export to RedPitaya...");
                    detailsPanel.set_status("Exporting...");
                    detailsPanel.set_progress(0.0);
                });

                double progressStep = 1.0 / selectedVersions.size();
                double progress = 0.0;

                for (const auto &version : selectedVersions)
                {
                    if (cancelExportFlag)
                    {
                        Glib::signal_idle().connect_once([this]() {
                            detailsPanel.append_log("Export canceled by user.");
                            detailsPanel.set_status("Canceled");
                            detailsPanel.set_progress(0.0);
                            cancelExportButton.set_sensitive(false);
                            buttonExportToRedPitaya.set_sensitive(true);
                        });
                        return;
                    }

                    bool ok = ExportManager::exportSingleVersionToRedPitaya(
                        modelFolder, version,
                        redpitayaHost, redpitayaPassword, targetDirectory,
                        cancelExportFlag);
                    

                    if (!ok)
                    {
                        Glib::signal_idle().connect_once([this, version]() {
                            detailsPanel.append_log("Failed to export version: " + version);
                            detailsPanel.set_status("Export failed");
                            detailsPanel.set_progress(0.0);
                            showErrorDialog(this, "Failed to export version: " + version, "");
                            cancelExportButton.set_sensitive(false);
                            buttonExportToRedPitaya.set_sensitive(true);
                        });
                        return;
                    }

                    progress += progressStep;
                    Glib::signal_idle().connect_once([this, version, progress]() {
                        detailsPanel.append_log("Exported version: " + version);
                        detailsPanel.set_progress(progress);
                    });
                }

                Glib::signal_idle().connect_once([this]() {
                    detailsPanel.set_status("Export complete");
                    detailsPanel.set_progress(1.0);
                    showInfoDialog(this, "Files and directories exported successfully!");
                    cancelExportButton.set_sensitive(false);
                    buttonExportToRedPitaya.set_sensitive(true);
                });
            }).detach();
        });

        selectDialog->show_all(); });

    dirDialog->show_all();
}

void Vue::showErrorDialog(Gtk::Widget *parent, const std::string &title, const std::string &text)
{
    Gtk::Window *window = dynamic_cast<Gtk::Window *>(parent);
    auto dialog = new Gtk::MessageDialog(*window, title, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, false);
    if (!text.empty())
        dialog->set_secondary_text(text);
    dialog->signal_response().connect([dialog](int)
                                      {
        dialog->hide();
        delete dialog; });
    dialog->show_all();
}

void Vue::showInfoDialog(Gtk::Widget *parent, const std::string &message)
{
    Gtk::Window *window = dynamic_cast<Gtk::Window *>(parent);
    auto dialog = new Gtk::MessageDialog(*window, message, false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, false);
    dialog->signal_response().connect([dialog](int)
                                      {
        dialog->hide();
        delete dialog; });
    dialog->show_all();
}

void Vue::onHelpClicked()
{
    auto dialog = new Gtk::Dialog("Help & Dependencies", false);
    dialog->set_resizable(false); 
    dialog->set_position(Gtk::WIN_POS_CENTER);

    dialog->set_modal(false);
    dialog->add_button("OK", Gtk::RESPONSE_OK);

    auto label = Gtk::make_managed<Gtk::Label>(
        "To export to Red Pitaya, ensure you have the required dependencies installed:\n\n"
        "For Ubuntu/Debian:\n"
        "    sudo apt update\n"
        "    sudo apt install sshpass\n\n"
        "For Arch Linux:\n"
        "    sudo pacman -S sshpass\n\n"
        "For macOS (Homebrew):\n"
        "    brew install hudochenkov/sshpass/sshpass\n\n"
        "Once installed, restart the program and retry the export.");
    label->set_line_wrap(true);
    label->set_margin_top(10);
    label->set_margin_bottom(10);
    label->set_margin_start(10);
    label->set_margin_end(10);

    dialog->get_content_area()->pack_start(*label, Gtk::PACK_EXPAND_WIDGET);
    dialog->show_all();
    dialog->resize(1, 1);

    dialog->signal_response().connect([dialog](int response)
                                      {
        if (response == Gtk::RESPONSE_OK) {
            dialog->hide();
            delete dialog;
        } });
}

void Vue::onQuitClicked()
{
    hide();
}

void Vue::onCheckShowDetailsClicked()
{
    bool show = checkShowDetails.get_active();
    if (show)
        detailsPanel.show();
    else
        detailsPanel.hide();

    resize(1, 1);
    queue_resize();
}
