/*ExportManager.hpp*/

#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <atomic>

class ExportManager
{
public:
    static const std::unordered_map<std::string, std::string> versionGitLinks;

    static bool cloneVersionFromGit(const std::string &version, const std::string &destination);

    static bool exportLocally(const std::string &modelFolder,
                              const std::string &version,
                              const std::string &targetFolder,
                              const std::atomic<bool> &cancelExportFlag);

    static bool exportSingleVersionToRedPitaya(const std::string &modelFolder,
                                               const std::string &version,
                                               const std::string &hostname,
                                               const std::string &password,
                                               const std::string &targetDirectory,
                                               const std::atomic<bool> &cancelExportFlag);

private:
    static void removeStaticFromModelC(const std::string &versionPath);
};
