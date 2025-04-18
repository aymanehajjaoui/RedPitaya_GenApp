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

    std::regex staticRegex(R"(\bstatic\b\s*)");
    std::string modified = std::regex_replace(content, staticRegex, "");

    std::ofstream out(modelCPath);
    out << modified;
    out.close();
}

bool ExportManager::exportLocally(const std::string &modelFolder,
                                  const std::string &genFilesDir,
                                  const std::string &version,
                                  const std::string &targetFolder,
                                  const std::atomic<bool> &cancelExportFlag)
{
    try
    {
        if (cancelExportFlag.load())
            return false;

        if (!fs::exists(targetFolder))
            fs::create_directories(targetFolder);

        fs::path versionDstPath = fs::path(targetFolder) / version;
        fs::path versionSrcPath = fs::path(genFilesDir) / version;

        if (cancelExportFlag.load())
            return false;
        fs::create_directories(versionDstPath);

        fs::copy(versionSrcPath, versionDstPath,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing);

        if (cancelExportFlag.load())
            return false;
        fs::copy(modelFolder, versionDstPath / "model",
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing);

        if ((version == "threads_mutex" || version == "threads_sem") && !cancelExportFlag.load())
            removeStaticFromModelC((versionDstPath / "model").string());

        return !cancelExportFlag.load();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Export failed for version " << version << ": " << e.what() << std::endl;
        return false;
    }
}

bool ExportManager::exportSingleVersionToRedPitaya(const std::string &modelFolder,
                                                   const std::string &genFilesDir,
                                                   const std::string &version,
                                                   const std::string &hostname,
                                                   const std::string &password,
                                                   const std::string &targetDirectory,
                                                   const std::atomic<bool> &cancelExportFlag)
{
    if (cancelExportFlag.load())
        return false;

    std::string remoteVersionDir = targetDirectory + "/" + version;
    if (!SSHManager::create_remote_directory(hostname, password, remoteVersionDir))
        return false;

    std::string remoteModelDir = remoteVersionDir + "/model";
    std::string tempModelDir = "/tmp/export_temp_model";

    fs::remove_all(tempModelDir);
    fs::create_directories(tempModelDir);

    if (cancelExportFlag.load())
        return false;
    fs::copy(modelFolder, tempModelDir,
             fs::copy_options::recursive | fs::copy_options::overwrite_existing);

    if ((version == "threads_mutex" || version == "threads_sem") && !cancelExportFlag.load())
        removeStaticFromModelC(tempModelDir);

    if (cancelExportFlag.load())
        return false;
    if (!SSHManager::scp_transfer(hostname, password, tempModelDir, remoteModelDir))
        return false;

    fs::path src = fs::path(genFilesDir) / version;

    if (fs::is_directory(src))
    {
        for (const auto &file : fs::directory_iterator(src))
        {
            if (cancelExportFlag.load())
                return false;

            std::string remoteFilePath = remoteVersionDir + "/" + file.path().filename().string();
            if (!SSHManager::scp_transfer(hostname, password, file.path().string(), remoteFilePath))
                return false;
        }
    }
    else if (fs::is_regular_file(src))
    {
        if (cancelExportFlag.load())
            return false;
        if (!SSHManager::scp_transfer(hostname, password, src.string(), remoteVersionDir))
            return false;
    }

    return !cancelExportFlag.load();
}
