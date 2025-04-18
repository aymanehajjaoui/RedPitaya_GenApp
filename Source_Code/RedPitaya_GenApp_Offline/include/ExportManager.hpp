/*ExportManager.hpp*/

#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <atomic>

class ExportManager
{
public:
    static bool exportLocally(const std::string &modelFolder,
                              const std::string &genFilesDir,
                              const std::string &version,
                              const std::string &targetFolder,
                              const std::atomic<bool> &cancelExportFlag);

    static bool exportSingleVersionToRedPitaya(const std::string &modelFolder,
                                               const std::string &genFilesDir,
                                               const std::string &version,
                                               const std::string &hostname,
                                               const std::string &password,
                                               const std::string &targetDirectory,
                                               const std::atomic<bool> &cancelExportFlag);

private:
    static void removeStaticFromModelC(const std::string &versionPath);
};

