#pragma once
#include "ContainerOverlay.h"

class ChestOverlay : public ContainerOverlay {
public:
    static ChestOverlay& Get() { static ChestOverlay i; return i; }
protected:
    void OnTick() override;
};
