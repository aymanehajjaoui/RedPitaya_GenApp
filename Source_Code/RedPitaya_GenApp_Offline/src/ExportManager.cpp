/*ExportManager.cpp*/

#include "ExportManager.hpp"
#include "SSHManager.hpp"

namespace fs = std::filesystem;

bool ExportManager::exportLocally(const std::string &modelFolder, const std::string &genFilesDir)
{
    Gtk::FileChooserDialog dialog("Choose the target folder", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_OK", Gtk::RESPONSE_OK);

    std::string targetFolder;
    if (dialog.run() == Gtk::RESPONSE_OK)
    {
        targetFolder = dialog.get_filename();
    }

    if (modelFolder.empty() || targetFolder.empty())
    {
        Gtk::MessageDialog errorDialog(dialog, "Please select a model and target location.", false, Gtk::MESSAGE_ERROR);
        errorDialog.run();
        return false;
    }

    try
    {
        if (!fs::exists(targetFolder))
        {
            fs::create_directories(targetFolder);
        }

        fs::copy(modelFolder, fs::path(targetFolder) / "model", fs::copy_options::recursive);
        
        for (const auto &entry : fs::directory_iterator(genFilesDir))
        {
            fs::copy(entry.path(), targetFolder / entry.path().filename(), fs::copy_options::recursive);
        }

        Gtk::MessageDialog successDialog(dialog, "Model exported successfully!", false, Gtk::MESSAGE_INFO);
        successDialog.run();
        return true;
    }
    catch (const std::exception &e)
    {
        Gtk::MessageDialog errorDialog(dialog, std::string("Error copying files: ") + e.what(), false, Gtk::MESSAGE_ERROR);
        errorDialog.run();
        return false;
    }
}

bool ExportManager::exportToRedPitaya(const std::string &modelFolder, const std::string &genFilesDir, const std::string &hostname, const std::string &password, const std::string &targetDirectory)
{
    bool dirCreated = SSHManager::create_remote_directory(hostname, password, targetDirectory);
    if (!dirCreated)
    {
        return false;
    }

    std::string targetPath = targetDirectory + "/model";
    bool scpSuccess = SSHManager::scp_transfer(hostname, password, modelFolder, targetPath);
    if (!scpSuccess)
    {
        return false;
    }

    for (const auto &entry : fs::directory_iterator(genFilesDir))
    {
        std::string targetFilePath = targetDirectory + "/" + entry.path().filename().string();
        if (fs::is_regular_file(entry))
        {
            if (!SSHManager::scp_transfer(hostname, password, entry.path().string(), targetFilePath))
            {
                return false;
            }
        }
        else if (fs::is_directory(entry))
        {
            std::string targetDirPath = targetDirectory + "/" + entry.path().filename().string();
            if (!SSHManager::create_remote_directory(hostname, password, targetDirPath))
            {
                return false;
            }

            for (const auto &subEntry : fs::directory_iterator(entry))
            {
                std::string targetSubFilePath = targetDirPath + "/" + subEntry.path().filename().string();
                if (!SSHManager::scp_transfer(hostname, password, subEntry.path().string(), targetSubFilePath))
                {
                    return false;
                }
            }
        }
    }

    return true;
}
