#pragma once
#include <string>
struct ImFont;
class c_gui {
public:
	void init() noexcept;
	void render() noexcept;
	bool isOpen{ false };

private:
	//static void hotkey(int&) noexcept;
	//void updateColors() const noexcept;
	void renderMenuBar() noexcept;
	//void renderMiscWindow() noexcept;
	void renderConfigWindow() noexcept;
	//void renderSkinChangerWindow() noexcept;
	void renderLuaWindow() noexcept;

	//struct {
		//bool misc{ false };
		//bool config{ false };
		//bool skinChanger{ false };
		//bool lua{ false };
	//} window;


	struct {
		ImFont* tahoma;
		ImFont* segoeui;
		ImFont* Deng;
	} fonts;

};
extern c_gui g_gui;