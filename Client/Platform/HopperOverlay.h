#pragma once
#include "ContainerOverlay.h"

class HopperOverlay : public ContainerOverlay {
public:
    static HopperOverlay& Get() { static HopperOverlay i; return i; }
protected:
    void OnTick() override;
};
