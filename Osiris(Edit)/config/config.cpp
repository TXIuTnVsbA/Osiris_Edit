#include <fstream>
#include <ShlObj.h>
#include "../utils/json/json.h"
#include "config.h"
c_config g_config{};
void c_config::init() {
    if (PWSTR pathToDocuments; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pathToDocuments))) {
        _path = pathToDocuments;
        _path /= "Osiris";
        path = _path;
        path /= "Config";
        CoTaskMemFree(pathToDocuments);
    }

    if (!std::filesystem::is_directory(_path)) {
        std::filesystem::remove(_path);
        std::filesystem::create_directory(_path);
    }

    if (!std::filesystem::is_directory(path)) {
        std::filesystem::remove(path);
        std::filesystem::create_directory(path);
    }

    std::transform(std::filesystem::directory_iterator{ path },
        std::filesystem::directory_iterator{ },
        std::back_inserter(configs),
        [](const auto& entry) { return std::string{ (const char*)entry.path().filename().u8string().c_str() }; });
}
std::filesystem::path c_config::get_path() {
    if (!std::filesystem::is_directory(_path)) {
        std::filesystem::remove(_path);
        std::filesystem::create_directory(_path);
    }
    return _path;
}