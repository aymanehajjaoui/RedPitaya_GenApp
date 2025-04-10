/*ExportManager.cpp*/

#include "ExportManager.hpp"
#include "SSHManager.hpp"

namespace fs = std::filesystem;

bool ExportManager::exportLocally(const std::string &modelFolder, const std::string & /* genFilesDir */)
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

    const std::string tempClonePath = "/tmp/brainmix_temp";

    try
    {
        // Clean up temp dir if it exists
        if (fs::exists(tempClonePath))
        {
            fs::remove_all(tempClonePath);
        }

        // Clone the repo into the temp directory
        std::string cloneCmd = "git clone https://github.com/aymanehajjaoui/BrainMix_RPCode_Template.git " + tempClonePath;
        int result = std::system(cloneCmd.c_str());

        if (result != 0)
        {
            Gtk::MessageDialog errorDialog(dialog, "Git clone failed.", false, Gtk::MESSAGE_ERROR);
            errorDialog.run();
            return false;
        }

        fs::remove_all(fs::path(tempClonePath) / ".git");
        fs::remove(fs::path(tempClonePath) / ".gitignore");

        // Ensure the target folder exists
        if (!fs::exists(targetFolder))
        {
            fs::create_directories(targetFolder);
        }

        // Copy repo contents into target folder
        for (const auto &entry : fs::directory_iterator(tempClonePath))
        {
            fs::copy(entry.path(), fs::path(targetFolder) / entry.path().filename(), fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        }

        // Copy model folder
        fs::copy(modelFolder, fs::path(targetFolder) / "model", fs::copy_options::recursive | fs::copy_options::overwrite_existing);

        // Cleanup
        fs::remove_all(tempClonePath);

        Gtk::MessageDialog successDialog(dialog, "Model and template exported successfully!", false, Gtk::MESSAGE_INFO);
        successDialog.run();
        return true;
    }
    catch (const std::exception &e)
    {
        Gtk::MessageDialog errorDialog(dialog, std::string("Error during export: ") + e.what(), false, Gtk::MESSAGE_ERROR);
        errorDialog.run();
        return false;
    }
}

bool ExportManager::exportToRedPitaya(const std::string &modelFolder, const std::string & /* genFilesDir */, const std::string &hostname, const std::string &password, const std::string &targetDirectory)
{
    const std::string tempClonePath = "/tmp/brainmix_temp";

    try
    {
        // Clone repo into temp directory
        if (fs::exists(tempClonePath))
        {
            fs::remove_all(tempClonePath);
        }

        std::string cloneCmd = "git clone https://github.com/aymanehajjaoui/BrainMix_RPCode_Template.git " + tempClonePath;
        int result = std::system(cloneCmd.c_str());

        if (result != 0)
        {
            std::cerr << "Git clone failed.\n";
            return false;
        }

        // Remove Git metadata
        fs::remove_all(fs::path(tempClonePath) / ".git");
        fs::remove(fs::path(tempClonePath) / ".gitignore");

        // Create target directory on RedPitaya
        if (!SSHManager::create_remote_directory(hostname, password, targetDirectory))
        {
            return false;
        }

        // Transfer cloned files to RedPitaya
        for (const auto &entry : fs::directory_iterator(tempClonePath))
        {
            std::string targetPath = targetDirectory + "/" + entry.path().filename().string();
            if (fs::is_regular_file(entry))
            {
                if (!SSHManager::scp_transfer(hostname, password, entry.path().string(), targetPath))
                    return false;
            }
            else if (fs::is_directory(entry))
            {
                if (!SSHManager::create_remote_directory(hostname, password, targetPath))
                    return false;

                for (const auto &subEntry : fs::directory_iterator(entry))
                {
                    std::string subTarget = targetPath + "/" + subEntry.path().filename().string();
                    if (!SSHManager::scp_transfer(hostname, password, subEntry.path().string(), subTarget))
                        return false;
                }
            }
        }

        // Transfer the model folder
        std::string modelTargetPath = targetDirectory + "/model";
        if (!SSHManager::scp_transfer(hostname, password, modelFolder, modelTargetPath))
        {
            return false;
        }

        // Clean up local temp directory
        fs::remove_all(tempClonePath);

        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error during export to RedPitaya: " << e.what() << std::endl;
        return false;
    }
}
