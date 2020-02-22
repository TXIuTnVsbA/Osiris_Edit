#include <fstream>
#include <functional>
#include <string>
#include <ShlObj.h>
#include <Windows.h>
#include <filesystem>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"

#include "gui.h"
#include "../config/config.h"
#include "../utils/console/console.h"
#include "../sdk/utils/hooks/hooks.h"
#include "../sdk/utils/interfaces/interfaces.h"
#include "../sdk/utils/memory/memory.h"
#include "../sdk/utils/netvars/netvars.h"

#include "../lua/Clua.h"

c_gui g_gui;
constexpr auto windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

void dmt(std::string key) {
	if (GET_BOOL["lua_devmode"] && ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::Text(key.c_str());
		ImGui::EndTooltip();
	}
}

void draw_lua_items(std::string tab, std::string container) {
	for (auto i : lua::menu_items[tab][container]) {
		if (!i.is_visible)
			continue;

		auto type = i.type;
		switch (type)
		{
		case lua::MENUITEM_CHECKBOX:
			if (ImGui::Checkbox(i.label.c_str(), &(GET_BOOL[i.key]))) {
				if (i.callback != sol::nil)
					i.callback(GET_BOOL[i.key]);
			}

			dmt(i.key);
			break;
		case lua::MENUITEM_SLIDERINT:
			if (ImGui::SliderInt(i.label.c_str(), &(GET_INT[i.key]), i.i_min, i.i_max, i.format.c_str())) {
				if (i.callback != sol::nil)
					i.callback(GET_INT[i.key]);
			}

			dmt(i.key);
			break;
		case lua::MENUITEM_SLIDERFLOAT:
			if (ImGui::SliderFloat(i.label.c_str(), &(GET_FLOAT[i.key]), i.f_min, i.f_max, i.format.c_str())) {
				if (i.callback != sol::nil)
					i.callback(GET_FLOAT[i.key]);
			}

			dmt(i.key);
			break;
			/*
		case lua::MENUITEM_KEYBIND:
			if (ImGui::Keybind(i.label.c_str(), &cfg->i[i.key], i.allow_style_change ? &cfg->i[i.key + "style"] : NULL)) {
				if (i.callback != sol::nil)
					i.callback(cfg->i[i.key], cfg->i[i.key + "style"]);
			}

			dmt(i.key + (i.allow_style_change ? " | " + i.key + "style" : ""));
			break;
			*/
		case lua::MENUITEM_TEXT:
			ImGui::Text(i.label.c_str());
			break;

			/*
		case lua::MENUITEM_SINGLESELECT:
			if (ImGui::SingleSelect(i.label.c_str(), &cfg->i[i.key], i.items)) {
				if (i.callback != sol::nil)
					i.callback(cfg->i[i.key]);
			}

			dmt(i.key);
			break;

		case lua::MENUITEM_MULTISELECT:
			if (ImGui::MultiSelect(i.label.c_str(), &cfg->m[i.key], i.items)) {
				if (i.callback != sol::nil)
					i.callback(cfg->m[i.key]);
			}

			dmt(i.key);
			break;
			*/
		case lua::MENUITEM_COLORPICKER:
			if (ImGui::ColorEdit4(i.label.c_str(), GET_COLOR[i.key])) {
				if (i.callback != sol::nil)
					i.callback(GET_COLOR[i.key][0] * 255, GET_COLOR[i.key][1] * 255, GET_COLOR[i.key][2] * 255, GET_COLOR[i.key][3] * 255);
			}

			dmt(i.key);
			break;
		case lua::MENUITEM_BUTTON:
			if (ImGui::Button(i.label.c_str())) {
				if (i.callback != sol::nil)
					i.callback();
			}
			break;
		default:
			break;
		}
	}
}

void draw_config_window(bool* isOpened) {
	ImGui::SetNextWindowSize({ 290.0f, 190.0f });
	ImGui::Columns(2, nullptr, false);
	ImGui::SetColumnOffset(1, 170.0f);

	ImGui::PushItemWidth(500.f);
	constexpr auto& configItems = g_config.getConfigs();
	static int currentConfig = -1;

	if (static_cast<size_t>(currentConfig) >= configItems.size())
		currentConfig = -1;

	static char buffer[16];

	if (ImGui::ListBox("", &currentConfig, [](void* data, int idx, const char** out_text) {
		auto& vector = *static_cast<std::vector<std::string>*>(data);
		*out_text = vector[idx].c_str();
		return true;
		}, &configItems, configItems.size(), 5) && currentConfig != -1)
		strcpy(buffer, configItems[currentConfig].c_str());

		ImGui::PushID(0);
		if (ImGui::InputText("", buffer, IM_ARRAYSIZE(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
			if (currentConfig != -1)
				g_config.rename(currentConfig, buffer);
		}
		ImGui::PopID();
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::PushItemWidth(100.0f);

		if (ImGui::Button("Refresh", { 100.0f, 25.0f }))
			g_config.init();

		if (ImGui::Button("Reset", { 100.0f, 25.0f }))
			g_config.reset();

		if (ImGui::Button("Create config", { 100.0f, 25.0f }))
			g_config.add(buffer);



		if (currentConfig != -1) {
			if (ImGui::Button("Load selected", { 100.0f, 25.0f }))
				g_config.load(currentConfig);
			if (ImGui::Button("Save selected", { 100.0f, 25.0f }))
				g_config.save(currentConfig);
			if (ImGui::Button("Delete selected", { 100.0f, 25.0f }))
				g_config.remove(currentConfig);
		}
		ImGui::PopItemWidth();

}

void draw_lua_window(bool* isOpened) {
	ImGui::SetNextWindowSize({ 290.0f, 190.0f });
	ImGui::Columns(2, nullptr, false);
	ImGui::SetColumnOffset(1, 170.0f);

	ImGui::PushItemWidth(500.f);
	constexpr auto& configItems = lua::scripts;
	static int currentConfig = -1;

	if (static_cast<size_t>(currentConfig) >= configItems.size())
		currentConfig = -1;

	static char buffer[16];

	if (ImGui::ListBox("", &currentConfig, [](void* data, int idx, const char** out_text) {
		auto& vector = *static_cast<std::vector<std::string>*>(data);
		*out_text = vector[idx].c_str();
		return true;
		}, &configItems, configItems.size(), 5) && currentConfig != -1)
		strcpy(buffer, configItems[currentConfig].c_str());

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::PushItemWidth(100.0f);
		if (ImGui::Button("Refresh", { 100.0f, 25.0f }))
		{
			lua::unload_all_scripts();
			lua::refresh_scripts();
			lua::load_script(lua::get_script_id("autorun.lua"));
		}

		if (ImGui::Button("Reload All", { 100.0f, 25.0f }))
			lua::reload_all_scripts();

		if (ImGui::Button("Unload All", { 100.0f, 25.0f }))
			lua::unload_all_scripts();

		if (currentConfig != -1) {
			if (lua::loaded[currentConfig])
			{
				if (ImGui::Button("Unload selected", { 100.0f, 25.0f }))
				{
					lua::unload_script(currentConfig);
				}
			}
			else {
				if (ImGui::Button("Load selected", { 100.0f, 25.0f }))
				{
					lua::load_script(currentConfig);
				}
			}

		}
		ImGui::PopItemWidth();
}

void draw_console_window(bool* isOpened) {
	char InputBuf[512] = {};
	ImGui::PushID(0);
	if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
		try
		{
			bool tmp = luaL_dostring(lua::g_lua_state, InputBuf);
			if (tmp) {
				std::printf("lua::Error: %s\n", lua_tostring(lua::g_lua_state, -1));
			}
		}
		catch (const std::exception & Error)
		{
			std::printf("std::Error: %s\n", Error.what());
			//std::printf("lua::Error: %s\n", lua_tostring(g_lua_state, 0));
			//g_console.log("std::Error: %s", Error.what());
			//g_console.log("lua::Error: %s", lua_tostring(g_lua_state, 0));
		}
	}
	ImGui::PopID();

	std::string OutputBuffer;
	if (OutputBuffer.size() >= OutputBuffer.max_size() - 1) {
		OutputBuffer.clear();
	}
	if (sizeof(g_console.out_buf) >= BLOCK_SIZE - 1) {
		g_console.out_buf[BLOCK_SIZE] = {};
	}
	for (int i = 0; i < BLOCK_SIZE; i++) {
		if (g_console.out_buf[i] != '\0') {
			OutputBuffer.append(1, g_console.out_buf[i]);
		}
		else {
			break;
		}
	}
	ImGui::PushID(1);
	ImGui::TextUnformatted(OutputBuffer.c_str());
	ImGui::PopID();
}


void c_gui::init() noexcept {
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(FindWindowW(L"Valve001", NULL));

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();

	style.ScrollbarSize = 9.0f;
	
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;

	if (PWSTR pathToFonts; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &pathToFonts))) {
		const std::filesystem::path path{ pathToFonts };
		CoTaskMemFree(pathToFonts);

		static constexpr ImWchar ranges[]{ 0x0020, 0xFFFF, 0 };
		fonts.tahoma = io.Fonts->AddFontFromFileTTF((path / "tahoma.ttf").string().c_str(), 16.0f, nullptr, ranges);
		fonts.segoeui = io.Fonts->AddFontFromFileTTF((path / "segoeui.ttf").string().c_str(), 16.0f, nullptr, ranges);
		//io.FontDefault = io.Fonts->AddFontFromFileTTF((path / "Deng.ttf").string().c_str(), 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
		fonts.Deng = io.Fonts->AddFontFromFileTTF((path / "Deng.ttf").string().c_str(), 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
	}
}

void c_gui::render() noexcept {
	//ImGui::SetNextWindowSize({ GET_FLOAT[g_config.gui.w], GET_FLOAT[g_config.gui.h] });
	//ImGui::Begin("Osiris", nullptr, windowFlags | ImGuiWindowFlags_NoTitleBar);
	renderMenuBar();
	//features
	//renderConfigWindow();
	//renderLuaWindow();
	//renderConsoleWindow();
	renderLuaItemsWindow();
	//ImGui::End();
}

void c_gui::renderMenuBar() noexcept {
	/*
	if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_Reorderable)) {
		for (auto tab : GET_STRINGS[g_config.gui.tab_name]) {
			auto tab_name = tab.second.c_str();
			if (ImGui::BeginTabItem(tab_name)) {
				GET_BOOLS_MAP[g_config.gui.tab_bool][tab_name] = true;
				ImGui::EndTabItem();
			}
			else {
				GET_BOOLS_MAP[g_config.gui.tab_bool][tab_name] = false;
			}
		}
		ImGui::EndTabBar();
	}
	*/
	if (ImGui::BeginMainMenuBar()) {
		for (auto tab : GET_STRINGS[g_config.gui.tab_name]) {
			auto tab_name = tab.second.c_str();
			//ImGui::Checkbox(tab_name, &(GET_BOOLS_MAP[g_config.gui.tab_bool][tab_name]));
			ImGui::MenuItem(tab_name, nullptr, &(GET_BOOLS_MAP[g_config.gui.tab_bool][tab_name]));
		}
		ImGui::EndMainMenuBar();
	}
}

void c_gui::renderLuaItemsWindow() noexcept {
	auto tabs = GET_STRINGS[g_config.gui.tab_name];
	for (auto tab : tabs) {
		auto tab_name = tab.second;
		auto tab_name_str = tab_name.c_str();
		bool* window = &(GET_BOOLS_MAP[g_config.gui.tab_bool][tab_name]);
		if (!(*window))
			continue;

		ImGui::Begin(tab_name_str, window, windowFlags);

		if (tab_name == "Config")
			draw_config_window(window);
		else if (tab_name == "Lua")
			draw_lua_window(window);
		else if (tab_name == "Console")
			draw_console_window(window);

		float w = GET_FLOATS_MAP[g_config.gui.w][tab_name];
		float h = GET_FLOATS_MAP[g_config.gui.h][tab_name];
		ImGui::SetWindowSize(ImVec2(w, h));

		int container_count = GET_INTS_MAP[g_config.gui.container_count][tab_name];
		if (container_count > 0) {
			for (int i = 0; i < container_count; i++) {
				char buffer[8] = { 0 }; _itoa(i, buffer, 10);
				draw_lua_items(tab_name, buffer);
			}
		}
		ImGui::End();
	}
}

/*
void c_gui::renderConfigWindow() noexcept {
	if (GET_BOOLS_MAP[g_config.gui.tab_bool]["Config"]) {
		ImGui::Columns(2, nullptr, false);
		//ImGui::SetColumnOffset(1, 170.0f);

		ImGui::PushItemWidth(500.f);
		constexpr auto& configItems = g_config.getConfigs();
		static int currentConfig = -1;

		if (static_cast<size_t>(currentConfig) >= configItems.size())
			currentConfig = -1;

		static char buffer[16];

		if (ImGui::ListBox("", &currentConfig, [](void* data, int idx, const char** out_text) {
			auto& vector = *static_cast<std::vector<std::string>*>(data);
			*out_text = vector[idx].c_str();
			return true;
			}, &configItems, configItems.size(), 5) && currentConfig != -1)
			strcpy(buffer, configItems[currentConfig].c_str());

			ImGui::PushID(0);
			if (ImGui::InputText("", buffer, IM_ARRAYSIZE(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
				if (currentConfig != -1)
					g_config.rename(currentConfig, buffer);
			}
			ImGui::PopID();
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::PushItemWidth(100.0f);

			if (ImGui::Button("Create config", { 100.0f, 25.0f }))
				g_config.add(buffer);

			if (ImGui::Button("Reset config", { 100.0f, 25.0f }))
				g_config.reset();

			if (currentConfig != -1) {
				if (ImGui::Button("Load selected", { 100.0f, 25.0f }))
					g_config.load(currentConfig);
				if (ImGui::Button("Save selected", { 100.0f, 25.0f }))
					g_config.save(currentConfig);
				if (ImGui::Button("Delete selected", { 100.0f, 25.0f }))
					g_config.remove(currentConfig);
			}
			ImGui::PopItemWidth();
			//ImGui::End();
	}
}

void c_gui::renderLuaWindow() noexcept {

}

void c_gui::renderConsoleWindow() noexcept {

}

void c_gui::renderLuaItems() noexcept {
	for (auto tab : GET_STRINGS[g_config.gui.tab_name]) {
		auto tab_name = tab.second;
		if (GET_BOOLS_MAP[g_config.gui.tab_bool][tab_name]) {
			int container_count = GET_INTS_MAP[g_config.gui.container_count][tab_name];
			if (container_count > 0) {
				for (int i = 0; i < container_count; i++) {
					char buffer[8] = { 0 }; _itoa(i, buffer, 10);
					draw_lua_items(tab_name, buffer);
				}
			}
		}
	}
}
*/