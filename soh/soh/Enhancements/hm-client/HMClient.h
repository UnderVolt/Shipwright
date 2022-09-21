#pragma once

#include <vector>
#include "api/HMTypes.h"

class HMClient {
public:
    static HMClient* Instance;
    void Init();
    void Save(const AuthSession& auth);
    void SetUser(const User& user) {
        this->user = new User(user);
    }
    const User* GetUser() {
        return this->user;
    }
  private:
    User* user;
    AuthSession session;
    std::vector<CloudSave> saves;
};

#ifdef __cplusplus
extern "C" {
#endif

void HMClient_Init(void);

#ifdef __cplusplus
}
#endif