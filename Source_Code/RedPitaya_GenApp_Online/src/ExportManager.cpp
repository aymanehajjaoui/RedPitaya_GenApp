/*ExportManager.cpp*/

#include "ExportManager.hpp"
#include "SSHManager.hpp"
#include <fstream>
#include <regex>
#include <sstream>
#include <cstdlib>
#include <unordered_map>

namespace fs = std::filesystem;


const std::unordered_map<std::string, std::string> ExportManager::versionGitLinks = {
    {"threads_mutex", "https:
    {"threads_sem", "https:
    {"process_mutex", "https:
    {"process_sem", "https:

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
    std::regex staticRegex(R"(\bstatic\b\s*)");
    std::string modified = std::regex_replace(content, staticRegex, "");

    std::ofstream out(modelCPath);
    out << modified;
    out.close();
}

bool ExportManager::cloneVersionFromGit(const std::string &version, const std::string &destination)
{
    auto it = versionGitLinks.find(version);
    if (it == versionGitLinks.end())
        return false;

    std::string command = "git clone --depth=1 " + it->second + " " + destination + " > /dev/null 2>&1";
    if (std::system(command.c_str()) != 0)
        return false;

    
    fs::remove_all(fs::path(destination) / ".git");

    return true;
}

bool ExportManager::exportLocally(const std::string &modelFolder, const std::vector<std::string> &versions)
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
        for (const auto &version : versions)
        {
            fs::path versionDstPath = fs::path(targetFolder) / version;

            
            if (!cloneVersionFromGit(version, versionDstPath.string()))
            {
                Gtk::MessageDialog errorDialog(dialog, "Failed to clone version: " + version, false, Gtk::MESSAGE_ERROR);
                errorDialog.run();
                return false;
            }

            
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
        Gtk::MessageDialog errorDialog(dialog, std::string("Error: ") + e.what(), false, Gtk::MESSAGE_ERROR);
        errorDialog.run();
        return false;
    }
}

bool ExportManager::exportToRedPitaya(const std::string &modelFolder, const std::vector<std::string> &versions,
                                      const std::string &hostname, const std::string &password, const std::string &targetDirectory)
{
    if (!SSHManager::create_remote_directory(hostname, password, targetDirectory))
        return false;

    for (const auto &version : versions)
    {
        std::string remoteVersionDir = targetDirectory + "/" + version;
        if (!SSHManager::create_remote_directory(hostname, password, remoteVersionDir))
            return false;

        std::string remoteModelDir = remoteVersionDir + "/model";
        std::string tempModelDir = "/tmp/export_temp_model_" + version;
        std::string tempCodeDir = "/tmp/export_code_" + version;

        fs::remove_all(tempModelDir);
        fs::remove_all(tempCodeDir);
        fs::create_directories(tempModelDir);

        
        fs::copy(modelFolder, tempModelDir, fs::copy_options::recursive | fs::copy_options::overwrite_existing);

        
        if (version == "threads_mutex" || version == "threads_sem")
        {
            removeStaticFromModelC(tempModelDir);
        }

        
        if (!cloneVersionFromGit(version, tempCodeDir))
            return false;

        
        if (!SSHManager::scp_transfer(hostname, password, tempModelDir, remoteModelDir))
            return false;

        for (const auto &file : fs::directory_iterator(tempCodeDir))
        {
            std::string remoteFilePath = remoteVersionDir + "/" + file.path().filename().string();
            if (!SSHManager::scp_transfer(hostname, password, file.path().string(), remoteFilePath))
                return false;
        }
    }

    return true;
}
