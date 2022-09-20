#include "HMClient.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <libultraship/Cvar.h>
#include <libultraship/ImGuiImpl.h>

#include "utils/Base64.h"
#include "api/HMApi.h"

using json = nlohmann::json;

void HMClient::Init() {
    CVar* var = CVar_Get("gHMAccountData");
    if (var != nullptr) {
        std::string data = base64_decode(std::string(var->value.valueStr));
        this->settings = json::parse(data).get<HMSettings>();
        return;
    }
}

void HMClient::Save() {

}

char* inputBuffer;
#define CODE_BUFFER_SIZE 6 + 1

void DrawHMGui(bool& open) {

    if (!open) {
        CVar_SetS32("gCloudSavesMenu", 0);
        return;
    }

    ImGui::Begin("Harbour Masters Account##HMClient", &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
    ImGui::Text("Welcome!");
    ImGui::Text("Enter the code that appears on your screen.");
    ImGui::Dummy(ImVec2(0, 10));
    ImGui::InputTextWithHint("##hmcode", "Code: 123456", inputBuffer, CODE_BUFFER_SIZE, ImGuiInputTextFlags_Numerical | ImGuiInputTextFlags_CharsNoBlank);
    ImGui::Dummy(ImVec2(0, 10));
    if (ImGui::Button("Link")) {
        std::cout << inputBuffer << std::endl;
    }
    ImGui::End();
}

void InitHMClient() {
    inputBuffer = new char[CODE_BUFFER_SIZE];
    strcpy(inputBuffer, "");
    SohImGui::AddWindow("Cloud Saves", "HMClient", DrawHMGui);
}

extern "C" void HMClient_Init(void) {
    InitHMClient();
    HMClient::Instance->Init();
}
