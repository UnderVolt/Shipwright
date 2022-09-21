#include "HMClient.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <libultraship/Cvar.h>
#include <libultraship/ImGuiImpl.h>

#include "utils/Base64.h"
#include "api/HMApi.h"
#include "utils/StringHelper.h"
#include "variables.h"
#include <codecvt>

#ifdef _WIN32
#define NOGDI
#include <windows.h>
#endif

using json = nlohmann::json;

void HMClient::Init() {
    CVar* var = CVar_Get("gHMAccountData");
    if (var != nullptr) {
        std::string data = base64_decode(std::string(var->value.valueStr));
        this->session = json::parse(data).get<AuthSession>();

        const Response res = HMApi::GetUser(session);

        if (res.code != ResponseCodes::OK) {
            SPDLOG_ERROR(res.error);
            return;
        }

        const User user = std::any_cast<User>(res.data);
        this->SetUser(user);
    }
}

void HMClient::Save(const AuthSession& auth) {
    json session = auth;
    std::string data = base64_encode(session.dump());
    CVar_SetString("gHMAccountData", data.c_str());
}

char inputBuffer[6 + 1] = "";
#define CODE_BUFFER_SIZE 6 + 1

void DrawLinkDeviceUI() {
    ImGui::Text("Welcome!");
    ImGui::Text("Enter the code that appears on your screen.");
    ImGui::Dummy(ImVec2(0, 10));
    ImGui::InputTextWithHint("##hmcode", "Code: 123456", inputBuffer, sizeof(inputBuffer) / sizeof(char),
                             ImGuiInputTextFlags_Numerical | ImGuiInputTextFlags_CharsNoBlank);
    ImGui::Dummy(ImVec2(0, 10));
    if (ImGui::Button("Link") && strlen(inputBuffer) == 6) {

#ifdef _WIN32
        HW_PROFILE_INFO hwInfo;
        GetCurrentHwProfile(&hwInfo);
        const DeviceType type = DeviceType::WINDOWS;
        const std::string version = StringHelper::Sprintf("Build: %d", GetVersion());
        const std::string hwid = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(hwInfo.szHwProfileGuid);
#elif defined(__linux__)
        DeviceType type = DeviceType::LINUX;
#elif defined(__APPLE__)
        DeviceType type = DeviceType::MAC;
#elif defined(__SWITCH__)
        DeviceType type = DeviceType::SWITCH;
        std::string version = StringHelper::Sprintf("OS: %d", GetVersion());
#elif defined(__WIIU__)
        DeviceType type = DeviceType::WII_U;
#endif

        const Response res =
            HMApi::LinkDevice(atoi(inputBuffer), type, version, GameID::OOT, std::string((char*)gBuildVersion), hwid);

        if (res.code != ResponseCodes::OK) {
            SPDLOG_ERROR(res.error);
        } else {
            const AuthSession session = std::any_cast<AuthSession>(res.data);
            const User user = std::any_cast<User>(HMApi::GetUser(session).data);
            HMClient::Instance->SetUser(user);
            HMClient::Instance->Save(session);
            SohImGui::RequestCvarSaveOnNextTick();
            SPDLOG_INFO("Successfully linked device!");
        }
    }
}

void DrawManagerUI(const User* user) {
    ImGui::Text("Welcome %s!", user->user.c_str());
    ImGui::Text("We fucking did it!");
}

void DrawHMGui(bool& open) {

    if (!open) {
        CVar_SetS32("gCloudSavesMenu", 0);
        return;
    }

    ImGui::Begin("Harbour Masters Account##HMClient", &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking);
    const User* user = HMClient::Instance->GetUser();
    if (user != nullptr) {
        DrawManagerUI(user);
    } else {
        DrawLinkDeviceUI();
    }
    ImGui::End();
}

void InitHMClient() {
    SohImGui::AddWindow("Cloud Saves", "HMClient", DrawHMGui);
}

extern "C" void HMClient_Init(void) {
    InitHMClient();
    HMClient::Instance->Init();
}
