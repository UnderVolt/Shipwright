#pragma once

#include <vector>
#include "api/HMTypes.h"

class HMClient {
public:
    static HMClient* Instance;
    void Init();
    void Save(const AuthSession& auth);
    void FetchData(const bool save = false);
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
    const AuthSession& GetSession() {
        return this->session;
    }
    const User* GetUser() {
        return this->user;
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

#ifdef __cplusplus
extern "C" {
#endif

void HMClient_Init(void);

#ifdef __cplusplus
}
#endif