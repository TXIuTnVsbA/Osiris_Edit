#include <fstream>
#include <ShlObj.h>
#include "../utils/json/json.h"
#include "config.h"
c_config g_config{};
void c_config::init() noexcept {
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

std::filesystem::path c_config::get_path() noexcept {
    if (!std::filesystem::is_directory(_path)) {
        std::filesystem::remove(_path);
        std::filesystem::create_directory(_path);
    }
    return _path;
}

void c_config::load(size_t id) noexcept {

}

void c_config::save(size_t id) const noexcept {


}

void c_config::add(const char* name) noexcept
{
    if (*name && std::find(std::cbegin(configs), std::cend(configs), name) == std::cend(configs))
        configs.emplace_back(name);
}

void c_config::remove(size_t id) noexcept
{
    std::filesystem::remove(path / (const char8_t*)configs[id].c_str());
    configs.erase(configs.cbegin() + id);
}

void c_config::rename(size_t item, const char* newName) noexcept
{
    std::filesystem::rename(path / (const char8_t*)configs[item].c_str(), path / (const char8_t*)newName);
    configs[item] = newName;
}

void c_config::reset() noexcept
{

}

void c_config::refresh() noexcept
{

}