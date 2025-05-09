/*FileManager.cpp*/

#include "Utility/FileManager.hpp"

bool FileManager::isValidQualiaModel(const std::string& folderPath)
{
    std::filesystem::path modelPath(folderPath);

    std::filesystem::path fullModelPath = modelPath / "full_model.h";
    std::filesystem::path modelCPath = modelPath / "model.c";
    std::filesystem::path includeModelPath = modelPath / "include" / "model.h";

    return std::filesystem::exists(fullModelPath) &&
           std::filesystem::exists(modelCPath) &&
           std::filesystem::exists(includeModelPath);
}
