#include "GlobalVars.h"
#include "UserCmd.h"
#include "../utils/interfaces/interfaces.h"
#include "Engine.h"
#include "Entity.h"
#include "EntityList.h"

float GlobalVars::serverTime(UserCmd* cmd) noexcept
{
    static int tick;
    static UserCmd* lastCmd;

    if (cmd) {
        if (!lastCmd || lastCmd->hasbeenpredicted)
            tick = g_interfaces.entityList->getEntity(g_interfaces.engine->getLocalPlayer())->tickBase();
        else
            tick++;
        lastCmd = cmd;
    }
    return tick * intervalPerTick;
}
