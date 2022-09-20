#include "HMApi.h"
#include <cpr/cpr.h>

Response HMApi::LinkDevice(int32_t code, DeviceType device_type, const std::string & device_version, GameID game_id, const std::string & game_version) {

	json body = {
        { "device_type", _devices[device_type] },
        { "device_version", device_version },
        { "game_id", _games[game_id] },
        { "game_version", game_version },
		{ "hardware_id", "tbd" }
    };

    cpr::Response r = cpr::Post(
        cpr::Url{ HM_ENDPOINT "/api/v1/link/" + std::to_string(code) },
		cpr::Body{body.dump()}
    );

	if (r.status_code != ResponseCodes::OK) {
        json j = json::parse(r.text);
        return { (ResponseCodes) r.status_code, j["error"] };
    }

    json j = json::parse(r.text);

    return { ResponseCodes::OK, "NONE", j["token"].get<AuthSession>() };
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

    return { ResponseCodes::OK, "NONE", j.get<User>() };
}

Response HMApi::ListSaves(const AuthSession & auth, GameID game_id, const std::string & rom_version) {
    cpr::Response r = cpr::Get(
        cpr::Url{ HM_ENDPOINT "/api/v1/saves/" },
        cpr::Parameters{ 
            { "game_id", _games[game_id] },
			{ "rom_version", rom_version }
        },
        cpr::Header{
		    { "authorization", "Auth " + auth.access_token }
        }
    );

    if (r.status_code != ResponseCodes::OK) {
        json j = json::parse(r.text);
        return { (ResponseCodes)r.status_code, j["error"] };
    }

    json j = json::parse(r.text);

	std::vector<Save> saves;
	
    for (auto& raw : j["saves"]) {
        saves.push_back(raw.get<Save>());
    }

    return { ResponseCodes::OK, "NONE", saves };
}

Response HMApi::NewSave(const AuthSession & auth, const std::string & name, const std::string & blob, GameID game_id, const std::string & rom_version, const std::string & game_version, int32_t version, Endianess endianess, const std::string& id) {
    json body = {
        { "name", name },
        { "blob", blob },
        { "game_id", _games[game_id] },
        { "version", version },
        { "endianess", _endianess[endianess] },
        { "rom_version", rom_version },
        { "game_version", game_version },
    };
	
    cpr::Response r = cpr::Post(cpr::Url{ HM_ENDPOINT "/api/v1/saves/" + id }, cpr::Body{ body.dump() });

    if (r.status_code != ResponseCodes::OK) {
        json j = json::parse(r.text);
        return { (ResponseCodes) r.status_code, j["error"] };
    }

    json j = json::parse(r.text);

    return { ResponseCodes::OK, "NONE", j["id"] };
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

    return { ResponseCodes::OK, "NONE", j["save"].get<Save>() };
}

Response HMApi::DeleteSave(const AuthSession & auth, const std::string & id) {
    cpr::Response r = cpr::Delete(cpr::Url{ HM_ENDPOINT "/api/v1/saves/" + id });

    if (r.status_code != ResponseCodes::OK) {
        json j = json::parse(r.text);
        return { (ResponseCodes)r.status_code, j["error"] };
    }

    return { ResponseCodes::OK };
}