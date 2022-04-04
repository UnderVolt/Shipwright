#include "SMod.h"
#include "CoreLoader.h"
#include "SohImGuiImpl.h"
#include "SohHooks.h"
#include "Models/gMarioShieldDL.h"

void BindGameHooks() {
    Ship::registerHookListener({ RMGR_LOAD_DLIST, [](const HookEvent event) {
        Gfx** dlist = (Gfx**)event->getArgument("dlist");
        std::string path((const char*)event->getArgument("path"));

        if (path.find("HylianShield") != std::string::npos) {
            (*dlist) = gShieldDL;
        }
    }});
}

SHIP_MAIN_FUNC() {

    BindGameHooks();

    INFO("Ship Mod loaded");

    return new ModMetadata{
        "Mario Shield",
        "Lywx & MelonSpeedruns",
        "v0.1"
    };
}

// Predefined functions - don't modify it

extern "C" __declspec(dllexport) void BindConsole(void* ref) {
    SohImGui::console = *(Console**)ref;
}

extern "C" __declspec(dllexport) void* DumpHookMGR() {
    const HookTableMap& map = Ship::getHookTableMap();
    return (void*) &map;
}