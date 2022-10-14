#include "HMApi.h"
#include <cpr/cpr.h>
#include <iostream>
#include <future>
#include "../utils/picosha2.h"

#define MAX_TIMEOUT 10 * 1000

void ProcessResponse(Response& rm, cpr::Response r) {
    if (r.status_code != ResponseCodes::OK) {
        bool isJson = r.header["Content-Type"] == "application/json";
        rm.code = (ResponseCodes)r.status_code;
        rm.error = isJson ? json::parse(r.text)["error"].get<std::string>() : r.text;
        return;
    }

    rm.code = ResponseCodes::OK;
    rm.error = "NONE";
    rm.data = r.text;
}

Response HMApi::LinkDevice(int32_t code, DeviceType device_type, const std::string & device_version, GameID game_id, const std::string & game_version, const std::string & hardware_id) {

    std::vector<unsigned char> hash(picosha2::k_digest_size);
    picosha2::hash256(hardware_id.begin(), hardware_id.end(), hash.begin(), hash.end());

    json body = {
        { "device_type", i_devices.at(device_type) },
        { "device_version", device_version },
        { "game_id", i_games.at(game_id) },
        { "game_version", game_version },
        { "hardware_id", picosha2::bytes_to_hex_string(hash.begin(), hash.end()) }
    };

    cpr::Response r = cpr::Post(
        cpr::Url{ HM_ENDPOINT "/api/v1/link/" + std::to_string(code) },
		cpr::Header{{ "Content-Type", "application/json" }},
        cpr::Body{body.dump()},
        cpr::Timeout{ MAX_TIMEOUT }
    );

    if (r.status_code != ResponseCodes::OK) {
        bool isJson = r.header["Content-Type"] == "application/json";
        return { (ResponseCodes) r.status_code, isJson ? json::parse(r.text)["error"].get<std::string>() : r.text };
    }

    json j = json::parse(r.text);

    return Response{ ResponseCodes::OK, "NONE", j.get<AuthSession>() };
}

Response HMApi::UnlinkDevice(const AuthSession& auth) {
    cpr::Response r = cpr::Delete(
        cpr::Url{ HM_ENDPOINT "/api/v1/unlink/" },
        cpr::Timeout{ MAX_TIMEOUT }
    );

    if (r.status_code != ResponseCodes::OK) {
        bool isJson = r.header["Content-Type"] == "application/json";
        return { (ResponseCodes) r.status_code, isJson ? json::parse(r.text)["error"].get<std::string>() : r.text };
    }

    return Response{ ResponseCodes::OK };
}

Response HMApi::RefreshUser(const AuthSession & auth) {

    json body = {
        { "access_token", auth.access_token },
        { "refresh_token", auth.refresh_token }
    };

    cpr::Response r = cpr::Post(
        cpr::Url{ HM_ENDPOINT "/api/v1/auth/refresh" },
        cpr::Header{
            { "authorization", body.dump() },
        },
        cpr::Timeout{ MAX_TIMEOUT }
    );

    if (r.status_code != ResponseCodes::OK) {
        bool isJson = r.header["Content-Type"] == "application/json";
        return { (ResponseCodes) r.status_code, isJson ? json::parse(r.text)["error"].get<std::string>() : r.text };
    }

    return Response{ ResponseCodes::OK, "NONE" };
}

Response HMApi::GetUser(const AuthSession & auth) {
    cpr::Response r = cpr::Get(
        cpr::Url{ HM_ENDPOINT "/api/v1/auth/me" },
        cpr::Header{
            { "authorization", "Auth " + auth.access_token }
        },
        cpr::Timeout{ MAX_TIMEOUT }
    );

    if(r.status_code == ResponseCodes::TOKEN_EXPIRED){
        HMApi::RefreshUser(auth);
    }

    if (r.status_code != ResponseCodes::OK) {
        bool isJson = r.header["Content-Type"] == "application/json";
        return { (ResponseCodes) r.status_code, isJson ? json::parse(r.text)["error"].get<std::string>() : r.text };
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
        cpr::Header{{ "authorization", "Auth " + auth.access_token }},
        cpr::Timeout{ MAX_TIMEOUT }
    );

    if(r.status_code == ResponseCodes::TOKEN_EXPIRED){
        HMApi::RefreshUser(auth);
    }

    if (r.status_code != ResponseCodes::OK) {
        bool isJson = r.header["Content-Type"] == "application/json";
        return { (ResponseCodes) r.status_code, isJson ? json::parse(r.text)["error"].get<std::string>() : r.text };
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
        cpr::Body{ body.dump() },
        cpr::Timeout{ MAX_TIMEOUT }
    );

    if(r.status_code == ResponseCodes::TOKEN_EXPIRED){
        HMApi::RefreshUser(auth);
    }

    if (r.status_code != ResponseCodes::OK) {
        bool isJson = r.header["Content-Type"] == "application/json";
        return { (ResponseCodes) r.status_code, isJson ? json::parse(r.text)["error"].get<std::string>() : r.text };
    }

    return Response{ ResponseCodes::OK, "NONE", r.text };
}

void HMApi::UploadSave(const AuthSession& auth, const std::string& name, const std::string& blob,
                                GameID game_id, const std::string& rom_version, const std::string& game_version, int32_t version,
                       Endianess endianess, std::function<void(Response&)> callback, const std::string& id) {

    std::vector<uint8_t> rawBlob(blob.begin(), blob.end());

    std::vector<unsigned char> hash(picosha2::k_digest_size);
    picosha2::hash256(blob.begin(), blob.end(), hash.begin(), hash.end());

    json body = {
        { "name", name },
        { "hash", picosha2::bytes_to_hex_string(hash.begin(), hash.end()) },
        { "blob", rawBlob },
        { "game_id", i_games.at(game_id) },
        { "version", version },
        { "endianess", i_endianess.at(endianess) },
        { "rom_version", rom_version },
        { "game_version", game_version },
    };

    cpr::PutCallback(
        [&](cpr::Response r) {
            Response m;
            ProcessResponse(m, r);
            callback(m);
        },
        cpr::Url{ HM_ENDPOINT "/api/v1/saves/" + id },
        cpr::Header{
            { "authorization", "Auth " + auth.access_token },
            { "Content-Type", "application/json" }
        },
        cpr::Body{ body.dump() },
        cpr::Timeout{ MAX_TIMEOUT }
    );
}

Response HMApi::LoadSave(const AuthSession & auth, const std::string & id) {
    cpr::Response r = cpr::Get(
        cpr::Url{ HM_ENDPOINT "/api/v1/saves/" + id },
        cpr::Header{
            { "authorization", "Auth " + auth.access_token }
        },
        cpr::Timeout{ MAX_TIMEOUT }
    );

    if(r.status_code == ResponseCodes::TOKEN_EXPIRED){
        HMApi::RefreshUser(auth);
    }

    if (r.status_code != ResponseCodes::OK) {
        bool isJson = r.header["Content-Type"] == "application/json";
        return { (ResponseCodes) r.status_code, isJson ? json::parse(r.text)["error"].get<std::string>() : r.text };
    }

    json j = json::parse(r.text);

    return Response{ ResponseCodes::OK, "NONE", j.get<CloudSave>() };
}

Response HMApi::DeleteSave(const AuthSession & auth, const std::string & id) {
    cpr::Response r = cpr::Delete(
        cpr::Url{ HM_ENDPOINT "/api/v1/saves/" + id },
        cpr::Header{
            { "authorization", "Auth " + auth.access_token }
        },
        cpr::Timeout{ MAX_TIMEOUT }
    );

    if(r.status_code == ResponseCodes::TOKEN_EXPIRED){
        HMApi::RefreshUser(auth);
    }

    if (r.status_code != ResponseCodes::OK) {
        bool isJson = r.header["Content-Type"] == "application/json";
        return { (ResponseCodes) r.status_code, isJson ? json::parse(r.text)["error"].get<std::string>() : r.text };
    }

    return Response{ ResponseCodes::OK };
}

void HMApi::LockSave(const AuthSession& auth, const std::string& id, const bool status, std::function<void(std::shared_ptr<Response>)> callback) {
    json body = {
        { "status", status }
    };

    std::shared_ptr<std::function<void(std::shared_ptr<Response>)>> rawCallback =
        std::make_shared<std::function<void(std::shared_ptr<Response>)>>(callback);

    cpr::PutCallback(
        [&rawCallback](cpr::Response r) {
            Response rm;
            if (r.status_code != ResponseCodes::OK) {
                bool isJson = r.header["Content-Type"] == "application/json";
                rm.code = (ResponseCodes)r.status_code;
                rm.error = isJson ? json::parse(r.text)["error"].get<std::string>() : r.text;
            } else {
                rm.code = ResponseCodes::OK;
                rm.error = "NONE";
                rm.data = r.text;
            }

            const std::function<void(std::shared_ptr<Response>)> sc = *rawCallback.get();
            sc(std::make_shared<Response>(rm));
        },
        cpr::Url{ HM_ENDPOINT "/api/v1/saves/status/" + id },
        cpr::Header{ { "authorization", "Auth " + auth.access_token }, { "Content-Type", "application/json" } },
        cpr::Body{ body.dump() }, cpr::Timeout{ MAX_TIMEOUT }
    );
}

Response HMApi::GetSaveLock(const AuthSession& auth, const std::string& id) {
    cpr::Response r = cpr::Get(
        cpr::Url{ HM_ENDPOINT "/api/v1/saves/status/" + id },
        cpr::Header{
            { "authorization", "Auth " + auth.access_token }
        },
        cpr::Timeout{ MAX_TIMEOUT }
    );

    if(r.status_code == ResponseCodes::TOKEN_EXPIRED){
        HMApi::RefreshUser(auth);
    }

    if (r.status_code != ResponseCodes::OK) {
        bool isJson = r.header["Content-Type"] == "application/json";
        return { (ResponseCodes) r.status_code, isJson ? json::parse(r.text)["error"].get<std::string>() : r.text };
    }

    json j = json::parse(r.text);

    return Response{ ResponseCodes::OK, "NONE", j["locked"].get<bool>() };
}

Response HMApi::UnlockAllSaves(const AuthSession& auth) {
    cpr::Response r = cpr::Put(
        cpr::Url{ HM_ENDPOINT "/api/v1/saves/status/" },
        cpr::Header{
            { "authorization", "Auth " + auth.access_token }
        },
        cpr::Timeout{ MAX_TIMEOUT }
    );

    if(r.status_code == ResponseCodes::TOKEN_EXPIRED){
        HMApi::RefreshUser(auth);
    }

    return Response{ ResponseCodes::OK };
}

void to_json(json& j, const LinkedSave& save) {
    j = json{ CNV(save, id), CNV(save, name) };
}

void to_json(json& j, const AuthSession& auth) {
    j = json{ CNV(auth, access_token), CNV(auth, refresh_token), CNV(auth, expires_in) };
}

void from_json(const json& j, AuthSession& auth) {
    LINK(auth, access_token);
    LINK(auth, refresh_token);
    LINK(auth, expires_in);
}

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
    LINK(save, player);
    LINK(save, lock_time);
    save.game_id = (GameID)(std::find(i_games.begin(), i_games.end(), j["game_id"]) - i_games.begin());
    save.endianess =
        (Endianess)(std::find(i_endianess.begin(), i_endianess.end(), j["endianess"]) - i_endianess.begin());
    LINK(save, blob);
    LINK(save, hash);
    LINK(save, metadata);
}

void from_json(const json& j, LinkedSave& save) {
    LINK(save, id);
    LINK(save, name);
}