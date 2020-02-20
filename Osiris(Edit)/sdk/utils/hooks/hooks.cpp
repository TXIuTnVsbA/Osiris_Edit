#include <functional>
#include <intrin.h>
#include <string>
#include <Windows.h>
#include <Psapi.h>
#include <algorithm>
#include <assert.h>

//imgui
#include "../../../config/config.h"
//gui
#include "hooks.h"
#include "../interfaces/interfaces.h"
#include "../memory/memory.h"

//features
//sdk
#include "../../interfaces/Engine.h"
#include "../../interfaces/Entity.h"
#include "../../interfaces/EntityList.h"
#include "../../interfaces/GameEvent.h"
#include "../../interfaces/GameUI.h"
#include "../../interfaces/InputSystem.h"
#include "../../interfaces/MaterialSystem.h"
#include "../../interfaces/ModelRender.h"
#include "../../interfaces/Panel.h"
#include "../../interfaces/RenderContext.h"
#include "../../interfaces/ResourceAccessControl.h"
#include "../../interfaces/SoundInfo.h"
#include "../../interfaces/SoundEmitter.h"
#include "../../interfaces/Surface.h"
#include "../../interfaces/UserCmd.h"

c_hooks g_hooks;
////////////////////////////////////////////////////////////////
uintptr_t* c_vmt::findFreeDataPage(void* const base, size_t vmtSize) noexcept
{
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQuery(base, &mbi, sizeof(mbi));
    MODULEINFO moduleInfo;
    GetModuleInformation(GetCurrentProcess(), static_cast<HMODULE>(mbi.AllocationBase), &moduleInfo, sizeof(moduleInfo));

    auto moduleEnd{ reinterpret_cast<uintptr_t*>(static_cast<std::byte*>(moduleInfo.lpBaseOfDll) + moduleInfo.SizeOfImage) };

    for (auto currentAddress = moduleEnd - vmtSize; currentAddress > moduleInfo.lpBaseOfDll; currentAddress -= *currentAddress ? vmtSize : 1)
        if (!*currentAddress)
            if (VirtualQuery(currentAddress, &mbi, sizeof(mbi)) && mbi.State == MEM_COMMIT
                && mbi.Protect == PAGE_READWRITE && mbi.RegionSize >= vmtSize * sizeof(uintptr_t)
                && std::all_of(currentAddress, currentAddress + vmtSize, [](uintptr_t a) { return !a; }))
                return currentAddress;

    return nullptr;
}

auto c_vmt::calculateLength(uintptr_t* vmt) noexcept
{
    size_t length{ 0 };
    MEMORY_BASIC_INFORMATION memoryInfo;
    while (VirtualQuery(LPCVOID(vmt[length]), &memoryInfo, sizeof(memoryInfo)) && memoryInfo.Protect == PAGE_EXECUTE_READ)
        length++;
    return length;
}

bool c_vmt::init(void* const base) noexcept
{
    assert(base);
    this->base = base;
    bool init = false;

    if (!oldVmt) {
        oldVmt = *reinterpret_cast<uintptr_t**>(base);
        length = calculateLength(oldVmt) + 1;

        if (newVmt = findFreeDataPage(base, length))
            std::copy(oldVmt - 1, oldVmt - 1 + length, newVmt);
        assert(newVmt);
        init = true;
    }
    if (newVmt)
        *reinterpret_cast<uintptr_t**>(base) = newVmt + 1;
    return init;
}

void c_vmt::restore() noexcept
{
    if (base && oldVmt)
        *reinterpret_cast<uintptr_t**>(base) = oldVmt;
    if (newVmt)
        ZeroMemory(newVmt, length * sizeof(uintptr_t));
}
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
void c_hooks::init() noexcept {
    bspQuery.init(g_interfaces.engine->getBSPTreeQuery());
    client.init(g_interfaces.client);
    clientMode.init(g_memory.clientMode);
    engine.init(g_interfaces.engine);
    gameEventManager.init(g_interfaces.gameEventManager);
    modelRender.init(g_interfaces.modelRender);
    panel.init(g_interfaces.panel);
    sound.init(g_interfaces.sound);
    surface.init(g_interfaces.surface);
    svCheats.init(g_interfaces.cvar->findVar("sv_cheats"));
    viewRender.init(g_memory.viewRender);
}
void c_hooks::hook() noexcept {
    //SkinChanger::initializeKits();
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    //client.hookAt(37, frameStageNotify);
    //clientMode.hookAt(24, createMove);
    //panel.hookAt(41, paintTraverse);
    //gameEventManager.hookAt(9, fireEventClientSide);
}
void c_hooks::restore() noexcept {
    bspQuery.restore();
    client.restore();
    clientMode.restore();
    engine.restore();
    gameEventManager.restore();
    modelRender.restore();
    panel.restore();
    sound.restore();
    surface.restore();
    svCheats.restore();
    viewRender.restore();
}