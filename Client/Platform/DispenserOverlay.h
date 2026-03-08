#pragma once
#include "ContainerOverlay.h"

class DispenserOverlay : public ContainerOverlay {
public:
    static DispenserOverlay& Get() { static DispenserOverlay i; return i; }
protected:
    void OnTick() override;
};
