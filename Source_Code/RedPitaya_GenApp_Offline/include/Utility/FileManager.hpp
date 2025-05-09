/*FileManager.hpp*/

#pragma once

#include <string>
#include <filesystem>

class FileManager
{
public:
    static bool isValidQualiaModel(const std::string& folderPath);
};