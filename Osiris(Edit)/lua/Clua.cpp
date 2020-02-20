#include <fstream>
#include <ShlObj.h>
#include "CLua.h"
#include "../utils/console/console.h"
#include "../utils/KeyState.h"
#include "../config/config.h"
namespace lua {
	bool g_unload_flag = false;
	lua_State* g_lua_state = NULL;
	c_lua_hookManager* hooks = new c_lua_hookManager();
	std::vector<bool> loaded;
	std::vector<std::string> scripts;
	std::vector<std::filesystem::path> pathes;
	std::map<std::string, std::map<std::string, std::vector<MenuItem_t>>> menu_items = {};
	
	void test_func() {
		g_console.log("RunTestFunc");
		for (auto hk : hooks->getHooks("on_test"))
		{
			try
			{
				hk.func();
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
		lua_state["exit"] = []() { g_unload_flag = true; };
		lua_state["test_func"] = []() { test_func(); };

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
		state.script_file(path, [](lua_State*, sol::protected_function_result result) {
			if (!result.valid()) {
				sol::error err = result;
				//Utilities->Game_Msg(err.what());
				g_console.log(err.what());
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
				for (int i = 0; i < oldScripts.size(); i++) {
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
		for (int i = 0; i < scripts.size(); i++) {
			if (scripts.at(i) == name)
				return i;
		}

		return -1;
	}

	int get_script_id_by_path(std::string path) {
		for (int i = 0; i < pathes.size(); i++) {
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
};