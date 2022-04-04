#pragma once

struct HookParameter {
    const char* name;
    void* parameter;
};

enum HookTable {
    CONTROLLER_READ,
    AUDIO_INIT,
    LOAD_TEXTURE,
    UPDATE_VOLUME,
    IMGUI_API_INIT,
    IMGUI_API_DRAW,
    WINDOW_API_INIT,
    WINDOW_API_HANDLE_EVENTS,
    WINDOW_API_START_FRAME,
    RMGR_LOAD_DLIST,
    // Graphics API Hooks
    GFX_PRE_START_FRAME,
    GFX_POST_START_FRAME,
    GFX_PRE_END_FRAME,
    GFX_POST_END_FRAME,
    GFX_ON_REZISE,
    GFX_INIT,
    GFX_SHUTDOWN,
    // End
    NULL_HOOK
};

#ifdef __cplusplus

#include <memory>
#include <unordered_map>

#define HOOK_PARAMETER(name, ptr) HookParameter({ name, static_cast<void*>(ptr) })


struct HookCall {
    HookTable event;
    std::unordered_map<unsigned long, void*> args;
    bool cancelled = false;

    void* getArgument(const char*);
};

typedef std::shared_ptr<HookCall> HookEvent;
typedef std::unordered_map<HookTable, std::vector<void (*)(HookEvent)>> HookTableMap;

struct HookListener {
    HookTable event;
    void (*callback)(HookEvent event);
    int priority = 0;
};

namespace Ship {
    void registerHookListener(const HookListener& listener);
    void bindHook(HookTable event);
    bool triggerHook(int length, ...);
    const HookTableMap& getHookTableMap();
}

#else

void bind_hook(char* name);
bool trigger_hook(int length, ...);

#endif