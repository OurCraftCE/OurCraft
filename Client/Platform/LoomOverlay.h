#pragma once
#include "ContainerOverlay.h"

class LoomOverlay : public ContainerOverlay {
public:
    static LoomOverlay& Get() { static LoomOverlay i; return i; }
protected:
    void OnTick() override;
};
