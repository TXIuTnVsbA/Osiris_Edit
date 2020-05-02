#include "interfaces.h"

#define GAME_INTERFACE(type, name, module, version) \
name = find<type>(L##module, version);

c_interfaces g_interfaces;
void c_interfaces::init() noexcept {
    GAME_INTERFACE(Client, client, "client_panorama", "VClient018");
    GAME_INTERFACE(Cvar, cvar, "vstdlib", "VEngineCvar007");
    GAME_INTERFACE(Engine, engine, "engine", "VEngineClient014");
    GAME_INTERFACE(EngineTrace, engineTrace, "engine", "EngineTraceClient004");
    GAME_INTERFACE(EntityList, entityList, "client_panorama", "VClientEntityList003");
    GAME_INTERFACE(GameEventManager, gameEventManager, "engine", "GAMEEVENTSMANAGER002");
    GAME_INTERFACE(GameMovement, gameMovement, "client_panorama", "GameMovement001");
    GAME_INTERFACE(GameUI, gameUI, "client_panorama", "GameUI011");
    GAME_INTERFACE(InputSystem, inputSystem, "inputsystem", "InputSystemVersion001");
    GAME_INTERFACE(Localize, localize, "localize", "Localize_001");
    GAME_INTERFACE(MaterialSystem, materialSystem, "materialsystem", "VMaterialSystem080");
    GAME_INTERFACE(ModelInfo, modelInfo, "engine", "VModelInfoClient004");
    GAME_INTERFACE(ModelRender, modelRender, "engine", "VEngineModel016");
    GAME_INTERFACE(Panel, panel, "vgui2", "VGUI_Panel009");
    GAME_INTERFACE(PhysicsSurfaceProps, physicsSurfaceProps, "vphysics", "VPhysicsSurfaceProps001");
    GAME_INTERFACE(Prediction, prediction, "client_panorama", "VClientPrediction001");
    GAME_INTERFACE(RenderView, renderView, "engine", "VEngineRenderView014");
    GAME_INTERFACE(Surface, surface, "vguimatsurface", "VGUI_Surface031");
    GAME_INTERFACE(Sound, sound, "engine", "IEngineSoundClient003");
    GAME_INTERFACE(SoundEmitter, soundEmitter, "soundemittersystem", "VSoundEmitter003");
    GAME_INTERFACE(StudioRender, studioRender, "studiorender", "VStudioRender026");
}