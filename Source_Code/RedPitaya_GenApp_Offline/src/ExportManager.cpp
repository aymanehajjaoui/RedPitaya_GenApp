/*ExportManager.cpp*/

#include "ExportManager.hpp"
#include "SSHManager.hpp"

namespace fs = std::filesystem;

void ExportManager::removeStaticFromModelC(const std::string &versionPath)
{
    fs::path modelCPath = fs::path(versionPath) / "model.c";
    if (!fs::exists(modelCPath))
        return;

    std::ifstream in(modelCPath);
    std::stringstream buffer;
    buffer << in.rdbuf();
    in.close();

    std::string content = buffer.str();

    // Remove all 'static' keywords cleanly
    std::regex staticRegex(R"(\bstatic\b\s*)");
    std::string modified = std::regex_replace(content, staticRegex, "");

    std::ofstream out(modelCPath);
    out << modified;
    out.close();
}

bool ExportManager::exportLocally(const std::string &modelFolder, const std::string &genFilesDir, const std::vector<std::string> &versions)
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

        for (const auto &version : versions)
        {
            fs::path versionDstPath = fs::path(targetFolder) / version;
            fs::path versionSrcPath = fs::path(genFilesDir) / version;

            fs::create_directories(versionDstPath);
            fs::copy(versionSrcPath, versionDstPath, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
            fs::copy(modelFolder, versionDstPath / "model", fs::copy_options::recursive | fs::copy_options::overwrite_existing);

            if (version == "threads_mutex" || version == "threads_sem")
            {
                removeStaticFromModelC((versionDstPath / "model").string());
            }
        }

        Gtk::MessageDialog successDialog(dialog, "Selected versions exported successfully!", false, Gtk::MESSAGE_INFO);
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

bool ExportManager::exportToRedPitaya(const std::string &modelFolder, const std::string &genFilesDir, const std::vector<std::string> &versions,
                                      const std::string &hostname, const std::string &password, const std::string &targetDirectory)
{
    if (!SSHManager::create_remote_directory(hostname, password, targetDirectory))
        return false;

    for (const auto &version : versions)
    {
        fs::path src = fs::path(genFilesDir) / version;
        std::string remoteVersionDir = targetDirectory + "/" + version;

        if (!SSHManager::create_remote_directory(hostname, password, remoteVersionDir))
            return false;

        std::string remoteModelDir = remoteVersionDir + "/model";
        std::string tempModelDir = "/tmp/export_temp_model";

        // Clean/create temporary copy of model directory
        fs::remove_all(tempModelDir);
        fs::create_directories(tempModelDir);
        fs::copy(modelFolder, tempModelDir, fs::copy_options::recursive | fs::copy_options::overwrite_existing);

        if (version == "threads_mutex" || version == "threads_sem")
        {
            removeStaticFromModelC(tempModelDir);
        }

        if (!SSHManager::scp_transfer(hostname, password, tempModelDir, remoteModelDir))
            return false;

        if (fs::is_directory(src))
        {
            for (const auto &file : fs::directory_iterator(src))
            {
                std::string remoteFilePath = remoteVersionDir + "/" + file.path().filename().string();
                if (!SSHManager::scp_transfer(hostname, password, file.path().string(), remoteFilePath))
                    return false;
            }
        }
        else if (fs::is_regular_file(src))
        {
            if (!SSHManager::scp_transfer(hostname, password, src.string(), remoteVersionDir))
                return false;
        }
    }

    return true;
}
