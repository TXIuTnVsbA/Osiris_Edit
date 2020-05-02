#include <functional>
#include <intrin.h>
#include <string>
#include <Windows.h>
#include <Psapi.h>
#include <algorithm>
#include <assert.h>

#include "../../../gui/imgui/imgui.h"
#include "../../../gui/imgui/imgui_impl_dx9.h"
#include "../../../gui/imgui/imgui_impl_win32.h"
#include "../../../config/config.h"
#include "../../../utils/console/console.h"
#include "../../../gui/gui.h"
#include "hooks.h"
#include "../interfaces/interfaces.h"
#include "../memory/memory.h"
#include "../../../lua/Clua.h"

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
#include "../../interfaces/SoundInfo.h"
#include "../../interfaces/SoundEmitter.h"
#include "../../interfaces/Surface.h"
#include "../../interfaces/UserCmd.h"
#include "../../interfaces/FrameStage.h"

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
static void __stdcall frameStageNotify(FrameStage stage) noexcept {
    for (auto hk : lua::hooks->getHooks("on_frame_stage_notify"))
    {
        try
        {
            auto result = hk.func((int)stage);
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
    g_hooks.client.callOriginal<void, 37>(stage);
}
static bool __stdcall createMove(float inputSampleTime, UserCmd* cmd) noexcept {
    auto result = g_hooks.clientMode.callOriginal<bool, 24>(inputSampleTime, cmd);
    if (!cmd->commandNumber)
        return result;

    for (auto hk : lua::hooks->getHooks("on_create_move"))
    {
        try
        {
            auto result = hk.func(cmd);
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


    return false;
}
static void __stdcall paintTraverse(unsigned int panel, bool forceRepaint, bool allowForce) noexcept
{
    if (g_interfaces.panel->getName(panel) == "MatSystemTopPanel") {
        for (auto hk : lua::hooks->getHooks("on_paint"))
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
    g_hooks.panel.callOriginal<void, 41>(panel, forceRepaint, allowForce);
}
////////////////////////////////////////////////////////////////
static LRESULT __stdcall wndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN && LOWORD(wParam) == VK_INSERT
        || ((msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) && VK_INSERT == VK_LBUTTON)
        || ((msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) && VK_INSERT == VK_RBUTTON)
        || ((msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) && VK_INSERT == VK_MBUTTON)
        || ((msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) && VK_INSERT == HIWORD(wParam) + 4)) {
        g_gui.isOpen = !g_gui.isOpen;
        if (!g_gui.isOpen) {
            ImGui::GetIO().MouseDown[0] = false;
            g_interfaces.inputSystem->resetInputState();
        }
    }
    LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (g_gui.isOpen && !ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam))
        return true;
    return CallWindowProc(g_hooks.originalWndProc, window, msg, wParam, lParam);
}

static HRESULT __stdcall present(IDirect3DDevice9* device, const RECT* src, const RECT* dest, HWND windowOverride, const RGNDATA* dirtyRegion) {
    static bool imguiInit{ ImGui_ImplDX9_Init(device) };

    if (g_gui.isOpen) {
        device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
        IDirect3DVertexDeclaration9* vertexDeclaration;
        device->GetVertexDeclaration(&vertexDeclaration);

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        g_gui.render();

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

        device->SetVertexDeclaration(vertexDeclaration);
        vertexDeclaration->Release();
    }
    return g_hooks.originalPresent(device, src, dest, windowOverride, dirtyRegion);
}

static HRESULT __stdcall reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) {
    ImGui_ImplDX9_InvalidateDeviceObjects();
    auto result = g_hooks.originalReset(device, params);
    ImGui_ImplDX9_CreateDeviceObjects();
    return result;
}

static void __stdcall lockCursor() noexcept
{
    if (g_gui.isOpen)
        return g_interfaces.surface->unlockCursor();
    return g_hooks.surface.callOriginal<void, 67>();
}
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
    originalWndProc = WNDPROC(SetWindowLongPtrA(FindWindowW(L"Valve001", nullptr), GWLP_WNDPROC, LONG_PTR(wndProc)));

    originalPresent = **reinterpret_cast<decltype(originalPresent)**>(g_memory.present);
    **reinterpret_cast<decltype(present)***>(g_memory.present) = present;
    originalReset = **reinterpret_cast<decltype(originalReset)**>(g_memory.reset);
    **reinterpret_cast<decltype(reset)***>(g_memory.reset) = reset;

    client.hookAt(37, frameStageNotify);
    clientMode.hookAt(24, createMove);
    panel.hookAt(41, paintTraverse);
    surface.hookAt(67, lockCursor);

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

    SetWindowLongPtrA(FindWindowW(L"Valve001", nullptr), GWLP_WNDPROC, LONG_PTR(originalWndProc));
    **reinterpret_cast<void***>(g_memory.present) = originalPresent;
    **reinterpret_cast<void***>(g_memory.reset) = originalReset;

    g_interfaces.inputSystem->enableInput(true);
}