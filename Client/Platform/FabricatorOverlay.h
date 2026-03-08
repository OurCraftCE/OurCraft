#pragma once
#include "ContainerOverlay.h"

class FabricatorOverlay : public ContainerOverlay {
public:
    static FabricatorOverlay& Get() { static FabricatorOverlay i; return i; }
protected:
    void OnTick() override;
};
