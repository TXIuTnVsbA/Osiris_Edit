#pragma once
#include <array>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <algorithm>
class c_config {
public:
	std::unordered_map<std::string, bool> b;
	std::unordered_map<std::string, int> i;
	std::unordered_map<std::string, float> f;
	std::unordered_map<std::string, float[4]> c;
	std::unordered_map<std::string, std::unordered_map<int, bool>> m_b;
	std::unordered_map<std::string, std::unordered_map<int, int>> m_i;
	std::unordered_map<std::string, std::unordered_map<int, float>> m_f;
	std::unordered_map<std::string, std::unordered_map<int, float[4]>> m_c;

	void init() noexcept;
    void load(size_t) noexcept;
    void save(size_t) const noexcept;
    void add(const char*) noexcept;
    void remove(size_t) noexcept;
    void rename(size_t, const char*) noexcept;
    void reset() noexcept;
    void refresh() noexcept;

    constexpr auto& getConfigs() noexcept
    {
        return configs;
    }

    std::filesystem::path get_path() noexcept;
private:
    std::filesystem::path _path;
    std::filesystem::path path;
    std::vector<std::string> configs;
};
extern c_config g_config;