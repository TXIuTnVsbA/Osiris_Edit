#include <Windows.h>
#include <thread>
#include "utils/KeyState.h"
//#include "lua/includes.h"
#include "lua/Clua.h"
#include "utils/console/console.h"
#include "config/config.h"
#include "gui/gui.h"
#include "sdk/utils/hooks/hooks.h"
#include "sdk/utils/interfaces/interfaces.h"
#include "sdk/utils/memory/memory.h"
#include "sdk/utils/netvars/netvars.h"

#include "sdk/interfaces/GameUI.h"
void init() {
#ifdef _DEBUG
	if (!g_console.allocate("Debug"))
		std::abort();

	g_console.log("INIT");

	// redirect warnings to a window similar to errors.
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_WNDW);

#else
	g_console.allocate();
#endif

	lua::init_state();

	g_config.init();
	g_config.default();
	g_interfaces.init();
	g_gui.init();
	g_memory.init();
	g_netvars.init();
	g_hooks.init();
	lua::init_command();
}

void load() {
#ifdef _DEBUG
#else
#endif
	for (auto hk : lua::hooks->getHooks("on_load"))
	{
		try
		{
			auto result = hk.func();
			if (!result.valid()) {
				sol::error err = result;
				g_console.log(err.what());
			}
		}
		catch (const std::exception&)
		{

		}
	}
	
	g_hooks.hook();
	g_interfaces.gameUI->messageBox("Osiris", "Loaded");
}

void unload() {
	for (auto hk : lua::hooks->getHooks("on_unload"))
	{
		try
		{
			auto result = hk.func();
			if (!result.valid()) {
				sol::error err = result;
				g_console.log(err.what());
			}
		}
		catch (const std::exception&)
		{

		}
	}
#ifdef _DEBUG
	g_console.log("UNLOAD");
	g_console.detach();
#else
#endif
	g_hooks.restore();
	g_netvars.restore();
	lua::unload();
	g_interfaces.gameUI->messageBox("Osiris", "Unloaded");
}

void wait() {
#ifdef _DEBUG
	g_console.log("WAIT");
	lua::init_console();
#else
	while (!lua::g_unload_flag)
	{
		if (KEYDOWN(VK_END)) {
			lua::g_unload_flag = true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
#endif
}

DWORD WINAPI Init(LPVOID base) {
	while (!GetModuleHandleA("serverbrowser.dll"))
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	try
	{
		//INIT
		init();

		//LOAD
		load();
		
		//WAIT
		wait();

		//UNLOAD
		unload();
		
	}
	catch (const std::exception&)
	{
#ifdef _DEBUG
		g_console.detach();
#endif
	}
	
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	FreeLibraryAndExitThread(static_cast<HMODULE>(base), 1);
	return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
		HANDLE init_thread = CreateThread(nullptr, NULL, Init, hModule, NULL, nullptr);
		if (!init_thread)
			return 0;

		CloseHandle(init_thread);
	}
	return TRUE;
}

