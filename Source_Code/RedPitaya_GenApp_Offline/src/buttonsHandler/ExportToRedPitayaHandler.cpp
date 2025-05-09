/*ExportToRedPitayaHandler.cpp*/

#include "buttonsHandler/ExportToRedPitayaHandler.hpp"
#include "Utility/ExportManager.hpp"
#include "Utility/ConnectionManager.hpp"
#include "Utility/SelectDialog.hpp"

namespace ExportToRedPitayaHandler
{
    static void showErrorDialog(Gtk::Window* parent, const std::string& title, const std::string& text)
    {
        auto dialog = new Gtk::MessageDialog(*parent, title, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, false);
        if (!text.empty())
            dialog->set_secondary_text(text);
        dialog->signal_response().connect([dialog](int)
        {
            dialog->hide();
            delete dialog;
        });
        dialog->show_all();
    }

    static void showInfoDialog(Gtk::Window* parent, const std::string& message)
    {
        auto dialog = new Gtk::MessageDialog(*parent, message, false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, false);
        dialog->signal_response().connect([dialog](int)
        {
            dialog->hide();
            delete dialog;
        });
        dialog->show_all();
    }

    void handle(Gtk::Window* parentWindow,
                Gtk::Button& buttonExportToRedPitaya,
                Gtk::Button& buttonConnectRedPitaya,
                Gtk::Button& cancelExportButton,
                const std::string& modelFolder,
                const std::string& genFilesDir,
                const std::string& redpitayaHost,
                const std::string& redpitayaPassword,
                const std::string& redpitayaPrivateKeyPath,
                std::atomic<bool>& cancelExportFlag,
                DetailsPanel& detailsPanel)
    {
        buttonExportToRedPitaya.set_sensitive(false);

        if (!ConnectionManager::pingHost(redpitayaHost) ||
            !ConnectionManager::isSSHConnectionAlive(redpitayaHost, redpitayaPassword, redpitayaPrivateKeyPath))
        {
            buttonConnectRedPitaya.set_sensitive(true);
            buttonConnectRedPitaya.set_label("Connect to RedPitaya");

            showErrorDialog(parentWindow, "SSH connection to RedPitaya seems lost.",
                            "Please reconnect using the 'Connect to RedPitaya' button.");
            buttonExportToRedPitaya.set_sensitive(true);
            return;
        }

        auto selectDialog = new SelectDialog();
        selectDialog->set_modal(false);
        selectDialog->set_position(Gtk::WIN_POS_CENTER);

        selectDialog->signal_response().connect([parentWindow, &buttonExportToRedPitaya, &cancelExportButton, &cancelExportFlag,
                                                 modelFolder, genFilesDir, redpitayaHost, redpitayaPassword, redpitayaPrivateKeyPath,
                                                 &detailsPanel, selectDialog](int selResp)
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
                    showErrorDialog(parentWindow, "No versions selected.",
                                    "Please select at least one version to export.");
                }
                buttonExportToRedPitaya.set_sensitive(true);
                return;
            }

            auto dirDialog = new Gtk::Dialog("Enter target folder on RedPitaya", false);
            dirDialog->set_transient_for(*parentWindow);
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

            dirDialog->signal_response().connect([parentWindow, dirDialog, entry, selectedVersions, &buttonExportToRedPitaya, &cancelExportButton,
                                                  &cancelExportFlag, modelFolder, genFilesDir, redpitayaHost, redpitayaPassword, redpitayaPrivateKeyPath,
                                                  &detailsPanel](int dirResp)
            {
                std::string targetDirectory;
                if (dirResp == Gtk::RESPONSE_OK)
                    targetDirectory = entry->get_text();

                dirDialog->hide();
                delete dirDialog;

                if (targetDirectory.empty())
                {
                    showErrorDialog(parentWindow, "Please enter a directory path!", "");
                    buttonExportToRedPitaya.set_sensitive(true);
                    return;
                }

                cancelExportButton.set_sensitive(true);
                cancelExportFlag = false;

                std::thread([parentWindow, selectedVersions, targetDirectory, modelFolder, genFilesDir, redpitayaHost, redpitayaPassword,
                             redpitayaPrivateKeyPath, &detailsPanel, &cancelExportButton, &buttonExportToRedPitaya, &cancelExportFlag]()
                {
                    Glib::signal_idle().connect_once([&detailsPanel]() {
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
                            Glib::signal_idle().connect_once([&detailsPanel, &cancelExportButton, &buttonExportToRedPitaya]() {
                                detailsPanel.append_log("Export canceled by user.");
                                detailsPanel.set_status("Canceled");
                                detailsPanel.set_progress(0.0);
                                cancelExportButton.set_sensitive(false);
                                buttonExportToRedPitaya.set_sensitive(true);
                            });
                            return;
                        }

                        bool ok = ExportManager::exportSingleVersionToRedPitaya(
                            modelFolder, genFilesDir, version,
                            redpitayaHost, redpitayaPassword, redpitayaPrivateKeyPath,
                            targetDirectory, cancelExportFlag);

                        if (!ok)
                        {
                            Glib::signal_idle().connect_once([&detailsPanel, version, &cancelExportButton, &buttonExportToRedPitaya, parentWindow]() {
                                detailsPanel.append_log("Failed to export version: " + version);
                                detailsPanel.set_status("Export failed");
                                detailsPanel.set_progress(0.0);
                                showErrorDialog(parentWindow, "Failed to export version: " + version, "");
                                cancelExportButton.set_sensitive(false);
                                buttonExportToRedPitaya.set_sensitive(true);
                            });
                            return;
                        }

                        progress += progressStep;
                        Glib::signal_idle().connect_once([&detailsPanel, version, progress]() {
                            detailsPanel.append_log("Exported version: " + version);
                            detailsPanel.set_progress(progress);
                        });
                    }

                    cancelExportButton.set_sensitive(false);

                    Glib::signal_idle().connect_once([&detailsPanel, &cancelExportButton, &buttonExportToRedPitaya, parentWindow]() {
                        detailsPanel.set_status("Export complete");
                        detailsPanel.set_progress(1.0);
                        showInfoDialog(parentWindow, "Files and directories exported successfully!");
                        cancelExportButton.set_sensitive(false);
                        buttonExportToRedPitaya.set_sensitive(true);
                    });

                }).detach();
            });

            dirDialog->show_all();
        });

        selectDialog->show_all();
    }
}
