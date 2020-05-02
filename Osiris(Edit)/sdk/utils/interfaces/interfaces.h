#pragma once
#include <sstream>
#include <type_traits>
#include <Windows.h>
class Client;
class Cvar;
class Engine;
class EngineTrace;
class EntityList;
class GameEventManager;
class GameMovement;
class GameUI;
class InputSystem;
class Localize;
class MaterialSystem;
class ModelInfo;
class ModelRender;
class Panel;
class PhysicsSurfaceProps;
class Prediction;
class RenderView;
class ResourceAccessControl;
class Surface;
class Sound;
class SoundEmitter;
class StudioRender;
class c_interfaces {
public:
    void init() noexcept;
    Client* client;
    Cvar* cvar;
    Engine* engine;
    EngineTrace* engineTrace;
    EntityList* entityList;
    GameEventManager* gameEventManager;
    GameMovement* gameMovement;
    GameUI* gameUI;
    InputSystem* inputSystem;
    Localize* localize;
    MaterialSystem* materialSystem;
    ModelInfo* modelInfo;
    ModelRender* modelRender;
    Panel* panel;
    PhysicsSurfaceProps* physicsSurfaceProps;
    Prediction* prediction;
    RenderView* renderView;
    ResourceAccessControl* resourceAccessControl;
    Surface* surface;
    Sound* sound;
    SoundEmitter* soundEmitter;
    StudioRender* studioRender;
public:
    template <typename T>
    static auto find(const wchar_t* module, const char* name) noexcept
    {
        if (HMODULE moduleHandle = GetModuleHandleW(module))
            if (const auto createInterface = reinterpret_cast<std::add_pointer_t<T * (const char* name, int* returnCode)>>(GetProcAddress(moduleHandle, "CreateInterface")))
                if (T* foundInterface = createInterface(name, nullptr))
                    return foundInterface;

        MessageBoxA(nullptr, (std::ostringstream{ } << "Failed to find " << name << " interface!").str().c_str(), "Osiris", MB_OK | MB_ICONERROR);
        std::exit(EXIT_FAILURE);
    }
};
extern c_interfaces g_interfaces;