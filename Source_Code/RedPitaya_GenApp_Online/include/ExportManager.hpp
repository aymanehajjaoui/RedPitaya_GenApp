/*ExportManager.hpp*/

#pragma once

#include <gtkmm.h>
#include <filesystem>
#include <string>

class ExportManager
{
public:
    static bool exportLocally(const std::string &modelFolder, const std::string &genFilesDir);
    static bool exportToRedPitaya(const std::string &modelFolder, const std::string &genFilesDir, const std::string &hostname, const std::string &password, const std::string &targetDirectory);
};
