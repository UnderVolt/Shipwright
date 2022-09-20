#pragma once

#include <string>
#include <nlohmann/json.hpp>

struct CloudSave {
    int slot;
    std::string saveid;
};

struct AuthSession {
    std::string access_token;
    std::string refresh_token;
    double expires_in;
};
 
struct HMSettings {
    AuthSession session;
    CloudSave saves[];
};

struct User {
    std::string uuid;
    std::string user;
    std::string discriminator;
    std::string email;
    std::string authid;
    std::string created_at;
};

struct Save {
    std::string id;
    std::string name;
    std::string blob;
    std::string md5;
    GameID game_id;
    std::string game_version;
    std::string rom_version;
    int32_t version;
    Endianess endianess;
    nlohmann::json metadata;
    std::string updated_at;
};