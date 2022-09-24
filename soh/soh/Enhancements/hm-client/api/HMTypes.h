#ifndef HMTypes
#define HMTypes

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
    TOKEN_EXPIRED = 204,
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
    std::string player;
    uint64_t lock_time;
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

void to_json(json& j, const LinkedSave& save);
void to_json(json& j, const AuthSession& auth);
void from_json(const json& j, AuthSession& auth);
void from_json(const json& j, User& user);
void from_json(const json& j, CloudSave& save);
void from_json(const json& j, LinkedSave& save);
#endif