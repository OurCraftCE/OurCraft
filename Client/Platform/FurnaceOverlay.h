#pragma once
#include "ContainerOverlay.h"

class FurnaceOverlay : public ContainerOverlay {
public:
    static FurnaceOverlay& Get() { static FurnaceOverlay i; return i; }
protected:
    void OnTick() override;
};
