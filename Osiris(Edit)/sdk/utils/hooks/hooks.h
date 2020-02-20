#pragma once
#include <d3d9.h>
#include <type_traits>
struct SoundInfo;
class c_vmt {
public:
    bool init(void* const) noexcept;
    void restore() noexcept;

    template<typename T>
    void hookAt(size_t index, T fun) const noexcept
    {
        if (index <= length)
            newVmt[index + 1] = reinterpret_cast<uintptr_t>(fun);
    }

    template<typename T, std::size_t Idx, typename ...Args>
    constexpr auto callOriginal(Args... args) const noexcept
    {
        return reinterpret_cast<T(__thiscall*)(void*, Args...)>(oldVmt[Idx])(base, args...);
    }

    template<typename T, typename ...Args>
    constexpr auto getOriginal(size_t index, Args... args) const noexcept
    {
        return reinterpret_cast<T(__thiscall*)(void*, Args...)>(oldVmt[index]);
    }
private:
    static uintptr_t* findFreeDataPage(void* const, size_t) noexcept;
    static auto calculateLength(uintptr_t*) noexcept;
    void* base = nullptr;
    uintptr_t* oldVmt = nullptr;
    uintptr_t* newVmt = nullptr;
    size_t length = 0;
};
class c_hooks {
public:
    void init() noexcept;
    void hook() noexcept;
    void restore() noexcept;

    WNDPROC originalWndProc;
    std::add_pointer_t<HRESULT __stdcall(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*)> originalPresent;
    std::add_pointer_t<HRESULT __stdcall(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*)> originalReset;
    std::add_pointer_t<int __fastcall(SoundInfo&)> originalDispatchSound;

    c_vmt bspQuery;
    c_vmt client;
    c_vmt clientMode;
    c_vmt engine;
    c_vmt gameEventManager;
    c_vmt modelRender;
    c_vmt panel;
    c_vmt sound;
    c_vmt surface;
    c_vmt svCheats;
    c_vmt viewRender;
};
extern c_hooks g_hooks;