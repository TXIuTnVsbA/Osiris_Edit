#include <fstream>
#include <ShlObj.h>
#include "CLua.h"
#include "../utils/console/console.h"
#include "../utils/KeyState.h"
#include "../config/config.h"

#include "../sdk/utils/interfaces/interfaces.h"
#include "../sdk/utils/memory/memory.h"
#include "../sdk/utils/netvars/netvars.h"

#include "../sdk/interfaces/Cvar.h"
#include "../sdk/interfaces/ConVar.h"
#include "../sdk/interfaces/Engine.h"
#include "../sdk/interfaces/Surface.h"
#include "../sdk/interfaces/Vector.h"
#include "../sdk/interfaces/UserCmd.h"
#include "../sdk/interfaces/Entity.h"
#include "../sdk/interfaces/EntityList.h"
#include "../sdk/interfaces/GameUI.h"
#include "../sdk/interfaces/InputSystem.h"
#include "../sdk/interfaces/Localize.h"
#include "../sdk/interfaces/GlobalVars.h"

namespace lua {
	bool g_unload_flag = false;
	lua_State* g_lua_state = NULL;
	c_lua_hookManager* hooks = new c_lua_hookManager();
	std::vector<bool> loaded;
	std::vector<std::string> scripts;
	std::vector<std::filesystem::path> pathes;
	std::map<std::string, std::map<std::string, std::vector<MenuItem_t>>> menu_items = {};

	int extract_owner(sol::this_state st) {
		sol::state_view lua_state(st);
		sol::table rs = lua_state["debug"]["getinfo"](2, "S");
		std::string source = rs["source"];
		std::string filename = std::filesystem::path(source.substr(1)).filename().string();
		return get_script_id(filename);
	}

	namespace ns_client {
		void set_event_callback(sol::this_state s, std::string eventname, sol::function func) {
			sol::state_view lua_state(s);
			sol::table rs = lua_state["debug"]["getinfo"](2, ("S"));
			std::string source = rs["source"];
			std::string filename = std::filesystem::path(source.substr(1)).filename().string();

			hooks->registerHook(eventname, get_script_id(filename), func);

			g_console.log("%s: subscribed to event %s", filename.c_str(), eventname.c_str());
		}

		void run_script(std::string scriptname) {
			load_script(get_script_id(scriptname));
		}

		void reload_active_scripts() {
			reload_all_scripts();
		}

		void refresh() {
			unload_all_scripts();
			refresh_scripts();
			load_script(get_script_id("autorun.lua"));
		}
	};

	namespace ns_config {
		/*
		config.get(key)
		Returns value of given key or nil if key not found.
		*/
		std::tuple<sol::object, sol::object, sol::object, sol::object> get(sol::this_state s, std::string key) {
			std::tuple<sol::object, sol::object, sol::object, sol::object> retn = std::make_tuple(sol::nil, sol::nil, sol::nil, sol::nil);

			for (auto kv : g_config.b)
				if (kv.first == key)
					retn = std::make_tuple(sol::make_object(s, kv.second), sol::nil, sol::nil, sol::nil);

			for (auto kv : g_config.c)
				if (kv.first == key)
					retn = std::make_tuple(sol::make_object(s, (int)(kv.second[0] * 255)), sol::make_object(s, (int)(kv.second[1] * 255)), sol::make_object(s, (int)(kv.second[2] * 255)), sol::make_object(s, (int)(kv.second[3] * 255)));

			for (auto kv : g_config.f)
				if (kv.first == key)
					retn = std::make_tuple(sol::make_object(s, kv.second), sol::nil, sol::nil, sol::nil);


			for (auto kv : g_config.i)
				if (kv.first == key)
					retn = std::make_tuple(sol::make_object(s, kv.second), sol::nil, sol::nil, sol::nil);

			for (auto kv : g_config.s)
				if (kv.first == key)
					retn = std::make_tuple(sol::make_object(s, kv.second), sol::nil, sol::nil, sol::nil);

			for (auto kv : g_config.i_b)
				if (kv.first == key)
					retn = std::make_tuple(sol::make_object(s, kv.second), sol::nil, sol::nil, sol::nil);

			for (auto kv : g_config.i_f)
				if (kv.first == key)
					retn = std::make_tuple(sol::make_object(s, kv.second), sol::nil, sol::nil, sol::nil);

			for (auto kv : g_config.i_i)
				if (kv.first == key)
					retn = std::make_tuple(sol::make_object(s, kv.second), sol::nil, sol::nil, sol::nil);

			for (auto kv : g_config.i_s)
				if (kv.first == key)
					retn = std::make_tuple(sol::make_object(s, kv.second), sol::nil, sol::nil, sol::nil);

			for (auto kv : g_config.s_b)
				if (kv.first == key)
					retn = std::make_tuple(sol::make_object(s, kv.second), sol::nil, sol::nil, sol::nil);

			for (auto kv : g_config.s_f)
				if (kv.first == key)
					retn = std::make_tuple(sol::make_object(s, kv.second), sol::nil, sol::nil, sol::nil);

			for (auto kv : g_config.s_i)
				if (kv.first == key)
					retn = std::make_tuple(sol::make_object(s, kv.second), sol::nil, sol::nil, sol::nil);

			for (auto kv : g_config.s_s)
				if (kv.first == key)
					retn = std::make_tuple(sol::make_object(s, kv.second), sol::nil, sol::nil, sol::nil);

			return retn;
		}

		/*
		config.set(key, value)
		Sets value for key
		*/
		void set_bool(std::string key, bool v) {
			g_config.b[key] = v;
		}

		void set_string(std::string key, std::string v) {
			g_config.s[key] = v;
		}

		void set_float(std::string key, float v) {
			if (ceilf(v) != v)
				g_config.f[key] = v;
			else
				g_config.i[key] = (int)v;
		}

		/*
		config.set(key, r,g,b,a)
		*/
		void set_color(std::string key, int r, int g, int b, int a) {
			g_config.c[key][0] = r / 255.f;
			g_config.c[key][1] = g / 255.f;
			g_config.c[key][2] = b / 255.f;
			g_config.c[key][3] = a / 255.f;
		}

		/*
		config.set(key, index, value)
		*/
		void set_select_tuple(std::string key, int pos, bool v) {
			g_config.i_b[key][pos] = v;
		}

		void set_string_tuple(std::string key, int pos, std::string v) {
			g_config.i_s[key][pos] = v;
		}

		void set_number_tuple(std::string key, int pos, float v) {
			if (ceilf(v) != v)
				g_config.i_f[key][pos] = v;
			else
				g_config.i_i[key][pos] = (int)v;

		}

		void set_select_map(std::string key, std::string pos, bool v) {
			g_config.s_b[key][pos] = v;
		}

		void set_string_map(std::string key, std::string pos, std::string v) {
			g_config.s_s[key][pos] = v;
		}

		void set_number_map(std::string key, std::string pos, float v) {
			if (ceilf(v) != v)
				g_config.s_f[key][pos] = v;
			else
				g_config.s_i[key][pos] = (int)v;

		}

		/*
		config.load()
		Loads selected config
		*/
		void load(size_t id) {
			g_config.load(id);
		}

		/*
		config.save()
		Saves selected config
		*/
		void save(size_t id) {
			g_config.save(id);
		}
	};

	namespace ns_cvar {
		ConVar* find_var(std::string name) {
			return g_interfaces.cvar->findVar(name.c_str());
		}
		void console_color_printf(CColor color, std::string v) {
			g_interfaces.cvar->consoleColorPrintf(color, v.c_str());
		}
		void console_printf(std::string v) {
			CColor color{};
			g_interfaces.cvar->consoleColorPrintf(color, v.c_str());
		}
	};

	namespace ns_convar {
		int get_int(ConVar* c_convar) {
			return c_convar->getInt();
		}

		float get_float(ConVar* c_convar) {
			return c_convar->getFloat();
		}

		void set_int(ConVar* c_convar, int value) {
			c_convar->setValue(value);
		}

		void set_float(ConVar* c_convar, float value) {
			c_convar->setValue(value);
		}

		void set_char(ConVar* c_convar, std::string value) {
			c_convar->setValue(value.c_str());
		}
	};

	namespace ns_engine {
		void client_cmd(std::string cmd) {
			g_interfaces.engine->clientCmd(cmd.c_str());
		}
		void client_cmd_unrestricted(std::string cmd) {
			g_interfaces.engine->clientCmdUnrestricted(cmd.c_str());
		}
		void execute_client_cmd(std::string cmd) {
			g_interfaces.engine->executeClientCmd(cmd.c_str());
		}

		PlayerInfo get_player_info(int ent) {
			PlayerInfo p;
			g_interfaces.engine->getPlayerInfo(ent, p);
			return p;
		}

		int get_player_for_user_id(int userid) {
			return g_interfaces.engine->getPlayerForUserID(userid);
		}

		int get_local_player_index() {
			return g_interfaces.engine->getLocalPlayer();
		}

		Vector get_view_angles() {
			Vector va;
			g_interfaces.engine->getViewAngles(va);
			return va;
		}

		void set_view_angles(Vector va) {
			g_interfaces.engine->setViewAngles(va);
		}

		int get_max_clients() {
			return g_interfaces.engine->getMaxClients();
		}

		bool is_in_game() {
			return g_interfaces.engine->isInGame();
		}

		bool is_connected() {
			return g_interfaces.engine->isConnected();
		}
	};

	namespace ns_entity {
		bool is_pistol(Entity* entity) {
			return entity->isPistol();
		}
		bool is_sniper_rifle(Entity* entity) {
			return entity->isSniperRifle();
		}
		Vector get_bone_position(Entity* entity, int bone) {
			return entity->getBonePosition(bone);
		}
		Vector get_eye_position(Entity* entity) {
			return entity->getEyePosition();
		}
		bool is_visible_none(Entity* entity) {
			return entity->isVisible();
		}
		bool is_visible_position(Entity* entity, Vector v) {
			return entity->isVisible(v);
		}
		bool is_enemy(Entity* entity) {
			return entity->isEnemy();
		}
		bool is_dormant(Entity* entity) {
			return entity->isDormant();
		}
		bool is_weapon(Entity* entity) {
			return entity->isWeapon();
		}
		bool is_alive(Entity* entity) {
			return entity->isAlive();
		}
		bool is_player(Entity* entity) {
			return entity->isPlayer();
		}
		Entity* get_active_weapon(Entity* entity) {
			return entity->getActiveWeapon();
		}
		int get_weapon_sub_type(Entity* entity) {
			return entity->getWeaponSubType();
		}
		WeaponData* get_weapon_data(Entity* entity) {
			return entity->getWeaponData();
		}
		float get_inaccuracy(Entity* entity) {
			return entity->getInaccuracy();
		}
		Vector get_abs_origin(Entity* entity) {
			return entity->getAbsOrigin();
		}
		AnimState* get_animstate(Entity* entity) {
			return entity->getAnimstate();
		}
		float get_max_desync_angle(Entity* entity) {
			return entity->getMaxDesyncAngle();
		}
		bool is_in_reload(Entity* entity) {
			return entity->isInReload();
		}
		Vector get_aim_punch(Entity* entity) {
			return entity->getAimPunch();
		}

		//NETVAR
		int get_health(Entity* entity) {
			return entity->health();
		}
		int get_body(Entity* entity) {
			return entity->body();
		}
		int get_hitboxSet(Entity* entity) {
			return entity->hitboxSet();
		}
		Vector get_origin(Entity* entity) {
			return entity->origin();
		}
		int get_crosshairId(Entity* entity) {
			return entity->crosshairId();
		}
		Vector get_aim_punch_angle(Entity* entity) {
			return entity->aimPunchAngle();
		}


		sol::object get_netvar_offset(sol::this_state s, Entity* entity, std::string class_name, std::string var_name, int offset) {
			auto hash = fnv::hash((class_name + "->" + var_name).c_str());
			Vector v4 = *reinterpret_cast<std::add_pointer_t<Vector>>(entity + g_netvars[hash] + offset);
			if (v4)
				return sol::make_object(s, v4);

			float v3 = *reinterpret_cast<std::add_pointer_t<float>>(entity + g_netvars[hash] + offset);
			if (v3)
				return sol::make_object(s, v3);

			int v1 = *reinterpret_cast<std::add_pointer_t<int>>(entity + g_netvars[hash] + offset);
			if (v1)
				return sol::make_object(s, v1);

			bool v2 = *reinterpret_cast<std::add_pointer_t<bool>>(entity + g_netvars[hash] + offset);
			if (v2)
				return sol::make_object(s, v2);

			return sol::nil;
		}

		int get_netvar_int(Entity* entity, std::string class_name, std::string var_name, int offset)
		{
			auto hash = fnv::hash((class_name + "->" + var_name).c_str());
			return *reinterpret_cast<std::add_pointer_t<int>>(entity + g_netvars[hash] + offset);
		}
		bool get_netvar_bool(Entity* entity, std::string class_name, std::string var_name, int offset)
		{
			auto hash = fnv::hash((class_name + "->" + var_name).c_str());
			return *reinterpret_cast<std::add_pointer_t<bool>>(entity + g_netvars[hash] + offset);

		}
		float get_netvar_float(Entity* entity, std::string class_name, std::string var_name, int offset)
		{
			auto hash = fnv::hash((class_name + "->" + var_name).c_str());
			return *reinterpret_cast<std::add_pointer_t<float>>(entity + g_netvars[hash] + offset);
		}
		Vector get_netvar_vector(Entity* entity, std::string class_name, std::string var_name, int offset)
		{
			auto hash = fnv::hash((class_name + "->" + var_name).c_str());
			return *reinterpret_cast<std::add_pointer_t<Vector>>(entity + g_netvars[hash] + offset);
		}

		void set_netvar_int(Entity* entity, std::string class_name, std::string var_name, int offset, int var)
		{
			auto hash = fnv::hash((class_name + "->" + var_name).c_str());
			auto v1 = reinterpret_cast<std::add_pointer_t<int>>(entity + g_netvars[hash] + offset);
			if (v1)
				*v1 = var;
		}
		void set_netvar_bool(Entity* entity, std::string class_name, std::string var_name, int offset, bool var)
		{
			auto hash = fnv::hash((class_name + "->" + var_name).c_str());
			auto v1 = reinterpret_cast<std::add_pointer_t<bool>>(entity + g_netvars[hash] + offset);
			if (v1)
				*v1 = var;
		}
		void set_netvar_float(Entity* entity, std::string class_name, std::string var_name, int offset, float var)
		{
			auto hash = fnv::hash((class_name + "->" + var_name).c_str());
			auto v1 = reinterpret_cast<std::add_pointer_t<float>>(entity + g_netvars[hash] + offset);
			if (v1)
				*v1 = var;
		}
		void set_netvar_vector(Entity* entity, std::string class_name, std::string var_name, int offset, Vector var)
		{
			auto hash = fnv::hash((class_name + "->" + var_name).c_str());
			auto v1 = reinterpret_cast<std::add_pointer_t<Vector>>(entity + g_netvars[hash] + offset);
			if (v1)
				*v1 = var;
		}

		//void set_pnetvar_int(Entity* entity, std::string class_name, std::string var_name, int offset, int var){}
		//void set_pnetvar_bool(Entity* entity, std::string class_name, std::string var_name, int offset, bool var){}
		//void set_pnetvar_float(Entity* entity, std::string class_name, std::string var_name, int offset, float var){}
		//void set_pnetvar_vector(Entity* entity, std::string class_name, std::string var_name, int offset, Vector var){}
	};

	namespace ns_entity_list {
		Entity* get_entity(int index) {
			return g_interfaces.entityList->getEntity(index);
		}
		Entity* get_entity_from_handle(int handle) {
			return g_interfaces.entityList->getEntityFromHandle(handle);
		}
		int get_highest_entity_index() {
			return g_interfaces.entityList->getHighestEntityIndex();
		}
	};

	namespace ns_surface {
		void draw_line(float x0, float y0, float x1, float y1) {
			g_interfaces.surface->drawLine(x0, y0, x1, y1);
		}
		void set_draw_color(int r, int g, int b, int a) {
			g_interfaces.surface->setDrawColor(r, g, b, a);
		}
		void draw_filled_rect(float x0, float y0, float x1, float y1) {
			g_interfaces.surface->drawFilledRect(x0, y0, x1, y1);
		}
		void draw_filled_rect_fade(int x, int y, int x2, int y2, int alpha, int alpha2, bool horizontal) {
			g_interfaces.surface->drawFilledRectFade(x, y, x2, y2, alpha, alpha2, horizontal);
		}
		void draw_outlined_rect(float x0, float y0, float x1, float y1) {
			g_interfaces.surface->drawOutlinedRect(x0, y0, x1, y1);
		}
		void draw_circle(float x, float y, int startRadius, int radius) {
			g_interfaces.surface->drawCircle(x, y, startRadius, radius);
		}
		void draw_outlined_circle(float x, float y, int r, int seg) {
			g_interfaces.surface->drawOutlinedCircle(x, y, r, seg);
		}
		int create_texture() {
			return g_interfaces.surface->createNewTextureID();
		}
		void set_texture(int id) {
			g_interfaces.surface->drawSetTexture(id);
		}
		void set_texture_rgba(int id, const unsigned char* rgba, int w, int h) {
			g_interfaces.surface->drawSetTextureRGBA(id, rgba, w, h);
		}
		void draw_textured_rect(int x, int y, int x2, int y2) {
			g_interfaces.surface->drawTexturedRect(x, y, x2, y2);
		}
		Vector2D get_screen_size() {
			Vector2D tmp0 = Vector2D();
			auto tmp1 = g_interfaces.surface->getScreenSize();
			tmp0.x = tmp1.first;
			tmp0.y = tmp1.second;
			return tmp0;
		}
		int create_font(std::string fontname, int w, int h, int blur, int flags) {
			auto f = g_interfaces.surface->createFont();
			g_interfaces.surface->setFontGlyphSet(f, fontname.c_str(), w, h, blur, 0, flags);
			return f;
		}
		void set_text_font(int font) {
			g_interfaces.surface->setTextFont(font);
		}
		void set_text_color(int r, int g, int b, int a) {
			g_interfaces.surface->setTextColor(r, g, b, a);
		}
		void set_text_pos(int x, int y) {
			g_interfaces.surface->setTextPosition(x, y);
		}
		void draw_text(std::wstring str) {
			g_interfaces.surface->printText(str.c_str(), str.length());
		}
		Vector2D get_text_size(int font, std::wstring text) {
			Vector2D tmp0 = Vector2D();
			auto tmp1 = g_interfaces.surface->getTextSize(font, text.c_str());
			tmp0.x = tmp1.first;
			tmp0.y = tmp1.second;
			return tmp0;
		}
		void play_sound(std::string fileName) {
			g_interfaces.surface->playSound(fileName.c_str());
		}
	};

	namespace ns_input_system {
		void enable_input(bool enable) {
			g_interfaces.inputSystem->enableInput(enable);
		}
		bool is_button_down(int buttonCode) {
			return g_interfaces.inputSystem->isButtonDown(buttonCode);
		}
		void reset_input_state() {
			g_interfaces.inputSystem->resetInputState();
		}
		const char* button_code_to_string(int buttonCode) {
			return g_interfaces.inputSystem->buttonCodeToString(buttonCode);
		}
		int virtual_key_to_button_code(int virtualKey) {
			return g_interfaces.inputSystem->virtualKeyToButtonCode(virtualKey);
		}
		const char* virtual_key_to_string(int virtualKey) {
			return g_interfaces.inputSystem->virtualKeyToString(virtualKey);
		}

		bool is_key_down(int keycode) {
			return KEYDOWN(keycode);
		}
		bool is_key_up(int keycode) {
			return KEYUP(keycode);
		}

		Vector2D get_mouse_position()
		{
			POINT mousePosition;
			GetCursorPos(&mousePosition);
			ScreenToClient(FindWindow(0, "Counter-Strike: Global Offensive"), &mousePosition);
			return { static_cast<float>(mousePosition.x), static_cast<float>(mousePosition.y) };
		}

		bool mouse_in_region(int x, int y, int x2, int y2) {
			if (get_mouse_position().x > x&& get_mouse_position().y > y&& get_mouse_position().x < x2 + x && get_mouse_position().y < y2 + y)
				return true;
			return false;
		}

	};

	namespace ns_localize {
		std::wstring find(std::string token) {
			return g_interfaces.localize->find(token.c_str());
		}
	};

	namespace ns_ui {
		std::string new_checkbox(sol::this_state s, std::string tab, std::string container, std::string label, std::string key, std::optional<bool> def, std::optional<sol::function> cb) {
			MenuItem_t item;
			item.type = MENUITEM_CHECKBOX;
			item.script = extract_owner(s);
			item.label = label;
			item.key = key;
			item.b_default = def.value_or(false);
			item.callback = cb.value_or(sol::nil);

			menu_items[tab][container].push_back(item);
			return key;
		}

		std::string new_slider_int(sol::this_state s, std::string tab, std::string container, std::string label, std::string key, int min, int max, std::optional<std::string> format, std::optional<int> def, std::optional<sol::function> cb) {
			MenuItem_t item;
			item.type = MENUITEM_SLIDERINT;
			item.script = extract_owner(s);
			item.label = label;
			item.key = key;
			item.i_default = def.value_or(0);
			item.i_min = min;
			item.i_max = max;
			item.format = format.value_or("%d");
			item.callback = cb.value_or(sol::nil);

			menu_items[tab][container].push_back(item);
			return key;
		}

		std::string new_slider_float(sol::this_state s, std::string tab, std::string container, std::string label, std::string key, float min, float max, std::optional<std::string> format, std::optional<float> def, std::optional<sol::function> cb) {
			MenuItem_t item;
			item.type = MENUITEM_SLIDERFLOAT;
			item.script = extract_owner(s);
			item.label = label;
			item.key = key;
			item.f_default = def.value_or(0.f);
			item.f_min = min;
			item.f_max = max;
			item.format = format.value_or("%.0f");
			item.callback = cb.value_or(sol::nil);

			menu_items[tab][container].push_back(item);
			return key;
		}

		std::string new_text(sol::this_state s, std::string tab, std::string container, std::string label, std::string key) {
			MenuItem_t item;
			item.type = MENUITEM_TEXT;
			item.script = extract_owner(s);
			item.label = label;
			item.key = key;

			menu_items[tab][container].push_back(item);
			return key;
		}

		std::string new_colorpicker(sol::this_state s, std::string tab, std::string container, std::string id, std::string key, std::optional<int> r, std::optional<int> g, std::optional<int> b, std::optional<int> a, std::optional<sol::function> cb) {
			MenuItem_t item;
			item.type = MENUITEM_COLORPICKER;
			item.script = extract_owner(s);
			item.label = id;
			item.key = key;
			item.c_default[0] = r.value_or(255) / 255.f;
			item.c_default[1] = g.value_or(255) / 255.f;
			item.c_default[2] = b.value_or(255) / 255.f;
			item.c_default[3] = a.value_or(255) / 255.f;
			item.callback = cb.value_or(sol::nil);

			menu_items[tab][container].push_back(item);
			return key;
		}

		std::string new_button(sol::this_state s, std::string tab, std::string container, std::string id, std::string key, std::optional<sol::function> cb) {
			MenuItem_t item;
			item.type = MENUITEM_BUTTON;
			item.script = extract_owner(s);
			item.label = id;
			item.key = key;
			item.callback = cb.value_or(sol::nil);

			menu_items[tab][container].push_back(item);
			return key;
		}

		void set_visibility(std::string key, bool v) {
			for (auto t : menu_items) {
				for (auto c : t.second) {
					for (auto& i : c.second) {
						if (i.key == key)
							i.is_visible = v;
					}
				}
			}
		}

		void set_items(std::string key, std::vector<const char*> items) {
			for (auto t : menu_items) {
				for (auto c : t.second) {
					for (auto& i : c.second) {
						if (i.key == key)
							i.items = items;
					}
				}
			}
		}

	};

	namespace ns_game_ui {
		void message_box(const char* title, const char* text) {
			g_interfaces.gameUI->messageBox(title, text);
		}
	};

	void test_func() {
		g_console.log("RunTestFunc");
		for (auto hk : hooks->getHooks("on_test"))
		{
			try
			{
				auto result = hk.func();
				if (!result.valid()) {
					sol::error err = result;
					g_console.log(err.what());
					g_interfaces.cvar->consoleColorPrintf(CColor(255, 0, 0), err.what());
				}
			}
			catch (const std::exception&)
			{

			}
		}
	}

	void init_state() {
#ifdef _DEBUG
		lua_writestring(LUA_COPYRIGHT, strlen(LUA_COPYRIGHT));
		lua_writeline();
#endif
		lua::unload();
		g_lua_state = luaL_newstate();
		luaL_openlibs(g_lua_state);
	}

	void init_command() {
		sol::state_view lua_state(g_lua_state);
		lua_state["test_func"] = []() { test_func(); };
		lua_state["exit"] = []() { g_unload_flag = true; };

		lua_state.new_enum("KEYCODES_A",
			//VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
			"VK_0", VK_0,
			"VK_1", VK_1,
			"VK_2", VK_2,
			"VK_3", VK_3,
			"VK_4", VK_4,
			"VK_5", VK_5,
			"VK_6", VK_6,
			"VK_7", VK_7,
			"VK_8", VK_8,
			"VK_9", VK_9
		);
		lua_state.new_enum("KEYCODES_B",
			//VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
			"VK_A", VK_A,
			"VK_B", VK_B,
			"VK_C", VK_C,
			"VK_D", VK_D,
			"VK_E", VK_E,
			"VK_F", VK_F,
			"VK_G", VK_G,
			"VK_H", VK_H,
			"VK_I", VK_I,
			"VK_J", VK_J,
			"VK_K", VK_K,
			"VK_L", VK_L,
			"VK_M", VK_M,
			"VK_N", VK_N,
			"VK_O", VK_O,
			"VK_P", VK_P,
			"VK_Q", VK_Q,
			"VK_R", VK_R,
			"VK_S", VK_S,
			"VK_T", VK_T,
			"VK_U", VK_U,
			"VK_V", VK_V,
			"VK_W", VK_W,
			"VK_X", VK_X,
			"VK_Y", VK_Y,
			"VK_Z", VK_Z
		);
		lua_state.new_enum("KEYCODES_C",
			//Other
			"VK_ESCAPE", VK_ESCAPE,
			"VK_RETURN", VK_RETURN,
			"VK_TAB", VK_TAB,
			"VK_CAPITAL", VK_CAPITAL,
			"VK_MENU", VK_MENU,
			"VK_LWIN", VK_LWIN,
			"VK_RWIN", VK_RWIN,
			"VK_MENU", VK_MENU,
			"VK_SHIFT", VK_SHIFT,
			"VK_LSHIFT", VK_LSHIFT,
			"VK_RSHIFT", VK_RSHIFT,
			"VK_SPACE", VK_SPACE,
			"VK_BACK", VK_BACK,
			"VK_UP", VK_UP,
			"VK_DOWN", VK_DOWN,
			"VK_LEFT", VK_LEFT,
			"VK_RIGHT", VK_RIGHT,
			"VK_END", VK_END,
			"VK_INSERT", VK_INSERT,
			"VK_HOME", VK_HOME,
			"VK_PRIOR", VK_PRIOR,
			"VK_NEXT", VK_NEXT,
			"VK_DELETE", VK_DELETE,
			"VK_LBUTTON", VK_LBUTTON,
			"VK_RBUTTON", VK_RBUTTON,
			"VK_CANCEL", VK_CANCEL,
			"VK_MBUTTON", VK_MBUTTON,
			"VK_DELETE", VK_DELETE
		);
		lua_state.new_enum("KEYCODES_D",
			"VK_F1", VK_F1,
			"VK_F2", VK_F2,
			"VK_F3", VK_F3,
			"VK_F4", VK_F4,
			"VK_F5", VK_F5,
			"VK_F6", VK_F6,
			"VK_F7", VK_F7,
			"VK_F8", VK_F8,
			"VK_F9", VK_F9,
			"VK_F10", VK_F10,
			"VK_F11", VK_F11,
			"VK_F12", VK_F12
		);
		lua_state.new_enum("KEYCODES_E",
			"VK_NUMLOCK", VK_NUMLOCK,
			"VK_NUMPAD0", VK_NUMPAD0,
			"VK_NUMPAD1", VK_NUMPAD1,
			"VK_NUMPAD2", VK_NUMPAD2,
			"VK_NUMPAD3", VK_NUMPAD3,
			"VK_NUMPAD4", VK_NUMPAD4,
			"VK_NUMPAD5", VK_NUMPAD5,
			"VK_NUMPAD6", VK_NUMPAD6,
			"VK_NUMPAD7", VK_NUMPAD7,
			"VK_NUMPAD8", VK_NUMPAD8,
			"VK_NUMPAD9", VK_NUMPAD9,
			"VK_DECIMAL", VK_DECIMAL,
			"VK_MULTIPLY", VK_MULTIPLY,
			"VK_ADD", VK_ADD,
			"VK_SUBTRACT", VK_SUBTRACT,
			"VK_DIVIDE", VK_DIVIDE,
			"VK_PAUSE", VK_PAUSE,
			"VK_SCROLL", VK_SCROLL
		);
		lua_state.new_enum("BUTTONS",
			"IN_ATTACK", UserCmd::IN_ATTACK,
			"IN_JUMP", UserCmd::IN_JUMP,
			"IN_DUCK", UserCmd::IN_DUCK,
			"IN_FORWARD", UserCmd::IN_FORWARD,
			"IN_BACK", UserCmd::IN_BACK,
			"IN_USE", UserCmd::IN_USE,
			"IN_MOVELEFT", UserCmd::IN_MOVELEFT,
			"IN_MOVERIGHT", UserCmd::IN_MOVERIGHT,
			"IN_ATTACK2", UserCmd::IN_ATTACK2,
			"IN_SCORE", UserCmd::IN_SCORE,
			"IN_BULLRUSH", UserCmd::IN_BULLRUSH);

		lua_state.new_usertype<Vector2D>("c_vector2d",
			"x", &Vector2D::x,
			"y", &Vector2D::y
			);

		lua_state.new_usertype<Vector>("c_vector",
			"x", &Vector::x,
			"y", &Vector::y,
			"z", &Vector::z
			);

		lua_state.new_usertype<CColor>("c_color",
			sol::constructors<CColor(), CColor(int, int, int), CColor(int, int, int, int)>(),
			"r", &CColor::r,
			"g", &CColor::g,
			"b", &CColor::b,
			"a", &CColor::a
			);


		lua_state.new_usertype<UserCmd>("c_usercmd",
			"command_number", sol::readonly(&UserCmd::commandNumber),
			"tick_count", sol::readonly(&UserCmd::tickCount),
			"viewangles", &UserCmd::viewangles,
			"aimdirection", &UserCmd::aimdirection,
			"forwardmove", &UserCmd::forwardmove,
			"sidemove", &UserCmd::sidemove,
			"upmove", &UserCmd::upmove,
			"buttons", &UserCmd::buttons,
			"impulse", sol::readonly(&UserCmd::impulse),
			"weaponselect", &UserCmd::weaponselect,
			"weaponsubtype", sol::readonly(&UserCmd::weaponsubtype),
			"random_seed", sol::readonly(&UserCmd::randomSeed),
			"mousedx", &UserCmd::mousedx,
			"mousedy", &UserCmd::mousedy,
			"hasbeenpredicted", sol::readonly(&UserCmd::hasbeenpredicted)
			);

		auto config = lua_state.create_table();
		config["get"] = ns_config::get;
		config["set"] = sol::overload(
			ns_config::set_bool,
			ns_config::set_color,
			ns_config::set_float,
			ns_config::set_string,
			ns_config::set_select_tuple,
			ns_config::set_number_tuple,
			ns_config::set_string_tuple,
			ns_config::set_select_map,
			ns_config::set_number_map,
			ns_config::set_string_map
		);
		config["save"] = ns_config::save;
		config["load"] = ns_config::load;
		lua_state["config"] = config;


		auto client = lua_state.create_table();
		client["set_event_callback"] = ns_client::set_event_callback;
		client["run_script"] = ns_client::run_script;
		client["reload_active_scripts"] = ns_client::reload_active_scripts;
		client["refresh"] = ns_client::refresh;
		lua_state["client"] = client;

		auto engine = lua_state.create_table();
		engine["client_cmd"] = ns_engine::client_cmd;
		engine["client_cmd_unrestricted"] = ns_engine::client_cmd_unrestricted;
		engine["execute_client_cmd"] = ns_engine::execute_client_cmd;
		engine["get_local_player_index"] = ns_engine::get_local_player_index;
		engine["get_max_clients"] = ns_engine::get_max_clients;
		engine["get_player_for_user_id"] = ns_engine::get_player_for_user_id;
		engine["get_player_info"] = ns_engine::get_player_info;
		engine["get_view_angles"] = ns_engine::get_view_angles;
		engine["is_connected"] = ns_engine::is_connected;
		engine["is_in_game"] = ns_engine::is_in_game;
		engine["set_view_angles"] = ns_engine::set_view_angles;
		lua_state["engine"] = engine;


		auto entity = lua_state.create_table();
		entity["is_pistol"] = ns_entity::is_pistol;
		entity["is_sniper_rifle"] = ns_entity::is_sniper_rifle;
		entity["get_bone_position"] = ns_entity::get_bone_position;
		entity["get_eye_position"] = ns_entity::get_eye_position;
		entity["is_visible_none"] = ns_entity::is_visible_none;
		entity["is_visible_position"] = ns_entity::is_visible_position;
		entity["is_enemy"] = ns_entity::is_enemy;
		entity["is_dormant"] = ns_entity::is_dormant;
		entity["is_weapon"] = ns_entity::is_weapon;
		entity["is_alive"] = ns_entity::is_alive;
		entity["is_player"] = ns_entity::is_player;
		entity["get_active_weapon"] = ns_entity::get_active_weapon;
		entity["get_weapon_sub_type"] = ns_entity::get_weapon_sub_type;
		//entity["get_weapon_data"] = ns_entity::get_weapon_data;
		entity["get_inaccuracy"] = ns_entity::get_inaccuracy;
		entity["get_abs_origin"] = ns_entity::get_abs_origin;
		entity["get_animstate"] = ns_entity::get_animstate;
		entity["get_max_desync_angle"] = ns_entity::get_max_desync_angle;
		entity["is_in_reload"] = ns_entity::is_in_reload;
		entity["get_aim_punch"] = ns_entity::get_aim_punch;
		//
		entity["get_health"] = ns_entity::get_health;
		entity["get_body"] = ns_entity::get_body;
		entity["get_hitboxSet"] = ns_entity::get_hitboxSet;
		entity["get_origin"] = ns_entity::get_origin;
		entity["get_crosshairId"] = ns_entity::get_crosshairId;
		entity["get_aim_punch_angle"] = ns_entity::get_aim_punch_angle;
		//
		//entity["get_netvar_offset"] = ns_entity::get_netvar_offset;
		//entity["get_pnetvar_offset"] = ns_entity::get_pnetvar_offset;
		//
		entity["get_netvar_int"] = ns_entity::get_netvar_int;
		entity["get_netvar_bool"] = ns_entity::get_netvar_bool;
		entity["get_netvar_float"] = ns_entity::get_netvar_float;
		entity["get_netvar_vector"] = ns_entity::get_netvar_vector;
		entity["get_prop"] = ns_entity::get_netvar_offset;

		entity["set_netvar_int"] = ns_entity::set_netvar_int;
		entity["set_netvar_bool"] = ns_entity::set_netvar_bool;
		entity["set_netvar_float"] = ns_entity::set_netvar_float;
		entity["set_netvar_vector"] = ns_entity::set_netvar_vector;

		entity["set_prop"] = sol::overload(
			ns_entity::set_netvar_int,
			ns_entity::set_netvar_bool,
			ns_entity::set_netvar_float,
			ns_entity::set_netvar_vector
		);
		lua_state["entity"] = entity;


		auto entity_list = lua_state.create_table();
		entity_list["get_entity"] = ns_entity_list::get_entity;
		entity_list["get_entity_from_handle"] = ns_entity_list::get_entity_from_handle;
		entity_list["get_highest_entity_index"] = ns_entity_list::get_highest_entity_index;
		lua_state["entity_list"] = entity_list;

		auto cvar = lua_state.create_table();
		cvar["find_var"] = ns_cvar::find_var;
		cvar["console_printf"] = ns_cvar::console_printf;
		cvar["console_color_printf"] = ns_cvar::console_color_printf;
		lua_state["cvar"] = cvar;


		auto convar = lua_state.create_table();
		convar["get_int"] = ns_convar::get_int;
		convar["set_int"] = ns_convar::set_int;
		convar["get_float"] = ns_convar::get_float;
		convar["set_float"] = ns_convar::set_float;
		convar["set_char"] = ns_convar::set_char;
		lua_state["convar"] = convar;


		auto surface = lua_state.create_table();
		surface["draw_line"] = ns_surface::draw_line;
		surface["set_draw_color"] = ns_surface::set_draw_color;
		surface["draw_filled_rect"] = ns_surface::draw_filled_rect;
		surface["draw_filled_rect_fade"] = ns_surface::draw_filled_rect_fade;
		surface["draw_outlined_rect"] = ns_surface::draw_outlined_rect;
		surface["draw_circle"] = ns_surface::draw_circle;
		surface["draw_outlined_circle"] = ns_surface::draw_outlined_circle;
		surface["create_texture"] = ns_surface::create_texture;
		surface["set_texture"] = ns_surface::set_texture;
		surface["set_texture_rgba"] = ns_surface::set_texture_rgba;
		surface["draw_textured_rect"] = ns_surface::draw_textured_rect;
		surface["get_screen_size"] = ns_surface::get_screen_size;
		surface["create_font"] = ns_surface::create_font;
		surface["set_text_font"] = ns_surface::set_text_font;
		surface["set_text_color"] = ns_surface::set_text_color;
		surface["set_text_pos"] = ns_surface::set_text_pos;
		surface["draw_text"] = ns_surface::draw_text;
		surface["get_text_size"] = ns_surface::get_text_size;
		surface["play_sound"] = ns_surface::play_sound;
		lua_state["surface"] = surface;

		auto input_system = lua_state.create_table();
		input_system["enable_input"] = ns_input_system::enable_input;
		input_system["is_button_down"] = ns_input_system::is_button_down;
		input_system["reset_input_state"] = ns_input_system::reset_input_state;
		input_system["button_code_to_string"] = ns_input_system::button_code_to_string;
		input_system["virtual_key_to_button_code"] = ns_input_system::virtual_key_to_button_code;
		input_system["virtual_key_to_string"] = ns_input_system::virtual_key_to_string;
		input_system["get_mouse_position"] = ns_input_system::get_mouse_position;
		input_system["mouse_in_region"] = ns_input_system::mouse_in_region;
		input_system["is_key_down"] = ns_input_system::is_key_down;
		input_system["is_key_up"] = ns_input_system::is_key_up;
		lua_state["input_system"] = input_system;

		auto localize = lua_state.create_table();
		localize["find"] = ns_localize::find;
		lua_state["localize"] = localize;

		auto ui = lua_state.create_table();
		ui["new_checkbox"] = ns_ui::new_checkbox;
		ui["new_colorpicker"] = ns_ui::new_colorpicker;
		//ui["new_keybind"] = ns_ui::new_keybind;
		//ui["new_multiselect"] = ns_ui::new_multiselect;
		//ui["new_singleselect"] = ns_ui::new_singleselect;
		ui["new_slider_float"] = ns_ui::new_slider_float;
		ui["new_slider_int"] = ns_ui::new_slider_int;
		ui["new_text"] = ns_ui::new_text;
		ui["new_button"] = ns_ui::new_button;
		//ui["set_callback"] = ns_ui::set_callback;
		ui["set_items"] = ns_ui::set_items;
		//ui["set_label"] = ns_ui::set_label;
		ui["set_visibility"] = ns_ui::set_visibility;
		//ui["is_bind_active"] = ns_ui::is_bind_active;
		lua_state["ui"] = ui;

		auto game_ui = lua_state.create_table();
		game_ui["message_box"] = ns_game_ui::message_box;
		lua_state["game_ui"] = game_ui;

		refresh_scripts();
		load_script(get_script_id("autorun.lua"));

	}

	void init_console() {
		std::string str;
		//printf("lua_ptr: 0x%x \n", &g_console);
		g_console.log("LUA");
		std::printf(">");
		while (!g_unload_flag && std::getline(std::cin, str))
		{
			try
			{
				bool tmp = luaL_dostring(g_lua_state, str.c_str());
				if (tmp) {
					std::printf("lua::Error: %s\n", lua_tostring(g_lua_state, -1));
				}
			}
			catch (const std::exception & Error)
			{
				std::printf("std::Error: %s\n", Error.what());
				//std::printf("lua::Error: %s\n", lua_tostring(g_lua_state, 0));
				//g_console.log("std::Error: %s", Error.what());
				//g_console.log("lua::Error: %s", lua_tostring(g_lua_state, 0));
			}
			g_console.log("LUA");
			std::printf(">");
		}
	}

	void unload() {
		if (g_lua_state != NULL) {
			lua_close(g_lua_state);
			g_lua_state = NULL;
		}
	}

	void load_script(int id) {
		if (id == -1)
			return;

		if (loaded.at(id))
			return;

		auto path = get_script_path(id);
		if (path == (""))
			return;

		sol::state_view state(g_lua_state);
		state.script_file(path, [](lua_State* me, sol::protected_function_result result) {
			if (!result.valid()) {
				sol::error err = result;
				//Utilities->Game_Msg(err.what());
				g_console.log(err.what());
				//printf("%s\n", err.what());
			}

			return result;
			});
		loaded.at(id) = true;
	}

	void unload_script(int id) {
		if (id == -1)
			return;

		if (!loaded.at(id))
			return;

		std::map<std::string, std::map<std::string, std::vector<MenuItem_t>>> updated_items;
		for (auto i : menu_items) {
			for (auto k : i.second) {
				std::vector<MenuItem_t> updated_vec;

				for (auto m : k.second)
					if (m.script != id)
						updated_vec.push_back(m);

				updated_items[k.first][i.first] = updated_vec;
			}
		}
		menu_items = updated_items;

		hooks->unregisterHooks(id);
		loaded.at(id) = false;
	}

	void reload_all_scripts() {
		for (auto s : scripts) {
			if (loaded.at(get_script_id(s))) {
				unload_script(get_script_id(s));
				load_script(get_script_id(s));
			}
		}
	}

	void unload_all_scripts() {
		for (auto s : scripts)
			if (loaded.at(get_script_id(s)))
				unload_script(get_script_id(s));
	}

	void refresh_scripts() {
		std::filesystem::path path;
		path = g_config.get_path();
		path /= "Lua";

		if (!std::filesystem::is_directory(path)) {
			std::filesystem::remove(path);
			std::filesystem::create_directory(path);
		}

		auto oldLoaded = loaded;
		auto oldScripts = scripts;

		loaded.clear();
		pathes.clear();
		scripts.clear();

		for (auto& entry : std::filesystem::directory_iterator((path))) {
			if (entry.path().extension() == (".lua")) {
				auto path = entry.path();
				auto filename = path.filename().string();

				bool didPut = false;
				int oldScriptsSize = 0;
				oldScriptsSize = oldScripts.size();
				if (oldScriptsSize < 0)
					continue;

				for (int i = 0; i < oldScriptsSize; i++) {
					if (filename == oldScripts.at(i)) {
						loaded.push_back(oldLoaded.at(i));
						didPut = true;
					}
				}

				if (!didPut)
					loaded.push_back(false);

				pathes.push_back(path);
				scripts.push_back(filename);
			}
		}
	}

	int get_script_id(std::string name) {
		int scriptsSize = 0;
		scriptsSize = scripts.size();
		if (scriptsSize <= 0)
			return -1;

		for (int i = 0; i < scriptsSize; i++) {
			if (scripts.at(i) == name)
				return i;
		}

		return -1;
	}

	int get_script_id_by_path(std::string path) {
		int pathesSize = 0;
		pathesSize = pathes.size();
		if (pathesSize <= 0)
			return -1;

		for (int i = 0; i < pathesSize; i++) {
			if (pathes.at(i).string() == path)
				return i;
		}

		return -1;
	}

	std::string get_script_path(std::string name) {
		return get_script_path(get_script_id(name));
	}

	std::string get_script_path(int id) {
		if (id == -1)
			return  "";

		return pathes.at(id).string();
	}

	void c_lua_hookManager::registerHook(std::string eventName, int scriptId, sol::protected_function func) {
		c_lua_hook hk = { scriptId, func };

		this->hooks[eventName].push_back(hk);
	}

	void c_lua_hookManager::unregisterHooks(int scriptId) {
		for (auto& ev : this->hooks) {
			int pos = 0;

			for (auto& hk : ev.second) {
				if (hk.scriptId == scriptId)
					ev.second.erase(ev.second.begin() + pos);

				pos++;
			}
		}
	}

	std::vector<c_lua_hook> c_lua_hookManager::getHooks(std::string eventName) {
		return this->hooks[eventName];
	}
};

