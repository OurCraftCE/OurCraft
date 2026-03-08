#pragma once
#include "ContainerOverlay.h"

class BrewingOverlay : public ContainerOverlay {
public:
    static BrewingOverlay& Get() { static BrewingOverlay i; return i; }
protected:
    void OnTick() override;
};
