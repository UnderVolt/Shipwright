#pragma once

#ifdef __cplusplus

#include <vector>
#include "api/HMTypes.h"

class HMClient {
public:
    static HMClient* Instance;
    void Init();
    void Save(const AuthSession& auth);
    void FetchData(const bool save = false);

    void BindSave(const std::string& id, int slot);
    void LoadSave(int slot);
    void ResetSave(int slot);
    bool CanLoadSave(int slot);
    void UploadSave(int slot, const std::string& data);
    void SetLockSave(int slot, bool status);

    static bool NeedsOnlineSave(int slot, const std::string& data);
    static bool NeedsOnlineLoad(int slot);

    void SetMaxSlots(int32_t slots) {
        this->linkedSaves.resize(slots);
        this->maxSlots = slots;
    }
    const size_t GetMaxSlots() {
        return this->maxSlots;
    }
    void SetUser(const User& user) {
        this->user = new User(user);
    }
    void SetSession(const AuthSession& auth) {
        this->session = auth;
    }
    void Disconnect() {
        this->linkedSaves.clear();
        this->linkedSaves.resize(3);
        this->saves.clear();
        this->session = {};
        this->user = nullptr;
    }
    const AuthSession& GetSession() {
        return this->session;
    }
    const User* GetUser() {
        return this->user;
    }
    const bool IsLoggedIn() {
        return this->user != nullptr;
    }
    std::vector<LinkedSave>& GetLinkedSaves() {
        return this->linkedSaves;
    }
    std::vector<CloudSave>& GetSaves() {
        return this->saves;
    }
 private:
    size_t maxSlots;
    User* user;
    AuthSession session;
    std::vector<CloudSave> saves;
    std::vector<LinkedSave> linkedSaves;
};

extern "C" {
#endif

void HMClient_Init(void);
void HMClient_SetLockSave(int slot, bool status);
void HMClient_SetEditEnabled(bool enabled);
bool HMClient_IsOnlineSave(int slot);
bool HMClient_CanLoadSave(int slot);

#ifdef __cplusplus
}
#endif