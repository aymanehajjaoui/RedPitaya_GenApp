/*ExportLocalHandler.cpp*/

#include "buttonsHandler/ExportLocalHandler.hpp"
#include "Utility/ExportManager.hpp"
#include "Utility/SelectDialog.hpp"

namespace ExportLocalHandler
{

    void handle(Gtk::Window* parentWindow,
        Gtk::Button& buttonExportLocally,
        Gtk::Button& cancelExportButton,
        const std::string& modelFolder,
        std::atomic<bool>& cancelExportFlag,
        DetailsPanel& detailsPanel)
    {
        buttonExportLocally.set_sensitive(false);

        auto selectDialog = new SelectDialog();
        selectDialog->set_modal(false);
        selectDialog->set_position(Gtk::WIN_POS_CENTER);

        selectDialog->signal_response().connect([parentWindow, &buttonExportLocally, &cancelExportButton, &detailsPanel, &modelFolder, &cancelExportFlag, selectDialog](int response)
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
            folderDialog->set_transient_for(*parentWindow);
            folderDialog->set_modal(false);
            folderDialog->set_resizable(true);
            folderDialog->set_position(Gtk::WIN_POS_CENTER);
            folderDialog->set_default_size(600, 400);
            folderDialog->add_button("_Cancel", Gtk::RESPONSE_CANCEL);
            folderDialog->add_button("_OK", Gtk::RESPONSE_OK);

            folderDialog->signal_response().connect([parentWindow, &buttonExportLocally, &cancelExportButton, &detailsPanel, &modelFolder, selectedVersions, &cancelExportFlag, folderDialog](int response)
            {
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
                
                cancelExportButton.set_sensitive(false);

                detailsPanel.set_status(allSucceeded ? "Export completed" : "Export completed with errors");

                auto resultDialog = new Gtk::MessageDialog(*parentWindow,
                                                allSucceeded ? "Export completed successfully." : "Some versions failed to export.",
                                                false,
                                                allSucceeded ? Gtk::MESSAGE_INFO : Gtk::MESSAGE_WARNING);
                resultDialog->signal_response().connect([resultDialog](int)
                {
                    resultDialog->hide();
                    delete resultDialog;
                });
                resultDialog->show_all();

                buttonExportLocally.set_sensitive(true);
            });

            folderDialog->show_all(); });

        selectDialog->show_all();
    }
}
