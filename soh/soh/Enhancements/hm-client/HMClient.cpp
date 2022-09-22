#include "HMClient.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <libultraship/Cvar.h>
#include <libultraship/ImGuiImpl.h>

#include "utils/Base64.h"
#include "api/HMApi.h"
#include "Utils/StringHelper.h"
#include "variables.h"
#include <codecvt>
#include "../../SaveManager.h"
#include <libultraship/Hooks.h>

#ifdef _WIN32
#define NOGDI
#include <windows.h>
#elif __SWITCH__
#include <libultraship/SwitchImpl.h>
#endif
#include <libultraship/Lib/ImGui/imgui_internal.h>

#define CODE_BUFFER_SIZE 6 + 1
#define ROM_VERSION "Vanilla"

extern "C" {
#include <z64.h>
#include "variables.h"
#include "functions.h"
#include "macros.h"
extern "C" void FileChoose_SetupFileSlot(s16 slot);
extern GlobalContext* gGlobalCtx;
}

using json = nlohmann::json;
char inputBuffer[CODE_BUFFER_SIZE] = "";
char nameInputBuffer[CODE_BUFFER_SIZE] = "";

bool canEditSaves = true;

void HMClient::Init() {
    CVar* var = CVar_Get("gHMAccountData");
    if (var != nullptr && strcmp(var->value.valueStr, "None") != 0) {
        std::string data = base64_decode(std::string(var->value.valueStr));
        this->session = json::parse(data).get<AuthSession>();
        this->FetchData();
    }
}

void HMClient::Save(const AuthSession& auth) {
    json session = auth;
    std::string data = base64_encode(session.dump());
    CVar_SetString("gHMAccountData", data.c_str());
    SohImGui::RequestCvarSaveOnNextTick();
}

void HMClient::FetchData(const bool save) {
    const Response res = HMApi::GetUser(session);

    if (res.code != ResponseCodes::OK) {
        SPDLOG_ERROR(res.error);
        return;
    }

    const User user = std::any_cast<User>(res.data);
    this->SetUser(user);
    this->SetMaxSlots(std::min(3, (int)user.slots));

    if (save) {
        this->Save(session);
    }

	HMApi::UnlockAllSaves(this->session);

    const Response saves = HMApi::ListSaves(this->session, GameID::OOT, ROM_VERSION);

    if (saves.code != ResponseCodes::OK) {
        SPDLOG_ERROR(saves.error);
        return;
    }

    this->saves.clear();
	this->saves = std::any_cast<std::vector<CloudSave>>(saves.data);

    for (auto& link : this->linkedSaves) {
        if (std::find_if(this->saves.begin(), this->saves.end(), [link](CloudSave& save) -> bool { return save.id == link.id; }) == this->saves.end()) {
            link.name = "";
            link.id = "";
        }
    }
}

void HMClient::LoadSave(int slot) {
    SaveManager* sm = SaveManager::Instance;
    LinkedSave& link = this->linkedSaves.at(slot);

	if (link.id.empty()) {
        return;
    }

    auto save = std::find_if(this->saves.begin(), this->saves.end(),
                             [link](CloudSave& save) -> bool { return save.id == link.id; });

    if (!save->has_data) {
        return;
    }

    const std::string data(save->blob.begin(), save->blob.end());
    sm->LoadJsonFile(data, slot);
}

void HMClient::BindSave(const std::string& id, int slot) {
    SaveManager* sm = SaveManager::Instance;
    auto save =
        std::find_if(this->saves.begin(), this->saves.end(), [id](CloudSave& save) -> bool { return save.id == id; });

    if (save == this->saves.end()) {
        SPDLOG_ERROR("Failed to bind save");
        return;
    }

    sm->DeleteZeldaFile(slot);

    if (save->has_data) {
        this->LoadSave(slot);
        FileChoose_SetupFileSlot(slot);
        return;
    }
}

void HMClient::ResetSave(int slot) {
    SaveManager* sm = SaveManager::Instance;

    sm->ClearZeldaFile(slot);

    if (sm->SaveFile_Exist(slot)) {
        sm->LoadFile(slot);
    }
}

void HMClient::SetLockSave(int slot, bool status) {
    LinkedSave& currentSave = this->GetLinkedSaves().at(slot);

    if (currentSave.id.empty()) {
        return;
    }

    HMApi::LockSave(this->GetSession(), currentSave.id, status);
}

void HMClient::UploadSave(int slot, const std::string& data) {
    const SaveManager* sm = SaveManager::Instance;
    LinkedSave& currentSave = this->GetLinkedSaves().at(slot);

    auto save = std::find_if(this->saves.begin(), this->saves.end(),
                             [currentSave](CloudSave& save) -> bool { return save.id == currentSave.id; });

    if (save == this->saves.end()) {
        SPDLOG_ERROR("Failed to find save");
        return;
    }

#if (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)) || defined(__BIG_ENDIAN__)
    Endianess endian = Endianess::BIG;
#else
    Endianess little = Endianess::LITTLE;
#endif

    const Response res = HMApi::UploadSave(this->session, currentSave.name, data, GameID::OOT, ROM_VERSION,
                                           std::string((char*)gBuildVersion), 1, little, currentSave.id);

    if (res.code != ResponseCodes::OK) {
        SPDLOG_ERROR(res.error);
        return;
    }

    SPDLOG_INFO("Successfully uploaded save!");
}

bool HMClient::CanLoadSave(int slot) {

    LinkedSave& currentSave = this->GetLinkedSaves().at(slot);

    const Response res = HMApi::GetSaveLock(this->GetSession(), currentSave.id);

    if (res.code != ResponseCodes::OK) {
        SPDLOG_ERROR(res.error);
        return false;
    }

    auto save = std::find_if(this->saves.begin(), this->saves.end(),
                             [currentSave](CloudSave& save) -> bool { return save.id == currentSave.id; });

    if (save == this->saves.end()) {
        SPDLOG_ERROR("Failed to find save");
        return false;
    }

    bool canLoad = std::any_cast<bool>(res.data);

    if (canLoad) {
        currentSave.id = "";
        currentSave.name = "";
        save->player = "[Blocked]";
        this->ResetSave(slot);
        return false;
    }

    return true;
}

bool HMClient::NeedsOnlineSave(int slot, const std::string& data) {
    HMClient* instance = HMClient::Instance;
    LinkedSave& currentSave = instance->GetLinkedSaves().at(slot);

	if (!currentSave.id.empty()) {
        instance->UploadSave(slot, data);
        return true;
    }

    return false;
}

bool HMClient::NeedsOnlineLoad(int slot) {
    HMClient* instance = HMClient::Instance;
    LinkedSave& currentSave = instance->GetLinkedSaves().at(slot);

    if (!currentSave.id.empty()) {
        instance->LoadSave(slot);
        return true;
    }

    return false;
}

void DrawLinkDeviceUI() {
    ImGui::Text("Welcome!");
    ImGui::Text("Enter the code that appears on your screen.");
    ImGui::Dummy(ImVec2(0, 10));
    ImGui::InputTextWithHint("##hmcode", "Code: 123456", inputBuffer, CODE_BUFFER_SIZE,
                             ImGuiInputTextFlags_Numerical | ImGuiInputTextFlags_CharsNoBlank);
    ImGui::Dummy(ImVec2(0, 10));

    if (ImGui::Button("Link") && strlen(inputBuffer) == (CODE_BUFFER_SIZE - 1)) {

#ifdef _WIN32
        HW_PROFILE_INFO hwInfo;
        GetCurrentHwProfile(&hwInfo);
        const DeviceType type = DeviceType::WINDOWS;
        const std::string version = StringHelper::Sprintf("Build: %d", GetVersion());
        const std::string hwid = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(hwInfo.szHwProfileGuid);
#elif defined(__SWITCH__)
        DeviceType type = DeviceType::SWITCH;
        std::string version = "Atmosphere";// Ship::Switch::GetSwitchVersion();
        std::string hwid    = "1.0"; // Ship::Switch::GetSwitchHWID();

        printf("Data: %s %s\n", hwid.c_str(), version.c_str());

#elif defined(__linux__)
        DeviceType type = DeviceType::LINUX;
#elif defined(__APPLE__)
        DeviceType type = DeviceType::MAC;
        std::string version;
        std::string line;
        std::ifstream file("/System/Library/CoreServices/SystemVersion.plist");
        if (file.is_open()) {
            while (getline(file, line)) {
                if (line.find("<key>ProductVersion</key>") != std::string::npos) {
                    getline(file, line);
                    version =
                        line.substr(line.find("<string>") + 8, line.find("</string>") - line.find("<string>") - 8);
                    break;
                }
            }
            file.close();
        }
        std::string hwid;
        std::string command = "ioreg -d2 -c IOPlatformExpertDevice | awk -F\\\" '/IOPlatformUUID/{print $(NF-1)}'";
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe)
            return -1;
        char buffer[128];
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != NULL)
                deviceId += buffer;
        }
        pclose(pipe);
#elif defined(__WIIU__)
        DeviceType type = DeviceType::WII_U;
#else
#error "Unsupported platform!"
#endif

        const Response res =
            HMApi::LinkDevice(atoi(inputBuffer), type, version, GameID::OOT, std::string((char*)gBuildVersion), hwid);

        printf("Response: %d\n", res.code);

        if (res.code != ResponseCodes::OK) {
            SPDLOG_ERROR(res.error);
        } else {
            HMClient* instance = HMClient::Instance;
            instance->SetSession(std::any_cast<AuthSession>(res.data));
            instance->FetchData(true);

            SPDLOG_INFO("Successfully linked device!");
        }
    }
}

void DrawSlotSelector(size_t slot) {
    HMClient* instance = HMClient::Instance;
    const User* user = instance->GetUser();

    std::vector<CloudSave>& saves = instance->GetSaves();
    std::vector<LinkedSave>& linkedSaves = instance->GetLinkedSaves();

    ImGui::Text("Slot: %d", slot);
    ImGui::SameLine();

    LinkedSave& currentSave = linkedSaves.at(slot);

    ImGui::SetNextItemWidth(150);
    if (ImGui::BeginCombo(StringHelper::Sprintf("##hmslot%d", slot).c_str(), currentSave.name.empty() ? "[None]" : currentSave.name.c_str())) {
        if (!saves.empty()) {
            for (size_t slotId = 0; slotId < saves.size(); slotId++) {
                const CloudSave& save = saves.at(slotId);

                const bool is_selected = save.id == currentSave.id;
                auto linkedSaveQuery = std::find_if(linkedSaves.begin(), linkedSaves.end(),
                                                    [save](LinkedSave& link) -> bool { return save.id == link.id; });

                if (linkedSaveQuery != linkedSaves.end())
                    continue;

				bool canSwapSave = save.player.empty() || save.player == instance->GetSession().access_token;

                if (!canSwapSave) {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                }

                if (ImGui::Selectable((canSwapSave ? save.name : (save.name + " [Locked]")).c_str(), is_selected)) {
                    currentSave.id = save.id;
                    currentSave.name = save.name;
                    instance->BindSave(currentSave.id, slot);
                }

				if (!canSwapSave) {
                    ImGui::PopItemFlag();
                    ImGui::PopStyleVar();
                }
            }
        }

        if (ImGui::Selectable("[None]", currentSave.name.empty())) {
            currentSave.id   = "";
            currentSave.name = "";
            instance->ResetSave(slot);
        }

        if (saves.size() < user->slots && ImGui::Selectable("[New Save]", false)) {
            std::string saveName = StringHelper::Sprintf("TemporalName[%d]", saves.size());
            const Response res = HMApi::NewSave(instance->GetSession(), saveName, GameID::OOT, ROM_VERSION,
                                                std::string((char*)gBuildVersion), 1.0);

            if (res.code != ResponseCodes::OK) {
                SPDLOG_ERROR(res.error);
            } else {
                std::string saveId = std::any_cast<std::string>(res.data);
                currentSave.id = saveId;
                currentSave.name = saveName;
                saves.push_back({ .id = saveId, .name = saveName });
            }
        }
        ImGui::EndCombo();
    }
    if (!currentSave.id.empty()) {
        ImGui::SameLine();
        if (ImGui::Button(("Delete##" + currentSave.id).c_str())) {

            const Response res = HMApi::DeleteSave(instance->GetSession(), currentSave.id);
            if (res.code != ResponseCodes::OK) {
                SPDLOG_ERROR(res.error);
            } else {
                std::erase_if(saves, [currentSave](const CloudSave& save) { return save.id == currentSave.id; });
                currentSave.id = "";
                currentSave.name = "";
                instance->ResetSave(slot);
                SPDLOG_INFO("Successfully removed device!");
            }
        }
    }
}

void DrawManagerUI(const User* user) {
    HMClient* instance = HMClient::Instance;
    ImGui::Text("Welcome %s!", user->user.c_str());
    ImGui::Dummy(ImVec2(0, 5));
    ImGui::Text("Linked Saves");
    ImGui::Dummy(ImVec2(0, 5));

    bool canSwapSave = gGlobalCtx == nullptr && canEditSaves;

    if (!canSwapSave) {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }

    for (size_t slot = 0; slot < instance->GetMaxSlots(); slot++) {
        DrawSlotSelector(slot);
    }
    if (ImGui::Button("Refresh")) {
        instance->FetchData();
    }

    ImGui::SameLine();

	if (ImGui::Button("Disconnect")) {
        CVar_SetString("gHMAccountData", "None");
        instance->Disconnect();
    }

	if (!canSwapSave) {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
}

void DrawHMGui(bool& open) {

    if (!open) {
        CVar_SetS32("gCloudSavesMenu", 0);
        return;
    }

    ImGui::Begin("Harbour Masters Account##HMClient", &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking);
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

extern "C" {
void HMClient_Init(void) {
    InitHMClient();
    HMClient::Instance->Init();

    Ship::RegisterHook<Ship::ExitGame>([] {
        if (gSaveContext.fileNum > 0 && gSaveContext.fileNum < 3)
            HMClient::Instance->SetLockSave(gSaveContext.fileNum, false);
    });

    Ship::RegisterHook<Ship::CrashGame>([] {
        if (gSaveContext.fileNum > 0 && gSaveContext.fileNum < 3)
            HMClient::Instance->SetLockSave(gSaveContext.fileNum, false);
    });
}

void HMClient_SetLockSave(int slot, bool status) {
    HMClient* instance = HMClient::Instance;
    instance->SetLockSave(slot, status);
}

void HMClient_SetEditEnabled(bool mode) {
    canEditSaves = mode;
}

bool HMClient_IsOnlineSave(int slot) {
    LinkedSave& currentSave = HMClient::Instance->GetLinkedSaves().at(slot);
    return !currentSave.id.empty();
}

bool HMClient_CanLoadSave(int slot) {
    HMClient* instance = HMClient::Instance;

    return instance->CanLoadSave(slot);
}
}