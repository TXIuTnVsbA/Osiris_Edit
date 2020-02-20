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
#include "../sdk/utils/hooks/hooks.h"
#include "../sdk/utils/interfaces/interfaces.h"
#include "../sdk/utils/memory/memory.h"
#include "../sdk/utils/netvars/netvars.h"

c_gui g_gui;
constexpr auto windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
void c_gui::init() noexcept {
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(FindWindowW(L"Valve001", NULL));

	//ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();

	//style.ScrollbarSize = 9.0f;
	style.WindowMinSize = ImVec2(30, 30);
	style.FramePadding = ImVec2(4, 3);
	style.ItemSpacing = ImVec2(8, 4);
	style.Alpha = 1.f;
	style.WindowRounding = 0.0f;
	style.IndentSpacing = 6.0f;
	style.ItemInnerSpacing = ImVec2(3, 4);
	style.ColumnsMinSpacing = 50.0f;
	style.GrabMinSize = 14.0f;
	style.GrabRounding = 16.0f;
	style.ScrollbarSize = 2.0f;
	style.ScrollbarRounding = 0.0f;
	style.AntiAliasedLines = true;
	style.WindowTitleAlign = ImVec2(0.5, 0.5);
	style.FrameRounding = 5.f;
	style.ChildRounding = 0.f;

	style.WindowRounding = 0.f;
	style.FramePadding = ImVec2(4, 1);
	style.ScrollbarSize = 10.f;
	style.ScrollbarRounding = 0.f;
	style.GrabMinSize = 5.f;

	style.Colors[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.045f, 0.045f, 0.045f, 1.00f);
	//style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.11f, 0.11f, 0.11f, 0.00f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.09f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.04f, 0.04f, 0.04f, 0.88f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.40f, 0.95f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.24f, 0.40f, 0.95f, 0.59f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.59f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.24f, 0.40f, 0.95f, 1.00f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.24f, 0.40f, 0.95f, 1.00f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.40f, 0.95f, 0.59f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.24f, 0.40f, 0.95f, 1.00f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.40f, 0.95f, 0.59f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	//style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.70f, 0.02f, 0.60f, 0.22f);
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;

	if (PWSTR pathToFonts; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &pathToFonts))) {
		const std::filesystem::path path{ pathToFonts };
		CoTaskMemFree(pathToFonts);

		static ImWchar ranges[] = { 0x0020, 0x00FF, 0x0100, 0x017f, 0 };
		fonts.tahoma = io.Fonts->AddFontFromFileTTF((path / "tahoma.ttf").string().c_str(), 16.0f, nullptr, ranges);
		fonts.segoeui = io.Fonts->AddFontFromFileTTF((path / "segoeui.ttf").string().c_str(), 16.0f, nullptr, ranges);
		//io.FontDefault = io.Fonts->AddFontFromFileTTF((path / "Deng.ttf").string().c_str(), 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
		fonts.Deng = io.Fonts->AddFontFromFileTTF((path / "Deng.ttf").string().c_str(), 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
	}
}

void c_gui::render() noexcept {
	renderMenuBar();
	renderConfigWindow();
	ImGui::End();
}

void c_gui::renderMenuBar() noexcept {
	ImGui::SetNextWindowSize({ 800.0f, 0.0f });
	ImGui::Begin("Osiris", nullptr, windowFlags | ImGuiWindowFlags_NoTitleBar);
	if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_Reorderable)) {
		if (ImGui::BeginTabItem("Config")) {
			window = { };
			window.config = true;
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

void c_gui::renderConfigWindow() noexcept {
	if (window.config) {
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