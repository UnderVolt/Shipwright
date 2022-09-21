#pragma once

#include <any>
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

enum DeviceType { WINDOWS, LINUX, MAC, XBOX, WII_U, SWITCH, IOS, ANDROID };

enum Endianess { BIG, LITTLE, NONE };

enum GameID { OOT };

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

struct LinkedSave {
    std::string id;
    std::string name;
};

struct AuthSession {
    std::string access_token;
    std::string refresh_token;
    uint64_t expires_in;
};

struct User {
    std::string uuid;
    std::string user;
    std::string discriminator;
    std::string email;
    std::string authid;
    std::string created_at;
    uint32_t slots;
};

struct CloudSave {
    std::string id;
    std::string name;
    std::vector<uint8_t> blob;
    std::string md5;
    GameID game_id;
    std::string game_version;
    std::string rom_version;
    bool has_data;
    uint32_t version;
    Endianess endianess;
    nlohmann::json metadata;
    std::string updated_at;
};

static std::vector<std::string> i_games = { "TLOZPO0" };
static std::vector<std::string> i_devices = { "windows", "linux", "mac", "xbox", "wiiu", "switch", "ios", "android", "web" };
static std::vector<std::string> i_endianess = { "big", "little", "none" };

#define LINK(type, key) j.at(#key).get_to(type.key)
#define CNV(type, key) { #key, type.key }

void to_json(json& j, const AuthSession& auth) {
    j = json{ CNV(auth, access_token), CNV(auth, refresh_token), CNV(auth, expires_in) };
}

void from_json(const json& j, AuthSession& auth) {
    LINK(auth, access_token);
    LINK(auth, refresh_token);
    LINK(auth, expires_in);
}

//void to_json(json& j, const LinkSave& save) {
//    j = json{ CNV(save, slot), CNV(save, save_id) };
//}

//void from_json(const json& j, LinkSave& save) {
//    LINK(save, slot);
//    LINK(save, save_id);
//}

void from_json(const json& j, User& user) {
    LINK(user, uuid);
    LINK(user, user);
    LINK(user, discriminator);
    LINK(user, email);
    LINK(user, authid);
    LINK(user, created_at);
    user.slots = (uint32_t) j["slots"];
}

void from_json(const json& j, CloudSave& save) {
    LINK(save, id);
    LINK(save, name);
    LINK(save, game_version);
    LINK(save, rom_version);
    LINK(save, updated_at);
    LINK(save, has_data);
    save.game_id = (GameID)(std::find(i_games.begin(), i_games.end(), j["game_id"]) - i_games.begin());
    save.endianess =
        (Endianess)(std::find(i_endianess.begin(), i_endianess.end(), j["endianess"]) - i_endianess.begin());
    LINK(save, blob);
    LINK(save, md5);
    LINK(save, metadata);
}