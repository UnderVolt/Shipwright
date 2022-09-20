#pragma once

#include "api/HMTypes.h"

class HMClient {
public:
    static HMClient* Instance;
    void Init();
    void Save();

  private:
    HMSettings settings = { 0 };
};

#ifdef __cplusplus
extern "C" {
#endif

void HMClient_Init(void);

#ifdef __cplusplus
}
#endif