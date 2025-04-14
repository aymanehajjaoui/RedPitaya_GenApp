/*ExportManager.hpp*/

#pragma once

#include <gtkmm.h>
#include <filesystem>
#include <string>
#include <fstream>
#include <sstream>
#include <regex>

class ExportManager
{
public:
    static const std::unordered_map<std::string, std::string> versionGitLinks;
    static bool cloneVersionFromGit(const std::string &version, const std::string &destination);

    static void removeStaticFromModelC(const std::string &versionPath);
    static bool exportLocally(const std::string &modelFolder);
    static bool exportLocally(const std::string &modelFolder, const std::vector<std::string> &versions);

    static bool exportToRedPitaya(const std::string &modelFolder, const std::string &hostname, const std::string &password, const std::string &targetDirectory);
    static bool exportToRedPitaya(const std::string &modelFolder, const std::vector<std::string> &versions,
                                  const std::string &hostname, const std::string &password, const std::string &targetDirectory);
};
