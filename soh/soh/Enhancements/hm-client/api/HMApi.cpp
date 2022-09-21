#include "HMApi.h"
#include <cpr/cpr.h>
#include <iostream>

Response HMApi::LinkDevice(int32_t code, DeviceType device_type, const std::string & device_version, GameID game_id, const std::string & game_version, const std::string & hardware_id) {

    json body = {
        { "device_type", i_devices.at(device_type) },
        { "device_version", device_version },
        { "game_id", i_games.at(game_id) },
        { "game_version", game_version },
        { "hardware_id", hardware_id }
    };

    cpr::Response r = cpr::Post(
        cpr::Url{ HM_ENDPOINT "/api/v1/link/" + std::to_string(code) },
		cpr::Header{{ "Content-Type", "application/json" }},
        cpr::Body{body.dump()}
    );

    if (r.status_code != ResponseCodes::OK) {
        json j = json::parse(r.text);
        return { (ResponseCodes) r.status_code, j["error"] };
    }

    json j = json::parse(r.text);

    return Response{ ResponseCodes::OK, "NONE", j.get<AuthSession>() };
}

Response HMApi::GetUser(const AuthSession & auth) {
    cpr::Response r = cpr::Get(
        cpr::Url{ HM_ENDPOINT "/api/v1/auth/me" },
        cpr::Header{
            { "authorization", "Auth " + auth.access_token }
        }
    );

    if (r.status_code != ResponseCodes::OK) {
        json j = json::parse(r.text);
        return { (ResponseCodes) r.status_code, j["error"] };
    }

    json j = json::parse(r.text);

    return Response{ ResponseCodes::OK, "NONE", j.get<User>() };
}

Response HMApi::ListSaves(const AuthSession & auth, GameID game_id, const std::string & rom_version) {
    cpr::Response r = cpr::Get(
        cpr::Url{ HM_ENDPOINT "/api/v1/saves/" },
        cpr::Parameters{
            { "game_id", i_games.at(game_id) },
            { "rom_version", rom_version }
        },
        cpr::Header{{ "authorization", "Auth " + auth.access_token }}
    );

    if (r.status_code != ResponseCodes::OK) {
        json j = json::parse(r.text);
        return { (ResponseCodes)r.status_code, j["error"] };
    }

    json jobj = json::parse(r.text);
    
    return Response{ ResponseCodes::OK, "NONE", jobj.get<std::vector<CloudSave>>() };
}

Response HMApi::NewSave(const AuthSession& auth, const std::string& name, GameID game_id, const std::string& rom_version, const std::string& game_version, int32_t version) {
    json body = {
        { "name", name },
        { "game_id", i_games.at(game_id) },
        { "version", version },
        { "rom_version", rom_version },
        { "game_version", game_version },
    };

    cpr::Response r = cpr::Post(
        cpr::Url{ HM_ENDPOINT "/api/v1/saves/" },
        cpr::Header{
            { "authorization", "Auth " + auth.access_token },
            { "Content-Type", "application/json" }
        },
        cpr::Body{ body.dump() }
    );

    if (r.status_code != ResponseCodes::OK) {
        json j = json::parse(r.text);
        return { (ResponseCodes)r.status_code, j["error"] };
    }

    return Response{ ResponseCodes::OK, "NONE", r.text };
}

Response HMApi::UploadSave(const AuthSession & auth, const std::string & name, const std::string & blob, GameID game_id, const std::string & rom_version, const std::string & game_version, int32_t version, Endianess endianess, const std::string& id) {
    
    std::vector<uint8_t> rawBlob(blob.begin(), blob.end());
	
    json body = {
        { "name", name },
        { "blob", rawBlob },
        { "game_id", i_games.at(game_id) },
        { "version", version },
        { "endianess", i_endianess.at(endianess) },
        { "rom_version", rom_version },
        { "game_version", game_version },
    };

    cpr::Response r = cpr::Put(
        cpr::Url{ HM_ENDPOINT "/api/v1/saves/" + id },
        cpr::Header{
            { "authorization", "Auth " + auth.access_token },
            { "Content-Type", "application/json" }
        },
        cpr::Body{ body.dump() }
    );

    if (r.status_code != ResponseCodes::OK) {
        json j = json::parse(r.text);
        return { (ResponseCodes) r.status_code, j["error"] };
    }

    return Response{ ResponseCodes::OK, "NONE", r.text };
}

Response HMApi::LoadSave(const AuthSession & auth, const std::string & id) {
    cpr::Response r = cpr::Get(
        cpr::Url{ HM_ENDPOINT "/api/v1/saves/" + id },
        cpr::Header{
            { "authorization", "Auth " + auth.access_token }
        }
    );

    if (r.status_code != ResponseCodes::OK) {
        json j = json::parse(r.text);
        return { (ResponseCodes)r.status_code, j["error"] };
    }

    json j = json::parse(r.text);

    return Response{ ResponseCodes::OK, "NONE", j.get<CloudSave>() };
}

Response HMApi::DeleteSave(const AuthSession & auth, const std::string & id) {
    cpr::Response r = cpr::Delete(
        cpr::Url{ HM_ENDPOINT "/api/v1/saves/" + id },
        cpr::Header{
            { "authorization", "Auth " + auth.access_token }
        }
    );

    if (r.status_code != ResponseCodes::OK) {
        json j = json::parse(r.text);
        return { (ResponseCodes)r.status_code, j["error"] };
    }

    return Response{ ResponseCodes::OK };
}