#pragma once

#include <vector>
#include "api/HMTypes.h"

struct HMSettings {
    AuthSession session;
    std::vector<CloudSave> saves;
};

class HMClient {
public:
    static HMClient* Instance;
    void Init();
    void Save();

  private:
    HMSettings settings;
};

#ifdef __cplusplus
extern "C" {
#endif

void HMClient_Init(void);

#ifdef __cplusplus
}
#endif