#pragma once
#include "ContainerOverlay.h"

class HorseOverlay : public ContainerOverlay {
public:
    static HorseOverlay& Get() { static HorseOverlay i; return i; }
protected:
    void OnTick() override;
};
