#pragma once

#include "HMTypes.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

#define HM_ENDPOINT "https://dev.shipofharkinian.com"

class HMApi {
public:
    // User:
    static Response LinkDevice(int32_t code, DeviceType device_type, const std::string& device_version, GameID game_id, const std::string& game_version);
    static Response GetUser(const AuthSession& auth);

    // Saves:

    static Response ListSaves(const AuthSession& auth, GameID game_id, const std::string& rom_version);
    static Response NewSave(const AuthSession& auth, const std::string& name, const std::string& blob, GameID game_id,
                            const std::string& rom_version, const std::string& game_version, int32_t version, Endianess endianess, const std::string& id = "");
    static Response LoadSave(const AuthSession& auth, const std::string& id);
    static Response DeleteSave(const AuthSession& auth, const std::string& id);
};