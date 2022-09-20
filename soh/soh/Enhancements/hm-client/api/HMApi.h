#pragma once

#include "HMTypes.h"
#include "nlohmann/json.hpp"
#include <any>

using json = nlohmann::json;

enum DeviceType {
    WINDOWS,
    LINUX,
    MAC,
    XBOX,
    WII_U,
    SWITCH,
    IOS,
    ANDROID
};

enum Endianess {
    BIG,
    LITTLE,
    NONE
};

enum GameID {
    OOT
};

enum ResponseCodes {
    OK = 200,
    NOT_VERIFIED = 201,
    INVALID_CREDENTIALS = 202,
    DUPLICATED_ACCOUNT = 203,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    INTERNAL_SERVER_ERROR = 500,
    SERVICE_UNAVAILABLE = 503,
};

struct Response {
    ResponseCodes code;
    std::string error;
    std::any data;
};

#define HM_ENDPOINT "https://dev.shipofharkinian.com"

static std::string _devices[] = {
    "windows", "linux"  , "mac",
    "xbox"   , "wiiu"   , "switch",
    "ios"    , "android", "web"
};
static std::string _games[] = { "TLOZPO0" };
static std::string _endianess[] = { "big", "little", "none" };

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